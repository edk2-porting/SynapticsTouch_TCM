// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 

#include <compat.h>
#include <internal.h>
#include <controller.h>
#include <device.h>
#include <spb.h>
#include <idle.h>
#include <hid.h>
#include <device.tmh>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, OnD0Exit)
#endif

BOOLEAN
OnInterruptIsr(
	IN WDFINTERRUPT Interrupt,
	IN ULONG MessageID
)
/*++

  Routine Description:

	This routine responds to interrupts generated by the
	controller. If one is recognized, it queues a DPC for
	processing.

	This is a PASSIVE_LEVEL ISR. ACPI should specify
	level-triggered interrupts when using Synaptics 3202.

  Arguments:

	Interrupt - a handle to a framework interrupt object
	MessageID - message number identifying the device's
		hardware interrupt message (if using MSI)

  Return Value:

	TRUE if interrupt recognized.

--*/
{
	PDEVICE_EXTENSION devContext;
	NTSTATUS status;
	WDFREQUEST request;
	BOOLEAN servicingComplete;
	DEV_REPORT hidReportFromDriver;
	PDEV_REPORT hidReportRequestBuffer;
	size_t hidReportRequestBufferLength;

	UNREFERENCED_PARAMETER(MessageID);

	status = STATUS_SUCCESS;
	servicingComplete = FALSE;
	devContext = GetDeviceContext(WdfInterruptGetDevice(Interrupt));
	request = NULL;


	//
	// If we're in diagnostic mode, let the diagnostic application handle
	// interrupt servicing
	//
	if (devContext->DiagnosticMode != FALSE)
	{
		goto exit;
	}


	//
	// Service the device interrupt
	//
	while (servicingComplete == FALSE)
	{
		//
		// Service touch interrupts. Success indicates we have a report
		// to complete to Hid. ServicingComplete indicates another report
		// is required to continue servicing this interrupt.
		//
		if (!NT_SUCCESS(TchServiceInterrupts(
			devContext->TouchContext,
			&devContext->I2CContext,
			&hidReportFromDriver,
			devContext->InputMode,
			&servicingComplete)))
		{
			//
			// hidReportFromDriver was not filled
			//
			continue;
		}

		//
		// Complete a HIDClass request if one is available
		//
		status = WdfIoQueueRetrieveNextRequest(
			devContext->PingPongQueue,
			&request);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_ERROR,
				TRACE_REPORTING,
				"No request pending from HIDClass, ignoring report - 0x%08lX",
				status);



			continue;
		}

		//
		// Validate an output buffer was provided
		//
		status = WdfRequestRetrieveOutputBuffer(
			request,
			sizeof(DEV_REPORT),
			&hidReportRequestBuffer,
			&hidReportRequestBufferLength);

		if (!NT_SUCCESS(status))
		{
			Trace(
				TRACE_LEVEL_VERBOSE,
				TRACE_SAMPLES,
				"Error retrieving HID read request output buffer - 0x%08lX",
				status);
		}
		else
		{
			//
			// Validate the size of the output buffer
			//
			if (hidReportRequestBufferLength < sizeof(DEV_REPORT))
			{
				status = STATUS_BUFFER_TOO_SMALL;

				Trace(
					TRACE_LEVEL_VERBOSE,
					TRACE_SAMPLES,
					"Error HID read request buffer is too small (%I64x bytes) - 0x%08lX",
					hidReportRequestBufferLength,
					status);
			}
			else
			{
				RtlCopyMemory(
					hidReportRequestBuffer,
					&hidReportFromDriver,
					sizeof(DEV_REPORT));

				WdfRequestSetInformation(request, sizeof(DEV_REPORT));
			}
		}

		WdfRequestComplete(request, status);
	}

exit:
	return TRUE;
}

NTSTATUS
OnD0Entry(
	IN WDFDEVICE Device,
	IN WDF_POWER_DEVICE_STATE PreviousState
)
/*++

Routine Description:

	This routine will power on the hardware

Arguments:

	Device - WDF device to power on
	PreviousState - Prior power state

Return Value:

	NTSTATUS indicating success or failure

*/
{
	NTSTATUS status;
	PDEVICE_EXTENSION devContext;

	devContext = GetDeviceContext(Device);

	UNREFERENCED_PARAMETER(PreviousState);

	status = TchWakeDevice(devContext->TouchContext, &devContext->I2CContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Error setting device to D0 - 0x%08lX",
			status);
	}

	//
	// N.B. This RMI chip's IRQ is level-triggered, but cannot be enabled in
	//      ACPI until passive-level interrupt handling is added to the driver.
	//      Service chip in case we missed an edge during D3 or boot-up.
	//
	devContext->ServiceInterruptsAfterD0Entry = TRUE;

	//
	// Complete any pending Idle IRPs
	//
	TchCompleteIdleIrp(devContext);

	return status;
}

NTSTATUS
OnD0Exit(
	IN WDFDEVICE Device,
	IN WDF_POWER_DEVICE_STATE TargetState
)
/*++

Routine Description:

	This routine will power down the hardware

Arguments:

	Device - WDF device to power off

	PreviousState - Prior power state

Return Value:

	NTSTATUS indicating success or failure

*/
{
	NTSTATUS status;
	PDEVICE_EXTENSION devContext;

	PAGED_CODE();

	devContext = GetDeviceContext(Device);

	UNREFERENCED_PARAMETER(TargetState);

	status = TchStandbyDevice(devContext->TouchContext, &devContext->I2CContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_POWER,
			"Error exiting D0 - 0x%08lX",
			status);
	}

	return status;
}

NTSTATUS
OnPrepareHardware(
	IN WDFDEVICE FxDevice,
	IN WDFCMRESLIST FxResourcesRaw,
	IN WDFCMRESLIST FxResourcesTranslated
)
/*++

  Routine Description:

	This routine is called by the PnP manager and supplies thie device instance
	with it's SPB resources (CmResourceTypeConnection) needed to find the I2C
	driver.

  Arguments:

	FxDevice - a handle to the framework device object
	FxResourcesRaw - list of translated hardware resources that
		the PnP manager has assigned to the device
	FxResourcesTranslated - list of raw hardware resources that
		the PnP manager has assigned to the device

  Return Value:

	NTSTATUS indicating sucess or failure

--*/
{
	NTSTATUS status;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR res;
	PDEVICE_EXTENSION devContext;
	ULONG resourceCount;
	ULONG i;

	UNREFERENCED_PARAMETER(FxResourcesRaw);

	status = STATUS_INSUFFICIENT_RESOURCES;
	devContext = GetDeviceContext(FxDevice);

	//
	// Get the resouce hub connection ID for our I2C driver
	//
	resourceCount = WdfCmResourceListGetCount(FxResourcesTranslated);

	for (i = 0; i < resourceCount; i++)
	{
		res = WdfCmResourceListGetDescriptor(FxResourcesTranslated, i);

		if (res->Type == CmResourceTypeConnection &&
			res->u.Connection.Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
			res->u.Connection.Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C)
		{
			devContext->I2CContext.I2cResHubId.LowPart =
				res->u.Connection.IdLowPart;
			devContext->I2CContext.I2cResHubId.HighPart =
				res->u.Connection.IdHighPart;

			status = STATUS_SUCCESS;
		}
	}

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error finding CmResourceTypeConnection resource - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Initialize Spb so the driver can issue reads/writes
	//
	status = SpbTargetInitialize(FxDevice, &devContext->I2CContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error in Spb initialization - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Prepare the hardware for touch scanning
	//
	status = TchAllocateContext(&devContext->TouchContext, FxDevice);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error allocating touch context - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Fetch controller settings from registry
	//
	status = TchRegistryGetControllerSettings(
		devContext->TouchContext,
		devContext->FxDevice);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error retrieving controller settings from registry - 0x%08lX",
			status);

		goto exit;
	}

	//
	// Start the controller
	//
	status = TchStartDevice(devContext->TouchContext, &devContext->I2CContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error starting touch device - 0x%08lX",
			status);

		goto exit;
	}

	status = PoRegisterPowerSettingCallback(
		NULL,
		&GUID_ACDC_POWER_SOURCE,
		TchPowerSettingCallback,
		devContext,
		devContext->PoFxPowerSettingCallbackHandle
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error registering power setting callback - 0x%08lX",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
OnReleaseHardware(
	IN WDFDEVICE FxDevice,
	IN WDFCMRESLIST FxResourcesTranslated
)
/*++

  Routine Description:

	This routine cleans up any resources provided.

  Arguments:

	FxDevice - a handle to the framework device object
	FxResourcesRaw - list of translated hardware resources that
		the PnP manager has assigned to the device
	FxResourcesTranslated - list of raw hardware resources that
		the PnP manager has assigned to the device

  Return Value:

	NTSTATUS indicating sucesss or failure

--*/
{
	NTSTATUS status;
	PDEVICE_EXTENSION devContext;

	UNREFERENCED_PARAMETER(FxResourcesTranslated);

	devContext = GetDeviceContext(FxDevice);

	status = PoUnregisterPowerSettingCallback(
		devContext->PoFxPowerSettingCallbackHandle
	);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Error unregistering power setting callback - 0x%08lX",
			status);
	}

	status = TchStopDevice(devContext->TouchContext, &devContext->I2CContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_PNP,
			"Error stopping device - 0x%08lX",
			status);
	}

	status = TchFreeContext(devContext->TouchContext);

	if (!NT_SUCCESS(status))
	{
		Trace(
			TRACE_LEVEL_ERROR,
			TRACE_PNP,
			"Error freeing touch context - 0x%08lX",
			status);
	}

	SpbTargetDeinitialize(FxDevice, &GetDeviceContext(FxDevice)->I2CContext);

	return status;
}


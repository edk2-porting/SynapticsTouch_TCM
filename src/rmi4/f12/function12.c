// Copyright (c) Microsoft Corporation. All Rights Reserved. 
// Copyright (c) Bingxing Wang. All Rights Reserved. 
// Copyright (c) LumiaWOA authors. All Rights Reserved. 

#include <compat.h>
#include <controller.h>
#include <HidCommon.h>
#include <spb.h>
#include <rmi4\rmiinternal.h>
#include <rmi4\f12\controlregisters.h>
#include <rmi4\f12\function12.h>
#include <function12.tmh>


NTSTATUS
RmiSetReportingFlagsF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN UCHAR NewMode,
	OUT UCHAR* OldMode
)
/*++

	Routine Description:

		Changes the F12 Reporting Mode on the controller as specified

	Arguments:

		ControllerContext - Touch controller context

		SpbContext - A pointer to the current i2c context

		NewMode - Either RMI_F12_REPORTING_MODE_CONTINUOUS
				  or RMI_F12_REPORTING_MODE_REDUCED

		OldMode - Old value of reporting mode

	Return Value:

		NTSTATUS indicating success or failure

--*/
{
	RMI4_F12_FINGER_REPORT_REGISTER reportingControl = { 0 };
	int index;
	NTSTATUS status;
	UINT8 indexCtrl20;

	RtlZeroMemory(&reportingControl, sizeof(RMI4_F12_FINGER_REPORT_REGISTER));

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	indexCtrl20 = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, F12_2D_CTRL20);

	if (indexCtrl20 == ControllerContext->ControlRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Ctrl20 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->ControlRegDesc.Registers[indexCtrl20].RegisterSize != sizeof(RMI4_F12_FINGER_REPORT_REGISTER))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Ctrl20 register size");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl20,
		&reportingControl,
		sizeof(reportingControl)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Ctrl20 register - %!STATUS!",
			status);

		goto exit;
	}

	if (OldMode)
	{
		*OldMode = reportingControl.ReportingFlags;
	}

	//
	// Assign new value
	//
	reportingControl.ReportingFlags = NewMode;

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl20,
		&reportingControl,
		sizeof(RMI4_F12_FINGER_REPORT_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write F12_2D_Ctrl20 register - %X",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiGetReportingConfigurationF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	OUT PRMI4_F12_OBJECT_REPORT_ENABLE_REGISTER ControlRegisterData
)
{
	int index;
	NTSTATUS status;
	UINT8 indexCtrl23;

	NT_ASSERT(ControlRegisterData != NULL);

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	indexCtrl23 = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, F12_2D_CTRL23);

	if (indexCtrl23 == ControllerContext->ControlRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Ctrl23 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->ControlRegDesc.Registers[indexCtrl23].RegisterSize != sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Ctrl23 register size: %d", ControllerContext->ControlRegDesc.Registers[indexCtrl23].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl23,
		ControlRegisterData,
		sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Ctrl23 register - %!STATUS!",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiSetReportingConfigurationF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN PRMI4_F12_OBJECT_REPORT_ENABLE_REGISTER ControlRegisterData
)
{
	int index;
	NTSTATUS status;
	UINT8 indexCtrl23;

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	indexCtrl23 = RmiGetRegisterIndex(&ControllerContext->ControlRegDesc, F12_2D_CTRL23);

	if (indexCtrl23 == ControllerContext->ControlRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Ctrl23 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->ControlRegDesc.Registers[indexCtrl23].RegisterSize != sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Ctrl23 register size: %d", ControllerContext->ControlRegDesc.Registers[indexCtrl23].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Write setting back to the controller
	//
	status = SpbWriteDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].ControlBase + indexCtrl23,
		ControlRegisterData,
		sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not write F12_2D_Ctrl23 register - %X",
			status);

		goto exit;
	}

exit:

	return status;
}

NTSTATUS
RmiConfigureControlRegisterF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	USHORT RegisterIndex
)
{
	NTSTATUS status = STATUS_SUCCESS;

	STDebugPrint(
		TRACE_LEVEL_ERROR,
		TRACE_INIT,
		"Configuring $12 Control Register F12_2D_CTRL%d",
		RegisterIndex);

	switch (RegisterIndex)
	{
	case F12_2D_CTRL20:
		status = RmiSetReportingFlagsF12(
			ControllerContext,
			SpbContext,
			RMI4_F12_REPORTING_CONTINUOUS_MODE,
			NULL
		);
		break;
	case F12_2D_CTRL23:
		status = RmiConfigureReportingF12(
			ControllerContext,
			SpbContext
		);
		break;
	default:
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Skipped configuring $12 Control Register F12_2D_CTRL%d as the driver does not support it.",
			RegisterIndex);
	}

	return status;
}

NTSTATUS
RmiConfigureReportingF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER ControlRegisterData = { 0 };
	RtlZeroMemory(&ControlRegisterData, sizeof(RMI4_F12_OBJECT_REPORT_ENABLE_REGISTER));

	status = RmiGetReportingConfigurationF12(
		ControllerContext,
		SpbContext,
		&ControlRegisterData
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"RmiConfigureReportingF12 - %!STATUS!",
			status);

		goto exit;
	}

	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Current Controller Reporting Configuration:"
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Reported Object Count: %d",
		ControlRegisterData.ReportedObjectCount
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported Objects ==--"
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Finger: %d",
		ControlRegisterData.FingerReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ActiveStylusReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.CoverReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.EraserReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.GlovedFingerReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.HoveringFingerReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.PalmReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.SmallObjectReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.StylusReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.UnclassifiedObjectReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.HandEdgeReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.NarrowObjectReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported As Finger ==--"
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ReportActiveStylusAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.ReportCoverAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.ReportEraserAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.ReportGlovedFingerAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.ReportHoveringFingerAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.ReportPalmAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.ReportSmallObjectAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.ReportStylusAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.ReportUnclassifiedObjectAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.ReportHandEdgeAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.ReportNarrowObjectSwipeAsFinger
	);

	/*
		Default config on Lumia 950s (Retail) is:
			- Reported Object Count: 10
			- Finger reporting: ON
			- Palm reporting: ON
			- Report Gloved Finger as Finger: ON
			- Everything else: OFF

		Turn on every object type and turn off reporting as finger,
		let that be handled by us instead.
	*/

	ControlRegisterData.ReportedObjectCount = 10;

	ControlRegisterData.FingerReportingEnabled = 1;
	ControlRegisterData.ActiveStylusReportingEnabled = 1;
	ControlRegisterData.CoverReportingEnabled = 1;
	ControlRegisterData.EraserReportingEnabled = 1;
	ControlRegisterData.GlovedFingerReportingEnabled = 1;
	ControlRegisterData.HoveringFingerReportingEnabled = 1;
	ControlRegisterData.PalmReportingEnabled = 1;
	ControlRegisterData.SmallObjectReportingEnabled = 1;
	ControlRegisterData.StylusReportingEnabled = 1;
	ControlRegisterData.UnclassifiedObjectReportingEnabled = 1;
	ControlRegisterData.NarrowObjectReportingEnabled = 1;
	ControlRegisterData.HandEdgeReportingEnabled = 1;

	ControlRegisterData.ReportActiveStylusAsFinger = 0;
	ControlRegisterData.ReportCoverAsFinger = 0;
	ControlRegisterData.ReportEraserAsFinger = 0;
	ControlRegisterData.ReportGlovedFingerAsFinger = 0;
	ControlRegisterData.ReportHoveringFingerAsFinger = 0;
	ControlRegisterData.ReportPalmAsFinger = 0;
	ControlRegisterData.ReportSmallObjectAsFinger = 0;
	ControlRegisterData.ReportStylusAsFinger = 0;
	ControlRegisterData.ReportUnclassifiedObjectAsFinger = 0;
	ControlRegisterData.ReportNarrowObjectSwipeAsFinger = 0;
	ControlRegisterData.ReportHandEdgeAsFinger = 0;

	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"New Controller Reporting Configuration:"
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Reported Object Count: %d",
		ControlRegisterData.ReportedObjectCount
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported Objects ==--"
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Finger: %d",
		ControlRegisterData.FingerReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ActiveStylusReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.CoverReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.EraserReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.GlovedFingerReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.HoveringFingerReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.PalmReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.SmallObjectReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.StylusReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.UnclassifiedObjectReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.HandEdgeReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.NarrowObjectReportingEnabled
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		" "
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"--== Reported As Finger ==--"
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Active Stylus: %d",
		ControlRegisterData.ReportActiveStylusAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Cover: %d",
		ControlRegisterData.ReportCoverAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Eraser: %d",
		ControlRegisterData.ReportEraserAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Gloved Finger: %d",
		ControlRegisterData.ReportGlovedFingerAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hovering Finger: %d",
		ControlRegisterData.ReportHoveringFingerAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Palm: %d",
		ControlRegisterData.ReportPalmAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Small Object: %d",
		ControlRegisterData.ReportSmallObjectAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Stylus: %d",
		ControlRegisterData.ReportStylusAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Unclassified Object: %d",
		ControlRegisterData.ReportUnclassifiedObjectAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Hand Edge: %d",
		ControlRegisterData.ReportHandEdgeAsFinger
	);
	STDebugPrint(
		TRACE_LEVEL_INFORMATION,
		TRACE_INIT,
		"Narrow Object Swipe: %d",
		ControlRegisterData.ReportNarrowObjectSwipeAsFinger
	);

	status = RmiSetReportingConfigurationF12(
		ControllerContext,
		SpbContext,
		&ControlRegisterData
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"RmiConfigureReportingF12 - %!STATUS!",
			status);

		goto exit;
	}

	ControllerContext->MaxFingers = ControlRegisterData.ReportedObjectCount;

exit:
	return status;
}

NTSTATUS
RmiConfigureControlRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	RMI4_F12_QUERY_5_REGISTER Query5Reg = { 0 };
	RtlZeroMemory(&Query5Reg, sizeof(RMI4_F12_QUERY_5_REGISTER));

	/*
		Lumia 950s (Retail) should have
		the following control registers:

		 8*  9* 10*  15(finger amplitude threshold) 18*
		 11(jitter level) 12   20(lpwg mode) 22 23(object report enable) 24 25
		26(glove mode) 27(lpwg mode - report rate) 28(report data) 29 30 33 34 35 36 37 38 39 40
	*/

	status = RmiGetSupportedControlRegistersF12(
		ControllerContext,
		SpbContext,
		&Query5Reg);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query 5 Register - %!STATUS!",
			status);
		goto exit;
	}

	for (USHORT i = 0; i < (sizeof(Query5Reg.Data) - 1) * 8; i++)
	{
		DWORD RegisterMask = (1 << (i % 8));
		DWORD DataIndex = (i - (i % 8)) / 8 + 1;

		if ((Query5Reg.Data[DataIndex] & RegisterMask) == RegisterMask)
		{
			STDebugPrint(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Discovered $12 Control Register F12_2D_CTRL%d",
				i);

			status = RmiConfigureControlRegisterF12(
				ControllerContext,
				SpbContext,
				i
			);

			if (!NT_SUCCESS(status)) {

				STDebugPrint(
					TRACE_LEVEL_ERROR,
					TRACE_INIT,
					"Failed to configure $12 Control Register F12_2D_CTRL%d - %!STATUS!",
					i,
					status);
				goto exit;
			}
		}
	}

exit:
	return status;
}

NTSTATUS
RmiQueryDataRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	RMI4_F12_QUERY_8_REGISTER Query8Reg = { 0 };
	RtlZeroMemory(&Query8Reg, sizeof(RMI4_F12_QUERY_8_REGISTER));

	/*
		Lumia 950s (Retail) should have
		the following data registers:

		1 - ok
		2
		4  (GESTURE_REPORT_DATA)
		13
		15 (FINGER_REPORT_DATA)
	*/

	status = RmiGetSupportedDataRegistersF12(
		ControllerContext,
		SpbContext,
		&Query8Reg);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query 5 Register - %!STATUS!",
			status);
		goto exit;
	}

	for (USHORT i = 0; i < (sizeof(Query8Reg.Data) - 1) * 8; i++)
	{
		DWORD RegisterMask = (1 << (i % 8));
		DWORD DataIndex = (i - (i % 8)) / 8 + 1;

		if ((Query8Reg.Data[DataIndex] & RegisterMask) == RegisterMask)
		{
			STDebugPrint(
				TRACE_LEVEL_ERROR,
				TRACE_INIT,
				"Discovered $12 Data Register F12_2D_DATA%d",
				i);
		}
	}

exit:
	return status;
}

NTSTATUS
RmiConfigureF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext
)
{
	NTSTATUS status;
	int index;

	BYTE queryF12Addr = 0;
	char buf;
	USHORT data_offset = 0;
	PRMI_REGISTER_DESC_ITEM item;

	//
	// Find 2D touch sensor function and configure it
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	// Retrieve base address for queries
	queryF12Addr = ControllerContext->Descriptors[index].QueryBase;
	status = SpbReadDataSynchronously(
		SpbContext,
		queryF12Addr,
		&buf,
		sizeof(char)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read general info register - %!STATUS!",
			status);
		goto exit;
	}

	++queryF12Addr;

	if (!(buf & BIT(0)))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Behavior of F12 without register descriptors is undefined."
		);

		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	ControllerContext->HasDribble = !!(buf & BIT(3));

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
		&ControllerContext->QueryRegDesc
	);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Query Register Descriptor - %!STATUS!",
			status);
		goto exit;
	}

	queryF12Addr += 3;

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
		&ControllerContext->ControlRegDesc
	);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Control Register Descriptor - %!STATUS!",
			status);
		goto exit;
	}
	queryF12Addr += 3;

	status = RmiReadRegisterDescriptor(
		SpbContext,
		queryF12Addr,
		&ControllerContext->DataRegDesc
	);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to read the Data Register Descriptor - %!STATUS!",
			status);
		goto exit;
	}
	queryF12Addr += 3;
	ControllerContext->PacketSize = RmiRegisterDescriptorCalcSize(
		&ControllerContext->DataRegDesc
	);

	status = RmiConfigureControlRegistersF12(
		ControllerContext,
		SpbContext
	);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to configure $12 Control Registers - %!STATUS!",
			status);
		goto exit;
	}

	status = RmiQueryDataRegistersF12(
		ControllerContext,
		SpbContext
	);

	if (!NT_SUCCESS(status)) {

		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Failed to query $12 Data Registers - %!STATUS!",
			status);
		goto exit;
	}

	// Skip rmi_f12_read_sensor_tuning for the prototype.

	/*
	* Figure out what data is contained in the data registers. HID devices
	* may have registers defined, but their data is not reported in the
	* HID attention report. Registers which are not reported in the HID
	* attention report check to see if the device is receiving data from
	* HID attention reports.
	*/
	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 0);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 1);
	if (item != NULL)
	{
		ControllerContext->Data1Offset = data_offset;
		ControllerContext->Data1Size = (USHORT)item->RegisterSize;

		data_offset += (USHORT)item->RegisterSize;
	}

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 2);
	if (item != NULL)
	{
		ControllerContext->Data2Offset = data_offset;
		ControllerContext->Data2Size = (USHORT)item->RegisterSize;

		data_offset += (USHORT)item->RegisterSize;
	}

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 3);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 4);
	if (item != NULL)
	{
		ControllerContext->Data4Offset = data_offset;
		ControllerContext->Data4Size = (USHORT)item->RegisterSize;

		data_offset += (USHORT)item->RegisterSize;
	}

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 5);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 6);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 7);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 8);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 9);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 10);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 11);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 12);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 13);
	if (item != NULL)
	{
		ControllerContext->Data13Offset = data_offset;
		ControllerContext->Data13Size = (USHORT)item->RegisterSize;

		data_offset += (USHORT)item->RegisterSize;
	}

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 14);
	if (item) data_offset += (USHORT)item->RegisterSize;

	item = RmiGetRegisterDescItem(&ControllerContext->DataRegDesc, 15);
	if (item != NULL)
	{
		ControllerContext->Data15Offset = data_offset;
		ControllerContext->Data15Size   = (USHORT)item->RegisterSize;

		data_offset += (USHORT)item->RegisterSize;
	}

exit:
	return status;
}

NTSTATUS
RmiGetObjectsFromControllerF12(
	IN VOID* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	IN RMI4_DETECTED_OBJECTS* Data
)
/*++

Routine Description:

	This routine reads raw touch messages from hardware. If there is
	no touch data available (if a non-touch interrupt fired), the
	function will not return success and no touch data was transferred.

Arguments:

	ControllerContext - Touch controller context
	SpbContext - A pointer to the current i2c context
	Data - A pointer to any returned F11 touch data

Return Value:

	NTSTATUS, where only success indicates data was returned

--*/
{
	NTSTATUS status;
	RMI4_CONTROLLER_CONTEXT* controller;

	int index, i, x, y;

	PRMI4_F12_FINGER_DATA_REGISTER controllerData = NULL;

	controller = (RMI4_CONTROLLER_CONTEXT*)ControllerContext;

	//
	// Locate RMI data base address of 2D touch function
	//
	index = RmiGetFunctionIndex(
		controller->Descriptors,
		controller->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == controller->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		controller->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	controllerData = ExAllocatePoolWithTag(
		NonPagedPoolNx,
		(ULONG)(controller->MaxFingers * sizeof(RMI4_F12_FINGER_DATA_REGISTER)),
		TOUCH_POOL_TAG_F12
	);

	if (controllerData == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto exit;
	}

	// 
	// Packets we need is determined by context
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		controller->Descriptors[index].DataBase + (BYTE)controller->Data1Offset,
		controllerData,
		(ULONG)(controller->MaxFingers * sizeof(RMI4_F12_FINGER_DATA_REGISTER))
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INTERRUPT,
			"Error reading finger status data - %!STATUS!",
			status);

		goto free_buffer;
	}

	for (i = 0; i < controller->MaxFingers; i++)
	{
		switch (controllerData[i].ObjectTypeAndStatus)
		{
		case (BYTE)RMI4_F12_OBJECT_FINGER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is finger",
				i);

			break;
		case (BYTE)RMI4_F12_OBJECT_HOVERING_FINGER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is hovering finger",
				i);

			break;
		case (BYTE)RMI4_F12_OBJECT_GLOVED_FINGER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is gloved finger",
				i);

			break;
		case (BYTE)RMI4_F12_OBJECT_ACTIVE_STYLUS:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_PRESENT_WITH_TIP;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is active stylus",
				i);

			break;
		case (BYTE)RMI4_F12_OBJECT_STYLUS:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_PRESENT_WITH_TIP;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is stylus",
				i);

			break;
		case (BYTE)RMI4_F12_OBJECT_ERASER:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_PRESENT_WITH_ERASER;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is eraser",
				i);

			break;
		case (BYTE)RMI4_F12_OBJECT_NONE:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;

			STDebugPrint(
				TRACE_LEVEL_VERBOSE,
				TRACE_INTERRUPT,
				"Finger[%d] status is none",
				i);

			break;
		default:
			Data->FingerStates[i] = RMI4_FINGER_STATE_NOT_PRESENT;
			Data->PenStates[i] = RMI4_PEN_STATE_NOT_PRESENT;

			STDebugPrint(
				TRACE_LEVEL_INFORMATION,
				TRACE_INTERRUPT,
				"Finger[%d] status is unknown: %d",
				i,
				controllerData[i].ObjectTypeAndStatus);

			break;
		}

		x = (controllerData[i].X_MSB << 8) | controllerData[i].X_LSB;
		y = (controllerData[i].Y_MSB << 8) | controllerData[i].Y_LSB;

		Data->Positions[i].X = x;
		Data->Positions[i].Y = y;

		if (controllerData[i].ObjectTypeAndStatus != (BYTE)RMI4_F12_OBJECT_NONE)
		{
			STDebugPrint(
				TRACE_LEVEL_VERBOSE,
				TRACE_INTERRUPT,
				"Finger[%d] X: %d, Y: %d, Z: %d, wX: %d, wY: %d",
				i,
				x,
				y,
				controllerData[i].Z,
				controllerData[i].wX,
				controllerData[i].wY);
		}
	}

free_buffer:
	ExFreePoolWithTag(
		controllerData,
		TOUCH_POOL_TAG_F12
	);

exit:
	return status;
}

NTSTATUS
RmiGetSupportedControlRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	OUT PRMI4_F12_QUERY_5_REGISTER ControlRegisterData
)
{
	int index;
	NTSTATUS status;
	UINT8 indexQuery4;
	UINT8 indexQuery5;

	NT_ASSERT(ControlRegisterData != NULL);

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	indexQuery4 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 4);

	if (indexQuery4 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query4 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery4].RegisterSize != sizeof(ControlRegisterData->Size))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query4 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery4].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery4,
		&(ControlRegisterData->Size),
		sizeof(ControlRegisterData->Size)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query4 register - %!STATUS!",
			status);

		goto exit;
	}

	if (ControlRegisterData->Size > sizeof(ControlRegisterData->Data))
		ControlRegisterData->Size = sizeof(ControlRegisterData->Data);

	indexQuery5 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 5);

	if (indexQuery5 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query5 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize < ControlRegisterData->Size)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query5 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	else if (ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize > ControlRegisterData->Size)
	{
		STDebugPrint(
			TRACE_LEVEL_WARNING,
			TRACE_INIT,
			"Higher F12_2D_Query5 register size: %d. Must expand driver!", ControllerContext->QueryRegDesc.Registers[indexQuery5].RegisterSize);
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery5,
		&(ControlRegisterData->Data),
		ControlRegisterData->Size
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query5 register - %!STATUS!",
			status);

		goto exit;
	}

exit:
	return status;
}

NTSTATUS
RmiGetSupportedDataRegistersF12(
	IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
	IN SPB_CONTEXT* SpbContext,
	OUT PRMI4_F12_QUERY_8_REGISTER ControlRegisterData
)
{
	int index;
	NTSTATUS status;
	UINT8 indexQuery7;
	UINT8 indexQuery8;

	NT_ASSERT(ControlRegisterData != NULL);

	//
	// Find RMI F12 function
	//
	index = RmiGetFunctionIndex(
		ControllerContext->Descriptors,
		ControllerContext->FunctionCount,
		RMI4_F12_2D_TOUCHPAD_SENSOR);

	if (index == ControllerContext->FunctionCount)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Set ReportingMode failure - RMI Function 12 missing");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	status = RmiChangePage(
		ControllerContext,
		SpbContext,
		ControllerContext->FunctionOnPage[index]);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not change register page");

		goto exit;
	}

	indexQuery7 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 7);

	if (indexQuery7 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query7 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery7].RegisterSize != sizeof(ControlRegisterData->Size))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query7 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery7].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery7,
		&(ControlRegisterData->Size),
		sizeof(ControlRegisterData->Size)
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query7 register - %!STATUS!",
			status);

		goto exit;
	}

	if (ControlRegisterData->Size > sizeof(ControlRegisterData->Data))
		ControlRegisterData->Size = sizeof(ControlRegisterData->Data);

	indexQuery8 = RmiGetRegisterIndex(&ControllerContext->QueryRegDesc, 8);

	if (indexQuery8 == ControllerContext->QueryRegDesc.NumRegisters)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Cannot find F12_2D_Query8 offset");

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}

	if (ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize < ControlRegisterData->Size)
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Unexpected F12_2D_Query8 register size: %d", ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize);

		status = STATUS_INVALID_DEVICE_STATE;
		goto exit;
	}
	else if (ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize > ControlRegisterData->Size)
	{
		STDebugPrint(
			TRACE_LEVEL_WARNING,
			TRACE_INIT,
			"Higher F12_2D_Query8 register size: %d. Must expand driver!", ControllerContext->QueryRegDesc.Registers[indexQuery8].RegisterSize);
	}

	//
	// Read Device Control register
	//
	status = SpbReadDataSynchronously(
		SpbContext,
		ControllerContext->Descriptors[index].QueryBase + indexQuery8,
		&(ControlRegisterData->Data),
		ControlRegisterData->Size
	);

	if (!NT_SUCCESS(status))
	{
		STDebugPrint(
			TRACE_LEVEL_ERROR,
			TRACE_INIT,
			"Could not read F12_2D_Query8 register - %!STATUS!",
			status);

		goto exit;
	}

exit:
	return status;
}
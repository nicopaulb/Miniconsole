/**
 ******************************************************************************
 * @file    App/p2p_client_app.c
 * @author  MCD Application Team
 * @brief   Peer to peer Client Application
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/

#include <hid_host_app.h>
#include "main.h"
#include "app_common.h"

#include "dbg_trace.h"

#include "ble.h"
#include "stm32_seq.h"
#include "app_ble.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct {
	/**
	 * state of the P2P Client
	 * state machine
	 */
	HID_HOST_Status_t state;
	uint16_t connHandle;
	// HID Service
	uint16_t HIDServiceHandle;
	uint16_t HIDServiceEndHandle;
	uint16_t HIDReadInformationCharHandle;
	uint16_t HIDReport1CharHandle;
	uint16_t HIDReport2CharHandle;
	uint16_t HIDReportReferenceDescHandle;
	uint16_t HIDReportReferenceDesc2Handle;
	uint16_t HIDClientCharDescHandle;
	// Battery Service
	uint16_t BatteryServiceHandle;
	uint16_t BatteryServiceEndHandle;
	uint16_t BatteryLevelCharHandle;
	uint16_t BatteryLevelCharDescHandle;
} HID_ClientContext_t;

/* Private defines ------------------------------------------------------------*/

/* Private macros -------------------------------------------------------------*/
#define UNPACK_2_BYTE_PARAMETER(ptr)  \
        (uint16_t)((uint16_t)(*((uint8_t *)ptr))) |   \
        (uint16_t)((((uint16_t)(*((uint8_t *)ptr + 1))) << 8))

/* Private variables ---------------------------------------------------------*/
static HID_ClientContext_t HIDHostContext;
static HID_Report_t HIDReport;
static uint8_t BatteryLevel = 0;

/* Private function prototypes -----------------------------------------------*/
static void HID_Report_Notification(uint8_t *payload, size_t length);
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event);
static void Update_Discovery();

/* Functions Definition ------------------------------------------------------*/
/**
 * @brief  Service initialization
 * @param  None
 * @retval None
 */
void HID_Host_Init(void) {
	memset(&HIDHostContext, 0, sizeof(HID_ClientContext_t));
	UTIL_SEQ_RegTask(1 << CFG_TASK_SEARCH_SERVICE_ID, UTIL_SEQ_RFU, Update_Discovery);

	/**
	 *  Register the event handler to the BLE controller
	 */
	SVCCTL_RegisterCltHandler(Event_Handler);
	APP_DBG_MSG("-- HID HOST INITIALIZED \n")
	return;
}

void HID_Host_Notification(HID_APP_ConnHandle_Not_evt_t *pNotification) {
	switch (pNotification->HID_Evt_Opcode) {
	case PEER_CONN_HANDLE_EVT:
		HIDHostContext.state = HID_HOST_CONNECTED;
		break;

	case PEER_PAIR_HANDLE_EVT:
		HIDHostContext.state = HID_HOST_DISCOVER_SERVICES;
		HIDHostContext.connHandle = pNotification->ConnectionHandle;
		UTIL_SEQ_SetTask(1 << CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0); // TODO maybe remove
		break;

	case PEER_DISCON_HANDLE_EVT:
		memset(&HIDHostContext, 0, sizeof(HID_ClientContext_t));
		break;

	default:
		break;
	}
	return;
}

HID_HOST_Status_t HID_Host_Get_State(void) {
	return HIDHostContext.state;
}

HID_Report_t* HID_Host_Get_Report(void) {
	return &HIDReport;
}

uint8_t HID_Host_Get_Battery_Level(void) {
	return BatteryLevel;
}

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/

/**
 * @brief  Event handler
 * @param  Event: Address of the buffer holding the Event
 * @retval Ack: Return whether the Event has been managed or not
 */
static SVCCTL_EvtAckStatus_t Event_Handler(void *Event) {
	SVCCTL_EvtAckStatus_t return_value;
	hci_event_pckt *event_pckt;
	evt_blecore_aci *blecore_evt;

	return_value = SVCCTL_EvtNotAck;
	event_pckt = (hci_event_pckt*) (((hci_uart_pckt*) Event)->data);

	switch (event_pckt->evt) {
	case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE: {
		blecore_evt = (evt_blecore_aci*) event_pckt->data;
		switch (blecore_evt->ecode) {

		case ACI_ATT_READ_BY_GROUP_TYPE_RESP_VSEVT_CODE: {
			aci_att_read_by_group_type_resp_event_rp0 *pr = (void*) blecore_evt->data;
			uint8_t numberService, i, idx;
			uint16_t uuid, handle;

			handle = pr->Connection_Handle;
			if (HIDHostContext.state != HID_HOST_IDLE) {
				APP_BLE_ConnStatus_t status;

				status = APP_BLE_Get_Client_Connection_Status(HIDHostContext.connHandle);

				if ((HIDHostContext.state == HID_HOST_DONE) && (status == APP_BLE_IDLE)) {
					HIDHostContext.state = HID_HOST_IDLE;
					HIDHostContext.connHandle = 0xFFFF;
					break;
				}
			}

			HIDHostContext.connHandle = handle;
			numberService = (pr->Data_Length) / pr->Attribute_Data_Length;
			/* the event data will be
			 * 2bytes start handle
			 * 2bytes end handle
			 * 2 or 16 bytes data
			 * we are interested only if the UUID is 2 bit.
			 * So check if the data length is 6
			 */
			idx = 0;
			if (pr->Attribute_Data_Length == 6) {
				for (i = 0; i < numberService; i++) {
					uuid = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx + 4]);
					switch (uuid) {
					case HUMAN_INTERFACE_DEVICE_SERVICE_UUID:
						APP_DBG_MSG("-- GATT : HID SERVICE FOUND - connection handle 0x%x\n", handle)
						HIDHostContext.HIDServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx]);
						HIDHostContext.HIDServiceEndHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx + 2]);
						HIDHostContext.state = HID_HOST_DISCOVER_HID_CHARACS;
						break;
					case BATTERY_SERVICE_UUID:
						APP_DBG_MSG("-- GATT : BATTERY SERVICE FOUND - connection handle 0x%x\n", handle)
						HIDHostContext.BatteryServiceHandle = UNPACK_2_BYTE_PARAMETER(&pr->Attribute_Data_List[idx]);
						HIDHostContext.BatteryServiceEndHandle = UNPACK_2_BYTE_PARAMETER(
								&pr->Attribute_Data_List[idx + 2]);
						break;
					}
					idx += 6;
				}
			}
		}
			break;

		case ACI_ATT_READ_BY_TYPE_RESP_VSEVT_CODE: {

			aci_att_read_by_type_resp_event_rp0 *pr = (void*) blecore_evt->data;
			uint8_t idx;
			uint16_t uuid, handle;

			/* the event data will be
			 * 2 bytes start handle
			 * 1 byte char properties
			 * 2 bytes handle
			 * 2 or 16 bytes data
			 */

			if (HIDHostContext.connHandle == pr->Connection_Handle) {
				/* we are interested in only 2 bit UUIDs */
				idx = 0;
				if (pr->Handle_Value_Pair_Length == 7) {
					pr->Data_Length -= 1;
					while (pr->Data_Length > 0) {
						uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx + 5]);
						handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_Value_Pair_Data[idx + 3]);
						switch (uuid) {
						case HID_INFORMATION_CHAR_UUID:
							APP_DBG_MSG("-- GATT : HID_INFORMATION_CHAR_UUID FOUND - connection handle 0x%x\n", handle)
							HIDHostContext.HIDReadInformationCharHandle = handle;
							break;
						case REPORT_CHAR_UUID:
							APP_DBG_MSG("-- GATT : REPORT_CHAR_UUID FOUND  - connection handle 0x%x\n", handle)
							HIDHostContext.state = HID_HOST_DISCOVER_REPORT_1_DESC;
							if (HIDHostContext.HIDReport1CharHandle == 0) {
								HIDHostContext.HIDReport1CharHandle = handle;
							} else {
								HIDHostContext.HIDReport2CharHandle = handle;
							}
							break;
						case REPORT_MAP_CHAR_UUID:
							APP_DBG_MSG("-- GATT : REPORT_MAP_CHAR_UUID FOUND - connection handle 0x%x\n", handle)
							break;
						case HID_CONTROL_POINT_CHAR_UUID:
							APP_DBG_MSG("-- GATT : HID_CONTROL_POINT_CHAR_UUID FOUND - connection handle 0x%x\n", handle)
							break;
						case BATTERY_LEVEL_CHAR_UUID:
							APP_DBG_MSG("-- GATT : BATTERY_LEVEL_CHAR_UUID FOUND - connection handle 0x%x\n", handle)
							HIDHostContext.BatteryLevelCharHandle = handle;
							HIDHostContext.state = HID_HOST_DISCOVER_LEVEL_DESC;
							break;

						default:
							break;
						}
						pr->Data_Length -= 7;
						idx += 7;
					}
				}
			}
		}
			break;

		case ACI_ATT_FIND_INFO_RESP_VSEVT_CODE: {
			aci_att_find_info_resp_event_rp0 *pr = (void*) blecore_evt->data;

			uint8_t numDesc, idx, i;
			uint16_t uuid, handle;

			/*
			 * event data will be of the format
			 * 2 bytes handle
			 * 2 bytes UUID
			 */
			if (HIDHostContext.connHandle == pr->Connection_Handle) {
				numDesc = (pr->Event_Data_Length) / 4;
				/* we are interested only in 16 bit UUIDs */
				idx = 0;
				if (pr->Format == UUID_TYPE_16) {
					for (i = 0; i < numDesc; i++) {
						handle = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx]);
						uuid = UNPACK_2_BYTE_PARAMETER(&pr->Handle_UUID_Pair[idx + 2]);

						if (uuid == CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID) {
							APP_DBG_MSG("-- GATT : CLIENT_CHAR_CONFIG_DESCRIPTOR_UUID- connection handle 0x%x\n", handle)
							if (HIDHostContext.state == HID_HOST_DISCOVERING_REPORT_1_DESC) {
								HIDHostContext.HIDClientCharDescHandle = handle;
							} else if (HIDHostContext.state == HID_HOST_DISCOVERING_LEVEL_DESC) {
								HIDHostContext.BatteryLevelCharDescHandle = handle;
								HIDHostContext.state = HID_HOST_ENABLE_ALL_NOTIFICATION_DESC;
							}
						} else if (uuid == REPORT_REFERENCE_DESCRIPTOR_UUID) {
							APP_DBG_MSG("-- GATT : REPORT_REFERENCE_DESCRIPTOR_UUID- connection handle 0x%x\n", handle)
							if (HIDHostContext.state == HID_HOST_DISCOVERING_REPORT_1_DESC) {
								HIDHostContext.HIDReportReferenceDescHandle = handle;
								HIDHostContext.state = HID_HOST_DISCOVER_REPORT_2_DESC;
							} else if (HIDHostContext.state == HID_HOST_DISCOVERING_REPORT_2_DESC) {
								HIDHostContext.HIDReportReferenceDesc2Handle = handle;
								HIDHostContext.state = HID_HOST_DISCOVER_LEVEL_CHARACS;
							}
						}
						idx += 4;
					}
				}
			}
		}
			break; /*ACI_ATT_FIND_INFO_RESP_VSEVT_CODE*/

		case ACI_GATT_NOTIFICATION_VSEVT_CODE: {
			aci_gatt_notification_event_rp0 *pr = (void*) blecore_evt->data;
			if (HIDHostContext.connHandle == pr->Connection_Handle) {
				if ((pr->Attribute_Handle == HIDHostContext.HIDReport1CharHandle)) {
					APP_DBG_MSG("-- GATT : ACI_GATT_NOTIFICATION_VSEVT_CODE for HID Report\n")
					HID_Report_Notification(&pr->Attribute_Value[0], pr->Attribute_Value_Length);
				} else if ((pr->Attribute_Handle == HIDHostContext.BatteryLevelCharHandle)) {
					APP_DBG_MSG("-- GATT : ACI_GATT_NOTIFICATION_VSEVT_CODE for Battery Level : %d\n", pr->Attribute_Value[0])
					BatteryLevel = pr->Attribute_Value[0];
				}
			}
		}
			break;/* end ACI_GATT_NOTIFICATION_VSEVT_CODE */

		case ACI_GATT_PROC_COMPLETE_VSEVT_CODE: {
			aci_gatt_proc_complete_event_rp0 *pr = (void*) blecore_evt->data;
			APP_DBG_MSG("-- GATT : ACI_GATT_PROC_COMPLETE_VSEVT_CODE \n")
			APP_DBG_MSG("\n")

			if (HIDHostContext.connHandle == pr->Connection_Handle) {
				UTIL_SEQ_SetTask(1 << CFG_TASK_SEARCH_SERVICE_ID, CFG_SCH_PRIO_0);
			}
		}
			break; /*ACI_GATT_PROC_COMPLETE_VSEVT_CODE*/
		default:
			break;
		}
	}

		break; /* HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE */

	default:
		break;
	}

	return (return_value);
}/* end BLE_CTRL_Event_Acknowledged_Status_t */

static void HID_Report_Notification(uint8_t *payload, size_t length) {
	if (length == 16) {
		char string[33] = { 0 };
		for (int i = 0; i < length; i++) {
			sprintf(&string[i * 2], "%02X", payload[i]);
		}
		APP_DBG_MSG(string)
		memcpy(&HIDReport, payload, length);
	}

	return;
}

static void Update_Discovery() {
	uint16_t enable = 0x0001;

	switch (HIDHostContext.state) {
	case HID_HOST_DISCOVER_SERVICES:
		APP_DBG_MSG("* GATT :  Start Searching Primary Services \r\n\r")
		HIDHostContext.state = HID_HOST_DISCOVERING_SERVICES;
		aci_gatt_disc_all_primary_services(HIDHostContext.connHandle);
		break;
	case HID_HOST_DISCOVER_HID_CHARACS:
		APP_DBG_MSG("* GATT : Discover HID Characteristics\n")
		HIDHostContext.state = HID_HOST_DISCOVERING_HID_CHARACS;
		aci_gatt_disc_all_char_of_service(HIDHostContext.connHandle, HIDHostContext.HIDServiceHandle,
				HIDHostContext.HIDServiceEndHandle);
		break;
	case HID_HOST_DISCOVER_REPORT_1_DESC:
		APP_DBG_MSG("* GATT : Discover Descriptor of Report 1 Characteristic\n")
		HIDHostContext.state = HID_HOST_DISCOVERING_REPORT_1_DESC;
		aci_gatt_disc_all_char_desc(HIDHostContext.connHandle, HIDHostContext.HIDReport1CharHandle,
				HIDHostContext.HIDReport1CharHandle + 2);
		break;
	case HID_HOST_DISCOVER_REPORT_2_DESC:
		APP_DBG_MSG("* GATT : Discover Descriptor of Report 2 Characteristic\n")
		HIDHostContext.state = HID_HOST_DISCOVERING_REPORT_2_DESC;
		aci_gatt_disc_all_char_desc(HIDHostContext.connHandle, HIDHostContext.HIDReport2CharHandle,
				HIDHostContext.HIDReport2CharHandle + 2);
		break;
	case HID_HOST_DISCOVER_LEVEL_CHARACS:
		APP_DBG_MSG("* GATT : Discover Battery Characteristics\n")
		HIDHostContext.state = HID_HOST_DISCOVERING_BAT_CHARACS;
		aci_gatt_disc_all_char_of_service(HIDHostContext.connHandle, HIDHostContext.BatteryServiceHandle,
				HIDHostContext.BatteryServiceEndHandle);
		break;
	case HID_HOST_DISCOVER_LEVEL_DESC:
		APP_DBG_MSG("* GATT : Discover Descriptor of Battery Level Characteristic\n")
		HIDHostContext.state = HID_HOST_DISCOVERING_LEVEL_DESC;
		aci_gatt_disc_all_char_desc(HIDHostContext.connHandle, HIDHostContext.BatteryLevelCharHandle,
				HIDHostContext.BatteryLevelCharHandle + 2);
		break;
	case HID_HOST_ENABLE_ALL_NOTIFICATION_DESC:
		APP_DBG_MSG("* GATT : Enable Battery Level Notification\n")
		aci_gatt_write_char_desc(HIDHostContext.connHandle, HIDHostContext.BatteryLevelCharDescHandle, 2,
				(uint8_t*) &enable);
		APP_DBG_MSG("* GATT : Enable Report Notification\n")
		aci_gatt_write_char_desc(HIDHostContext.connHandle, HIDHostContext.HIDClientCharDescHandle, 2,
				(uint8_t*) &enable);
		HIDHostContext.state = HID_HOST_DONE;
	case HID_HOST_DONE:
		break;

	default:
		break;
	}

	return;
}

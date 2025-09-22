#include "main.h"

#include "app_common.h"

#include "dbg_trace.h"

#include "ble.h"
#include "tl.h"
#include "app_ble.h"

#include "stm32_seq.h"
#include "shci.h"
#include "stm32_lpm.h"
#include "otp.h"

#include "hid_host_app.h"

/**
 * security parameters structure
 */
typedef struct _tSecurityParams {
	/**
	 * IO capability of the device
	 */
	uint8_t ioCapability;

	/**
	 * Authentication requirement of the device
	 * Man In the Middle protection required?
	 */
	uint8_t mitm_mode;

	/**
	 * bonding mode of the device
	 */
	uint8_t bonding_mode;

	/**
	 * this variable indicates whether to use a fixed pin
	 * during the pairing process or a passkey has to be
	 * requested to the application during the pairing process
	 * 0 implies use fixed pin and 1 implies request for passkey
	 */
	uint8_t Use_Fixed_Pin;

	/**
	 * minimum encryption key size requirement
	 */
	uint8_t encryptionKeySizeMin;

	/**
	 * maximum encryption key size requirement
	 */
	uint8_t encryptionKeySizeMax;

	/**
	 * fixed pin to be used in the pairing process if
	 * Use_Fixed_Pin is set to 1
	 */
	uint32_t Fixed_Pin;

	/**
	 * this flag indicates whether the host has to initiate
	 * the security, wait for pairing or does not have any security
	 * requirements.
	 * 0x00 : no security required
	 * 0x01 : host should initiate security by sending the slave security
	 *        request command
	 * 0x02 : host need not send the clave security request but it
	 * has to wait for paiirng to complete before doing any other
	 * processing
	 */
	uint8_t initiateSecurity;
} tSecurityParams;

/**
 * global context
 * contains the variables common to all
 * services
 */
typedef struct _tBLEProfileGlobalContext {
	/**
	 * security requirements of the host
	 */
	tSecurityParams bleSecurityParam;

	/**
	 * gap service handle
	 */
	uint16_t gapServiceHandle;

	/**
	 * device name characteristic handle
	 */
	uint16_t devNameCharHandle;

	/**
	 * appearance characteristic handle
	 */
	uint16_t appearanceCharHandle;

	/**
	 * connection handle of the current active connection
	 * When not in connection, the handle is set to 0xFFFF
	 */
	uint16_t connectionHandle;

	/**
	 * length of the UUID list to be used while advertising
	 */
	uint8_t advtServUUIDlen;

	/**
	 * the UUID list to be used while advertising
	 */
	uint8_t advtServUUID[100];
} BleGlobalContext_t;

typedef struct {
	BleGlobalContext_t BleApplicationContext_legacy;
	APP_BLE_ConnStatus_t Device_Connection_Status;
	uint8_t SwitchOffGPIO_timer_Id;
	uint8_t DeviceServerFound;
} BleApplicationContext_t;

#define APPBLE_GAP_DEVICE_NAME_LENGTH 8
#define BD_ADDR_SIZE_LOCAL    6

PLACE_IN_SECTION("MB_MEM1") ALIGN(4) static TL_CmdPacket_t BleCmdBuffer;

static const uint8_t a_MBdAddr[BD_ADDR_SIZE_LOCAL] =
		{ (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x0000000000FF)), (uint8_t) ((CFG_ADV_BD_ADDRESS & 0x00000000FF00) >> 8),
				(uint8_t) ((CFG_ADV_BD_ADDRESS & 0x000000FF0000) >> 16),
				(uint8_t) ((CFG_ADV_BD_ADDRESS & 0x0000FF000000) >> 24),
				(uint8_t) ((CFG_ADV_BD_ADDRESS & 0x00FF00000000) >> 32),
				(uint8_t) ((CFG_ADV_BD_ADDRESS & 0xFF0000000000) >> 40) };

static uint8_t a_BdAddrUdn[BD_ADDR_SIZE_LOCAL];
/**
 *   Identity root key used to derive IRK and DHK(Legacy)
 */
static const uint8_t a_BLE_CfgIrValue[16] = CFG_BLE_IR;

/**
 * Encryption root key used to derive LTK(Legacy) and CSRK
 */
static const uint8_t a_BLE_CfgErValue[16] = CFG_BLE_ER;

static const uint8_t completeLocalName[24] = { 0x58, 0x62, 0x6F, 0x78, 0x20, 0x57, 0x69, 0x72, 0x65, 0x6C, 0x65, 0x73,
		0x73, 0x20, 0x43, 0x6F, 0x6E, 0x74, 0x72, 0x6F, 0x6C, 0x6C, 0x65, 0x72 };

tBDAddr SERVER_REMOTE_BDADDR;
uint8_t SERVER_REMOTE_ADDR_TYPE;

HID_APP_ConnHandle_Not_evt_t handleNotification;
static BleApplicationContext_t BleApplicationContext;

static void BLE_UserEvtRx(void *pPayload);
static void BLE_StatusNot(HCI_TL_CmdStatus_t status);
static void Ble_Tl_Init(void);
static void Ble_Hci_Gap_Gatt_Init(void);
static const uint8_t* BleGetBdAddress(void);
static void Scan_Request(void);
static void Connect_Request(void);
static void Pairing_Request(void);

void APP_BLE_Init(void) {
	SHCI_CmdStatus_t status;
	SHCI_C2_Ble_Init_Cmd_Packet_t ble_init_cmd_packet = { { { 0, 0, 0 } }, /**< Header unused */
	{ 0, /** pBleBufferAddress not used */
	0, /** BleBufferSize not used */
	CFG_BLE_NUM_GATT_ATTRIBUTES,
	CFG_BLE_NUM_GATT_SERVICES,
	CFG_BLE_ATT_VALUE_ARRAY_SIZE,
	CFG_BLE_NUM_LINK,
	CFG_BLE_DATA_LENGTH_EXTENSION,
	CFG_BLE_PREPARE_WRITE_LIST_SIZE,
	CFG_BLE_MBLOCK_COUNT,
	CFG_BLE_MAX_ATT_MTU,
	CFG_BLE_PERIPHERAL_SCA,
	CFG_BLE_CENTRAL_SCA,
	CFG_BLE_LS_SOURCE,
	CFG_BLE_MAX_CONN_EVENT_LENGTH,
	CFG_BLE_HSE_STARTUP_TIME,
	CFG_BLE_VITERBI_MODE,
	CFG_BLE_OPTIONS, 0,
	CFG_BLE_MAX_COC_INITIATOR_NBR,
	CFG_BLE_MIN_TX_POWER,
	CFG_BLE_MAX_TX_POWER,
	CFG_BLE_RX_MODEL_CONFIG,
	CFG_BLE_MAX_ADV_SET_NBR,
	CFG_BLE_MAX_ADV_DATA_LEN,
	CFG_BLE_TX_PATH_COMPENS,
	CFG_BLE_RX_PATH_COMPENS,
	CFG_BLE_CORE_VERSION,
	CFG_BLE_OPTIONS_EXT } };

	/**
	 * Initialize Ble Transport Layer
	 */
	Ble_Tl_Init();

	/**
	 * Do not allow standby in the application
	 */
	UTIL_LPM_SetOffMode(1 << CFG_LPM_APP_BLE, UTIL_LPM_DISABLE);

	/**
	 * Register the hci transport layer to handle BLE User Asynchronous Events
	 */
	UTIL_SEQ_RegTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, UTIL_SEQ_RFU, hci_user_evt_proc);

	/**
	 * Starts the BLE Stack on CPU2
	 */
	status = SHCI_C2_BLE_Init(&ble_init_cmd_packet);
	if (status != SHCI_Success) {
		APP_DBG_MSG("  Fail   : SHCI_C2_BLE_Init command, result: 0x%02x\n\r", status)
				/* if you are here, maybe CPU2 doesn't contain STM32WB_Copro_Wireless_Binaries, see Release_Notes.html */
		Error_Handler();
	} else {
		APP_DBG_MSG("  Success: SHCI_C2_BLE_Init command\n\r")
	}

	/**
	 * Initialization of HCI & GATT & GAP layer
	 */
	Ble_Hci_Gap_Gatt_Init();

	/**
	 * Initialization of the BLE Services
	 */
	SVCCTL_Init();

	HID_Host_Init();

	/**
	 * From here, all initialization are BLE application specific
	 */
	UTIL_SEQ_RegTask(1 << CFG_TASK_START_SCAN_ID, UTIL_SEQ_RFU, Scan_Request);
	UTIL_SEQ_RegTask(1 << CFG_TASK_CONN_DEV_ID, UTIL_SEQ_RFU, Connect_Request);
	UTIL_SEQ_RegTask(1 << CFG_TASK_PAIR_DEV_ID, UTIL_SEQ_RFU, Pairing_Request);

	/**
	 * Initialization of the BLE App Context
	 */
	BleApplicationContext.Device_Connection_Status = APP_BLE_IDLE;

	/**
	 * Start scanning
	 */
	UTIL_SEQ_SetTask(1 << CFG_TASK_START_SCAN_ID, CFG_SCH_PRIO_0);
	return;
}

SVCCTL_UserEvtFlowStatus_t SVCCTL_App_Notification(void *pckt) {
	hci_event_pckt *event_pckt;
	evt_le_meta_event *meta_evt;
	hci_le_connection_complete_event_rp0 *connection_complete_event;
	aci_gap_pairing_complete_event_rp0 *p_pairing_complete;
	evt_blecore_aci *blecore_evt;
	hci_le_advertising_report_event_rp0 *le_advertising_event;
	event_pckt = (hci_event_pckt*) ((hci_uart_pckt*) pckt)->data;
	hci_disconnection_complete_event_rp0 *cc = (void*) event_pckt->data;
	uint8_t event_type, event_data_size;
	int k;
	uint8_t adtype, adlength;

	switch (event_pckt->evt) {
	case HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE: {
		blecore_evt = (evt_blecore_aci*) event_pckt->data;
		switch (blecore_evt->ecode) {

		case ACI_GAP_PROC_COMPLETE_VSEVT_CODE: {
			aci_gap_proc_complete_event_rp0 *gap_evt_proc_complete = (void*) blecore_evt->data;
			/* CHECK GAP GENERAL DISCOVERY PROCEDURE COMPLETED & SUCCEED */
			if (gap_evt_proc_complete->Procedure_Code == GAP_GENERAL_DISCOVERY_PROC
					&& gap_evt_proc_complete->Status == 0x00) {
				APP_DBG_MSG("-- GAP GENERAL DISCOVERY PROCEDURE_COMPLETED\n\r")

				if (BleApplicationContext.DeviceServerFound == 0x01
						&& BleApplicationContext.Device_Connection_Status != APP_BLE_CONNECTED) {
					// if a device found, connect to it
					UTIL_SEQ_SetTask(1 << CFG_TASK_CONN_DEV_ID, CFG_SCH_PRIO_0);
				} else {
					// else restart new scan
					UTIL_SEQ_SetTask(1 << CFG_TASK_START_SCAN_ID, CFG_SCH_PRIO_0);
				}
			}
		}
			break;

		case ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE: {
			p_pairing_complete = (aci_gap_pairing_complete_event_rp0*) event_pckt->data;

			APP_DBG_MSG(">>== ACI_GAP_PAIRING_COMPLETE_VSEVT_CODE\n")
			if (p_pairing_complete->Status != 0) {
				APP_DBG_MSG("     - Pairing KO \n     - Status: 0x%x\n     - Reason: 0x%x\n", p_pairing_complete->Status, p_pairing_complete->Reason)
			} else {
				APP_DBG_MSG("     - Pairing Success\n")
			}
			handleNotification.HID_Evt_Opcode = PEER_PAIR_HANDLE_EVT;
			handleNotification.ConnectionHandle = BleApplicationContext.BleApplicationContext_legacy.connectionHandle;
			HID_Host_Notification(&handleNotification);
		}
			break;

		default:
			break;
		}
	}
		break;

	case HCI_DISCONNECTION_COMPLETE_EVT_CODE: {
		if (cc->Connection_Handle == BleApplicationContext.BleApplicationContext_legacy.connectionHandle) {
			BleApplicationContext.BleApplicationContext_legacy.connectionHandle = 0;
			BleApplicationContext.Device_Connection_Status = APP_BLE_IDLE;
			APP_DBG_MSG("\r\n\r** DISCONNECTION EVENT WITH SERVER \n\r")
			handleNotification.HID_Evt_Opcode = PEER_DISCON_HANDLE_EVT;
			handleNotification.ConnectionHandle = BleApplicationContext.BleApplicationContext_legacy.connectionHandle;
			HID_Host_Notification(&handleNotification);
			// Restart scan
			UTIL_SEQ_SetTask(1 << CFG_TASK_START_SCAN_ID, CFG_SCH_PRIO_0);
		}
	}
		break; /* HCI_DISCONNECTION_COMPLETE_EVT_CODE */

	case HCI_LE_META_EVT_CODE: {
		meta_evt = (evt_le_meta_event*) event_pckt->data;

		switch (meta_evt->subevent) {

		case HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE: {
			/**
			 * The connection is done,
			 */
			connection_complete_event = (hci_le_connection_complete_event_rp0*) meta_evt->data;
			BleApplicationContext.BleApplicationContext_legacy.connectionHandle =
					connection_complete_event->Connection_Handle;
			BleApplicationContext.Device_Connection_Status = APP_BLE_CONNECTED;

			APP_DBG_MSG("\r\n\r**  CONNECTION COMPLETE EVENT\n\r")
			handleNotification.HID_Evt_Opcode = PEER_CONN_HANDLE_EVT;
			handleNotification.ConnectionHandle = BleApplicationContext.BleApplicationContext_legacy.connectionHandle;
			HID_Host_Notification(&handleNotification);
			// Start pairing
			UTIL_SEQ_SetTask(1 << CFG_TASK_PAIR_DEV_ID, CFG_SCH_PRIO_0);
		}
		break; /* HCI_LE_CONNECTION_COMPLETE_SUBEVT_CODE */

		case HCI_LE_ADVERTISING_REPORT_SUBEVT_CODE: {
			uint8_t *adv_report_data;
			le_advertising_event = (hci_le_advertising_report_event_rp0*) meta_evt->data;

			event_type = le_advertising_event->Advertising_Report[0].Event_Type;

			event_data_size = le_advertising_event->Advertising_Report[0].Length_Data;

			/* WARNING: be careful when decoding advertising report as its raw format cannot be mapped on a C structure.
			 The data and RSSI values could not be directly decoded from the RAM using the data and RSSI field from hci_le_advertising_report_event_rp0 structure.
			 Instead they must be read by using offsets (please refer to BLE specification).
			 RSSI = (int8_t)*(uint8_t*) (adv_report_data + le_advertising_event->Advertising_Report[0].Length_Data);
			 */
			adv_report_data = (uint8_t*) (&le_advertising_event->Advertising_Report[0].Length_Data) + 1;
			k = 0;

			if (event_type == ADV_IND || event_type == SCAN_RSP) {
				while (k < event_data_size) {
					adlength = adv_report_data[k];
					adtype = adv_report_data[k + 1];
					switch (adtype) {
					case AD_TYPE_FLAGS:
						break;

					case AD_TYPE_COMPLETE_LOCAL_NAME:
						if (adlength == 25 && memcmp(&adv_report_data[k + 2], completeLocalName, 24) == 0) {
							APP_DBG_MSG("-- XBOX CONTROLLER DETECTED -- VIA COMPLETE LOCAL NAME\n\r")
							BleApplicationContext.DeviceServerFound = 0x01;
							SERVER_REMOTE_ADDR_TYPE = le_advertising_event->Advertising_Report[0].Address_Type;
							SERVER_REMOTE_BDADDR[0] = le_advertising_event->Advertising_Report[0].Address[0];
							SERVER_REMOTE_BDADDR[1] = le_advertising_event->Advertising_Report[0].Address[1];
							SERVER_REMOTE_BDADDR[2] = le_advertising_event->Advertising_Report[0].Address[2];
							SERVER_REMOTE_BDADDR[3] = le_advertising_event->Advertising_Report[0].Address[3];
							SERVER_REMOTE_BDADDR[4] = le_advertising_event->Advertising_Report[0].Address[4];
							SERVER_REMOTE_BDADDR[5] = le_advertising_event->Advertising_Report[0].Address[5];

							// Immediatly stop advertising since we found the device
							aci_gap_terminate_gap_proc(GAP_GENERAL_DISCOVERY_PROC);
						}
						break;

					case AD_TYPE_TX_POWER_LEVEL:
						break;

					case AD_TYPE_MANUFACTURER_SPECIFIC_DATA:
						break;

					case AD_TYPE_SERVICE_DATA:
						break;

					default:
						break;
					}
					k += adlength + 1;
				}
			}

		}
			break;

		default:
			break;
		}
	}
		break; /* HCI_LE_META_EVT_CODE */

	default:
		break;
	}

	return (SVCCTL_UserEvtFlowEnable);
}

APP_BLE_ConnStatus_t APP_BLE_Get_Client_Connection_Status(uint16_t Connection_Handle) {
	if (BleApplicationContext.BleApplicationContext_legacy.connectionHandle == Connection_Handle) {
		return BleApplicationContext.Device_Connection_Status;
	}
	return APP_BLE_IDLE;
}

/*************************************************************
 *
 * LOCAL FUNCTIONS
 *
 *************************************************************/
static void Ble_Tl_Init(void) {
	HCI_TL_HciInitConf_t Hci_Tl_Init_Conf;

	Hci_Tl_Init_Conf.p_cmdbuffer = (uint8_t*) &BleCmdBuffer;
	Hci_Tl_Init_Conf.StatusNotCallBack = BLE_StatusNot;
	hci_init(BLE_UserEvtRx, (void*) &Hci_Tl_Init_Conf);

	return;
}

static void Ble_Hci_Gap_Gatt_Init(void) {
	uint8_t role;
	uint16_t gap_service_handle, gap_dev_name_char_handle, gap_appearance_char_handle;
	const uint8_t *p_bd_addr;

	uint16_t a_appearance[1] = { BLE_CFG_GAP_APPEARANCE };
	tBleStatus ret = BLE_STATUS_INVALID_PARAMS;

	APP_DBG_MSG("==>> Start Ble_Hci_Gap_Gatt_Init function\n")
	;

	/**
	 * Initialize HCI layer
	 */
	/*HCI Reset to synchronise BLE Stack*/
	ret = hci_reset();
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : hci_reset command, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: hci_reset command\n")
		;
	}

	/**
	 * Write the BD Address
	 */
	p_bd_addr = BleGetBdAddress();
	ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, (uint8_t*) p_bd_addr);
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_PUBADDR_OFFSET\n")
		;
		APP_DBG_MSG("  Public Bluetooth Address: %02x:%02x:%02x:%02x:%02x:%02x\n",p_bd_addr[5],p_bd_addr[4],p_bd_addr[3],p_bd_addr[2],p_bd_addr[1],p_bd_addr[0])
		;
	}

	/**
	 * Write Identity root key used to derive IRK and DHK(Legacy)
	 */
	ret = aci_hal_write_config_data(CONFIG_DATA_IR_OFFSET, CONFIG_DATA_IR_LEN, (uint8_t*) a_BLE_CfgIrValue);
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_IR_OFFSET, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_IR_OFFSET\n")
		;
	}

	/**
	 * Write Encryption root key used to derive LTK and CSRK
	 */
	ret = aci_hal_write_config_data(CONFIG_DATA_ER_OFFSET, CONFIG_DATA_ER_LEN, (uint8_t*) a_BLE_CfgErValue);
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_hal_write_config_data command - CONFIG_DATA_ER_OFFSET, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_hal_write_config_data command - CONFIG_DATA_ER_OFFSET\n")
		;
	}

	/**
	 * Set TX Power.
	 */
	ret = aci_hal_set_tx_power_level(1, CFG_TX_POWER);
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_hal_set_tx_power_level command, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_hal_set_tx_power_level command\n")
		;
	}

	/**
	 * Initialize GATT interface
	 */
	ret = aci_gatt_init();
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_gatt_init command, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_gatt_init command\n")
		;
	}

	/**
	 * Initialize GAP interface
	 */
	role = 0;

#if (BLE_CFG_PERIPHERAL == 1)
  role |= GAP_PERIPHERAL_ROLE;
#endif /* BLE_CFG_PERIPHERAL == 1 */

#if (BLE_CFG_CENTRAL == 1)
	role |= GAP_CENTRAL_ROLE;
#endif /* BLE_CFG_CENTRAL == 1 */

	/* USER CODE BEGIN Role_Mngt*/

	/* USER CODE END Role_Mngt */

	if (role > 0) {
		ret = aci_gap_init(role,
		CFG_PRIVACY,
		APPBLE_GAP_DEVICE_NAME_LENGTH, &gap_service_handle, &gap_dev_name_char_handle, &gap_appearance_char_handle);

		if (ret != BLE_STATUS_SUCCESS) {
			APP_DBG_MSG("  Fail   : aci_gap_init command, result: 0x%x \n", ret)
			;
		} else {
			APP_DBG_MSG("  Success: aci_gap_init command\n")
			;
		}

		ret = aci_gatt_update_char_value(gap_service_handle, gap_dev_name_char_handle, 0, CFG_GAP_DEVICE_NAME_LENGTH,
				(uint8_t*) CFG_GAP_DEVICE_NAME);
		if (ret != BLE_STATUS_SUCCESS) {
			BLE_DBG_SVCCTL_MSG("  Fail   : aci_gatt_update_char_value - Device Name\n");
		} else {
			BLE_DBG_SVCCTL_MSG("  Success: aci_gatt_update_char_value - Device Name\n");
		}
	}

	ret = aci_gatt_update_char_value(gap_service_handle, gap_appearance_char_handle, 0, 2, (uint8_t*) &a_appearance);
	if (ret != BLE_STATUS_SUCCESS) {
		BLE_DBG_SVCCTL_MSG("  Fail   : aci_gatt_update_char_value - Appearance\n");
	} else {
		BLE_DBG_SVCCTL_MSG("  Success: aci_gatt_update_char_value - Appearance\n");
	}

	/**
	 * Initialize IO capability
	 */
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.ioCapability = CFG_IO_CAPABILITY;
	ret = aci_gap_set_io_capability(BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.ioCapability);
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_gap_set_io_capability command, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_gap_set_io_capability command\n")
		;
	}

	/**
	 * Initialize authentication
	 */
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.mitm_mode = CFG_MITM_PROTECTION;
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin =
	CFG_ENCRYPTION_KEY_SIZE_MIN;
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax =
	CFG_ENCRYPTION_KEY_SIZE_MAX;
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.Use_Fixed_Pin = CFG_USED_FIXED_PIN;
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.Fixed_Pin = CFG_FIXED_PIN;
	BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode = CFG_BONDING_MODE;

	ret = aci_gap_set_authentication_requirement(
			BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode,
			BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.mitm_mode,
			CFG_SC_SUPPORT,
			CFG_KEYPRESS_NOTIFICATION_SUPPORT,
			BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMin,
			BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.encryptionKeySizeMax,
			BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.Use_Fixed_Pin,
			BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.Fixed_Pin,
			CFG_IDENTITY_ADDRESS);
	if (ret != BLE_STATUS_SUCCESS) {
		APP_DBG_MSG("  Fail   : aci_gap_set_authentication_requirement command, result: 0x%x \n", ret)
		;
	} else {
		APP_DBG_MSG("  Success: aci_gap_set_authentication_requirement command\n")
		;
	}

	/**
	 * Initialize whitelist
	 */
	if (BleApplicationContext.BleApplicationContext_legacy.bleSecurityParam.bonding_mode) {
		ret = aci_gap_configure_whitelist();
		if (ret != BLE_STATUS_SUCCESS) {
			APP_DBG_MSG("  Fail   : aci_gap_configure_whitelist command, result: 0x%x \n", ret)
			;
		} else {
			APP_DBG_MSG("  Success: aci_gap_configure_whitelist command\n")
			;
		}
	}
	APP_DBG_MSG("==>> End Ble_Hci_Gap_Gatt_Init function\n\r")
	;
}

static void Scan_Request(void) {
	tBleStatus result;
	if (BleApplicationContext.Device_Connection_Status != APP_BLE_CONNECTED) {
		result = aci_gap_start_general_discovery_proc(SCAN_P, SCAN_L, CFG_BLE_ADDRESS_TYPE, 1);
		if (result == BLE_STATUS_SUCCESS) {
			APP_DBG_MSG(" \r\n\r** START GENERAL DISCOVERY (SCAN) **  \r\n\r")
		} else {
			APP_DBG_MSG("-- BLE_App_Start_Limited_Disc_Req, Failed \r\n\r")
		}
	}
	return;
}

static void Connect_Request(void) {
	tBleStatus result;

	if (BleApplicationContext.Device_Connection_Status != APP_BLE_CONNECTED) {
		APP_DBG_MSG("\r\n\r** CREATE CONNECTION TO SERVER **  \r\n\r")
		result = aci_gap_create_connection(SCAN_P,
		SCAN_L, SERVER_REMOTE_ADDR_TYPE, SERVER_REMOTE_BDADDR,
		CFG_BLE_ADDRESS_TYPE,
		CONN_P1,
		CONN_P2, 0,
		SUPERV_TIMEOUT,
		CONN_L1,
		CONN_L2);

		if (result == BLE_STATUS_SUCCESS) {
			BleApplicationContext.Device_Connection_Status = APP_BLE_CONNECTING;

		} else {
			BleApplicationContext.Device_Connection_Status = APP_BLE_IDLE;

		}
	}
	return;
}

static void Pairing_Request(void) {
	tBleStatus result;

	if (BleApplicationContext.Device_Connection_Status != APP_BLE_PAIRED) {
		APP_DBG_MSG("\r\n\r** SENDING PAIRING REQUEST TO SERVER **  \r\n\r")
		result = aci_gap_send_pairing_req(BleApplicationContext.BleApplicationContext_legacy.connectionHandle, 0x01);

		if (result == BLE_STATUS_SUCCESS) {
			BleApplicationContext.Device_Connection_Status = APP_BLE_PAIRING;

		} else {
			BleApplicationContext.Device_Connection_Status = APP_BLE_IDLE;

		}
	}
}

const uint8_t* BleGetBdAddress(void) {
	uint8_t *p_otp_addr;
	const uint8_t *p_bd_addr;
	uint32_t udn;
	uint32_t company_id;
	uint32_t device_id;

	udn = LL_FLASH_GetUDN();

	if (udn != 0xFFFFFFFF) {
		company_id = LL_FLASH_GetSTCompanyID();
		device_id = LL_FLASH_GetDeviceID();

		/**
		 * Public Address with the ST company ID
		 * bit[47:24] : 24bits (OUI) equal to the company ID
		 * bit[23:16] : Device ID.
		 * bit[15:0] : The last 16bits from the UDN
		 * Note: In order to use the Public Address in a final product, a dedicated
		 * 24bits company ID (OUI) shall be bought.
		 */
		a_BdAddrUdn[0] = (uint8_t) (udn & 0x000000FF);
		a_BdAddrUdn[1] = (uint8_t) ((udn & 0x0000FF00) >> 8);
		a_BdAddrUdn[2] = (uint8_t) device_id;
		a_BdAddrUdn[3] = (uint8_t) (company_id & 0x000000FF);
		a_BdAddrUdn[4] = (uint8_t) ((company_id & 0x0000FF00) >> 8);
		a_BdAddrUdn[5] = (uint8_t) ((company_id & 0x00FF0000) >> 16);

		p_bd_addr = (const uint8_t*) a_BdAddrUdn;
	} else {
		p_otp_addr = OTP_Read(0);
		if (p_otp_addr) {
			p_bd_addr = ((OTP_ID0_t*) p_otp_addr)->bd_address;
		} else {
			p_bd_addr = a_MBdAddr;
		}
	}

	return p_bd_addr;
}

/*************************************************************
 *
 * WRAP FUNCTIONS
 *
 *************************************************************/
void hci_notify_asynch_evt(void *pdata) {
	UTIL_SEQ_SetTask(1 << CFG_TASK_HCI_ASYNCH_EVT_ID, CFG_SCH_PRIO_0);
	return;
}

void hci_cmd_resp_release(uint32_t flag) {
	UTIL_SEQ_SetEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
	return;
}

void hci_cmd_resp_wait(uint32_t timeout) {
	UTIL_SEQ_WaitEvt(1 << CFG_IDLEEVT_HCI_CMD_EVT_RSP_ID);
	return;
}

static void BLE_UserEvtRx(void *pPayload) {
	SVCCTL_UserEvtFlowStatus_t svctl_return_status;
	tHCI_UserEvtRxParam *pParam;

	pParam = (tHCI_UserEvtRxParam*) pPayload;

	svctl_return_status = SVCCTL_UserEvtRx((void*) &(pParam->pckt->evtserial));
	if (svctl_return_status != SVCCTL_UserEvtFlowDisable) {
		pParam->status = HCI_TL_UserEventFlow_Enable;
	} else {
		pParam->status = HCI_TL_UserEventFlow_Disable;
	}

	return;
}

static void BLE_StatusNot(HCI_TL_CmdStatus_t status) {
	uint32_t task_id_list;
	switch (status) {
	case HCI_TL_CmdBusy:
		/**
		 * All tasks that may send an aci/hci commands shall be listed here
		 * This is to prevent a new command is sent while one is already pending
		 */
		task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
		UTIL_SEQ_PauseTask(task_id_list);
		break;

	case HCI_TL_CmdAvailable:
		/**
		 * All tasks that may send an aci/hci commands shall be listed here
		 * This is to prevent a new command is sent while one is already pending
		 */
		task_id_list = (1 << CFG_LAST_TASK_ID_WITH_HCICMD) - 1;
		UTIL_SEQ_ResumeTask(task_id_list);
		break;

	default:
		break;
	}
	return;
}

void SVCCTL_ResumeUserEventFlow(void) {
	hci_resume_flow();
	return;
}

/**
  ******************************************************************************
  * @file    App/p2p_client_app.h
  * @author  MCD Application Team
  * @brief   Header for p2p_client_app.c module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef HID_HOST_APP_H
#define HID_HOST_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  PEER_CONN_HANDLE_EVT,
  PEER_PAIR_HANDLE_EVT,
  PEER_DISCON_HANDLE_EVT,
} HID_APP_Opcode_Notification_evt_t;


typedef enum
{
	HID_HOST_IDLE,
	HID_HOST_CONNECTED,
	HID_HOST_DISCOVER_SERVICES,
	HID_HOST_DISCOVERING_SERVICES,
	HID_HOST_DISCOVER_HID_CHARACS,
	HID_HOST_DISCOVERING_HID_CHARACS,
	HID_HOST_DISCOVER_REPORT_1_DESC,
	HID_HOST_DISCOVERING_REPORT_1_DESC,
	HID_HOST_DISCOVER_REPORT_2_DESC,
	HID_HOST_DISCOVERING_REPORT_2_DESC,
	HID_HOST_DISCOVERING_BAT_CHARACS,
	HID_HOST_DISCOVER_LEVEL_CHARACS,
	HID_HOST_DISCOVER_LEVEL_DESC,
	HID_HOST_DISCOVERING_LEVEL_DESC,
	HID_HOST_ENABLE_ALL_NOTIFICATION_DESC,
	HID_HOST_DONE,
} HID_HOST_Status_t;


typedef struct
{
  HID_APP_Opcode_Notification_evt_t          HID_Evt_Opcode;
  uint16_t                                    ConnectionHandle;

} HID_APP_ConnHandle_Not_evt_t;

typedef struct __attribute__((packed))
{
  uint16_t JOY_LeftAxisX;                      		 // Left Joystick X, Value = 0 to 65535
  uint16_t JOY_LeftAxisY;                       	 // Left Joystick Y, Value = 0 to 65535
  uint16_t JOY_RightAxisX;                       	 // Right Joystick X, Value = 0 to 65535
  uint16_t JOY_RightAxisY;                     		 // Right Joystick Y, Value = 0 to 65535

  uint16_t TRG_Left : 10;                   		 // Left Trigger, Value = 0 to 1023
  uint8_t  : 6;

  uint16_t TRG_Right : 10;             				 // Right Trigger, Value = 0 to 1023
  uint8_t  : 6;

  uint8_t  HAT_Switch : 4;                  		// Hat switch, Value = 1 to 8, Physical = (Value - 1) x 45 in degrees
#define HATSWITCH_NONE          0x00
#define HATSWITCH_UP      		0x01
#define HATSWITCH_UPRIGHT       0x02
#define HATSWITCH_RIGHT         0x03
#define HATSWITCH_DOWNRIGHT     0x04
#define HATSWITCH_DOWN          0x05
#define HATSWITCH_DOWNLEFT      0x06
#define HATSWITCH_LEFT          0x07
#define HATSWITCH_UPLEFT        0x08
  uint8_t  : 4;

  uint8_t  BTN_A : 1;
  uint8_t  BTN_B : 1;
  uint8_t  BTN_RightJoystick : 1;
  uint8_t  BTN_X : 1;
  uint8_t  BTN_Y : 1;
  uint8_t  BTN_BackLeft : 1;
  uint8_t  BTN_BackRight : 1;

  uint8_t  : 2;
  uint8_t  BTN_View : 1;
  uint8_t  BTN_Menu : 1;
  uint8_t  BTN_Xbox : 1;
  uint8_t  BTN_LeftJoystick : 1;
  uint8_t  : 2;

  uint8_t  BTN_Profile : 1;
  uint8_t  : 7;
} HID_Report_t;

/* Exported constants --------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macros ------------------------------------------------------------*/

/* Exported functions ---------------------------------------------*/
void HID_Host_Init( void );
void HID_Host_Notification( HID_APP_ConnHandle_Not_evt_t *pNotification );
HID_HOST_Status_t HID_Host_Get_State(void);
HID_Report_t* HID_Host_Get_Report(void);
uint8_t HID_Host_Get_Battery_Level(void);

#ifdef __cplusplus
}
#endif

#endif /*HID_HOST_APP_H */

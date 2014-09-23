/**
  ******************************************************************************
  * @file    lib_ConfigManager.h 
  * @author  MMY Application Team
  * @version V4.0.0
  * @date    02/06/2014
  * @brief   This file provides set of firmware functions to manages device modes. 
  ******************************************************************************
  * @copyright
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  */ 

/* Define to prevent recursive inclusion --------------------------------------------*/
#ifndef _LIB_MANAGER_H
#define _LIB_MANAGER_H

/* Includes -------------------------------------------------------------------------*/
#include "lib_iso15693pcd.h"
#include "lib_iso14443Apcd.h"
#include "lib_iso14443Bpcd.h"
#include "lib_iso18092pcd.h"
#include "lib_nfcdepinitiator.h"


#include "lib_piccemulator.h"


#include "drv_LED.h"

/* Manager status and erroc code ----------------------------------------------------*/
#define MANAGER_SUCCESSCODE															RESULTOK
#define MANAGER_ERRORCODE_DEFAULT												0xF1
#define MANAGER_ERRORCODE_PORERROR											0xF2


/* Flags for PICC/PCD tracking  ----------------------------------------------------------*/
#define	TRACK_NOTHING			0x00
#define	TRACK_NFCTYPE1 		0x01 /* 0000 0001 */
#define	TRACK_NFCTYPE2 		0x02 /* 0000 0010 */
#define	TRACK_NFCTYPE3 		0x04 /* 0000 0100 */
#define	TRACK_NFCTYPE4A 	0x08 /* 0000 1000 */
#define	TRACK_NFCTYPE4B 	0x10 /* 0001 0000 */
#define	TRACK_NFCTYPE5 		0x20 /* 0010 0000 */
#define TRACK_ALL 				0xFF /* 1111 1111 */

/* Flags for Initiator/Target tracking  ------------------------------------------------------*/
#define	P2P_NOTHING				0x00
#define	INITIATOR_NFCA 		0x01 /* 0000 0001 */
#define	INITIATOR_NFCF 		0x02 /* 0000 0010 */
#define	TARGET_NFCA 			0x04 /* 0000 0100 */
#define	TARGET_NFCF 			0x08 /* 0000 1000 */
#define P2P_ALL 					0xFF /* 1111 1111 */

/* Flags for SelectMode  ----------------------------------------------------------*/
#define	SELECT_NOTHING		0x00
#define	SELECT_PCD			 	0x01 /* 0000 0001 */
#define	SELECT_PICC			 	0x02 /* 0000 0010 */
#define	SELECT_P2P			 	0x04 /* 0000 0100 */
#define SELECT_ALL 				0xFF /* 1111 1111 */

/* structure of the manager state----------------------------------------------------*/
//typedef struct {
//	bool TrackAllTags;
//	bool TrackTT1;
//	bool TrackTT2;
//	bool TrackTT3;
//	bool TrackTT4A;
//	bool TrackTT4B;
//	bool TrackTT5;
//}MANAGER_PCD_SELECTED_PROTOCOL;

//typedef struct {
////	bool EmulTT1; /* Not supported */
//	bool EmulTT2;
//	bool EmulTT3;
//	bool EmulTT4A;
//	bool EmulTT4B;
////	bool EmulTT5; /* under dev */
//}MANAGER_PICC_SELECTED_PROTOCOL;

//typedef struct {
//	bool NfcDep;
//	/* other Proprietary to add*/
//}MANAGER_P2P_SELECTED_PROTOCOL;


//typedef enum {
//	PCDMode,
//	PICCMode,
//	P2PMode,
//	AllMode
//}MANAGER_SELECTED_MODE;

typedef struct {
//	MANAGER_SELECTED_MODE SelectedMode;
// 	MANAGER_PCD_SELECTED_PROTOCOL	PcdMode;
//	MANAGER_PICC_SELECTED_PROTOCOL	PiccMode;
//	MANAGER_P2P_SELECTED_PROTOCOL	P2pMode;
	uint8_t SelectedMode;
	uint8_t PcdMode;
	uint8_t PiccMode;
	uint8_t P2pMode;
	uint8_t Result;
}MANAGER_CONFIG;


/* public function	 ----------------------------------------------------------------*/

void ConfigManager_HWInit (void);
void ConfigManager_AutoMode (MANAGER_CONFIG *pManagerConfig);
uint8_t ConfigManager_TagHunting ( uint8_t tagsToFind );
uint8_t ConfigManager_TagEmulation (PICCEMULATOR_SELECT_TAG_TYPE TagEmulated, uint16_t delay_ms);
uint8_t ConfigManager_P2P(uint8_t P2Pmode);

void ConfigManager_Stop(void);
#endif


/******************* (C) COPYRIGHT 2014 STMicroelectronics *****END OF FILE****/


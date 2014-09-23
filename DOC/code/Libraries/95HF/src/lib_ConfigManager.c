/**
  ******************************************************************************
  * @file    lib_ConfigManager.c 
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

/* Includes ------------------------------------------------------------------------------ */
#include "lib_ConfigManager.h"

/** @addtogroup _95HF_Libraries
 * 	@{
 *	@brief  <b>This is the library used by the whole 95HF family (RX95HF, CR95HF, ST95HF) <br />
 *				  You will find ISO libraries ( 14443A, 14443B, 15693, ...) for PICC and PCD <br />
 *				  The libraries selected in the project will depend of the application targetted <br />
 *				  and the product chosen (RX95HF emulate PICC, CR95HF emulate PCD, ST95HF can do both)</b>
 */

/** @addtogroup Config_Manager
 * 	@{
 *	@brief  This part of the library manage the configuration of the chip.
 */


/** @addtogroup Configuration_Manager
 * 	@{
 *	@brief  This file is used to select a configuration, PICC or PCD or Initiator/Target.
*/
static void ConfigManager_Init( void);
static int8_t ConfigManager_IDN(uint8_t *pResponse);
static void ConfigManager_Start(void );
static int8_t ConfigManager_PORsequence( void );

/** @addtogroup lib_ConfigManager_Private_Functions
 * 	@{
 */

/**
 * @brief  This buffer contains the data send/received by xx95HF
 */
extern uint8_t									u95HFBuffer [RFTRANS_95HF_MAX_BUFFER_SIZE+3];
extern ISO14443A_CARD ISO14443A_Card;
extern bool lockKEYUpDown;

bool		uAppliTimeOut;
uint8_t TagUID[16];

bool 	StopProcess = false;
PICCEMULATOR_SELECT_TAG_TYPE commandReceived = PICCEMULATOR_TAG_TYPE_UNKNOWN;


/**
 *	@brief  This function initialize the PICC
 *  @param  None 
 *  @retval None
 */
static void ConfigManager_Init( void)
{
	/* initialize the structure of the Rf tranceiver */
	drv95HF_InitConfigStructure ();
	
#ifdef SPI_INTERRUPT_MODE_ACTIVATED	
	/* inform driver to use interrupt mode */
	drv95HF_EnableInterrupt ( );
#endif /* SPI_INTERRUPT_MODE_ACTIVATED */
	
	/* configure the Serial interface to communicate with the RF transceiver */
	drv95HF_InitilizeSerialInterface ( );
}

/**
 *	@brief  this function sends an IDN command to the PICC device
 *  @param  pResponse : pointer on the PICC device reply
 *  @retval PICC_SUCCESSCODE : the function is succesful 
 */
static int8_t ConfigManager_IDN(uint8_t *pResponse)
{
	uc8 DataToSend[] = {IDN	,0x00};

	/* send the command to the PICC and retrieve its response */
	drv95HF_SendReceive(DataToSend, pResponse);

	return MANAGER_SUCCESSCODE;
}

/**
 *	@brief  This function set a variable to inform Manager that a task is on going
 *  @param  none
 *  @retval none
 */
static void ConfigManager_Start( void )
{
	StopProcess = false;
	uAppliTimeOut = false;
}

/**
 *	@brief  This function sends POR sequence. It is used to initialize chip after a POR.
 *  @param  none
 *  @retval MANAGER_ERRORCODE_PORERROR : the POR sequence doesn't succeded
 *  @retval MANAGER_SUCCESSCODE : chip is ready
 */
static int8_t ConfigManager_PORsequence( void )
{
	uint16_t NthAttempt=0;
	uc8 command[]= {ECHO};

	if(drv95HF_GetSerialInterface() == RFTRANS_95HF_INTERFACE_SPI)
	{
		drv95HF_ResetSPI();		
	}
	
	do{
	
		/* send an ECHO command and checks response */
		drv95HF_SendReceive(command, u95HFBuffer);

		if (u95HFBuffer[0]==ECHORESPONSE)
			return MANAGER_SUCCESSCODE;	

		/* if the SPI interface is selected then send a reset command*/
		if(drv95HF_GetSerialInterface() == RFTRANS_95HF_INTERFACE_SPI)
		{	
			drv95HF_ResetSPI();				
		}
#ifdef USE_CR95HF_DEVICE		
		/* if the UART interface is selected then send 255 ECHO commands*/
		else if(drv95HF_GetSerialInterface() == RFTRANS_95HF_INTERFACE_UART)
		{
			do {
				/* send an ECHO command and checks response */
				drv95HF_SendReceive(command, u95HFBuffer);
				if (u95HFBuffer[0] == ECHORESPONSE)
					return MANAGER_SUCCESSCODE;	
			}while(NthAttempt++ < RFTRANS_95HF_MAX_BUFFER_SIZE);
		}
#endif /* USE_CR95HF_DEVICE */
	} while (u95HFBuffer[0]!=ECHORESPONSE && NthAttempt++ <5);

	return MANAGER_ERRORCODE_PORERROR;
}


/**
  * @}
  */ 

 /** @addtogroup lib_ConfigManager_Public_Functions
 * 	@{
 */

/**
 *	@brief  This interface function inform Manager that current task must be stopped
 *  @param  none
 *  @retval none
 */
void ConfigManager_Stop(void )
{
	StopProcess = true;
}

/**
 * @brief  This function initialize the NFC chip 
 * @brief  Physical communication with chip enabled, RF communication not enabled
 * @param  None
 * @retval None
 */
void ConfigManager_HWInit (void)
{
	
	/* Initialize HW according to protocol to use */
	ConfigManager_Init();
	
	/* initilialize the RF transceiver */
	if (ConfigManager_PORsequence( ) != MANAGER_SUCCESSCODE)
	{
		/* nothing to do, this is a trap for debug purpose you can use it to detect HW issue */
		/* or GPIO config issue */
	}
	
	ConfigManager_IDN(u95HFBuffer);

}


/**
 * @brief  This function launch an automatic mode where NFC chip change is state 
 * @brief  (card reader, card emulation, P2P)
 * @param  *pManagerConfig configuration that must be done by the NFC chip
 * @retval None
 */
void ConfigManager_AutoMode (MANAGER_CONFIG *pManagerConfig)
{
	ConfigManager_Start();

	while(!StopProcess)
	{	
		/** PCD looking for a tag **/
		if(!StopProcess&&(pManagerConfig->SelectedMode&SELECT_PCD) != 0)
		{
			if (ConfigManager_TagHunting(pManagerConfig->PcdMode) != TRACK_NOTHING)
				break;
		}	
		
		/** PICC looking for a PCD **/
		if(!StopProcess&&(pManagerConfig->SelectedMode&SELECT_PICC) != 0)
		{
			/* TT2 emulation */
			if((pManagerConfig->PiccMode&TRACK_NFCTYPE2) != 0)
			{
				if (ConfigManager_TagEmulation(PICCEMULATOR_TAG_TYPE_2,500) != TRACK_NOTHING)
					break;
			}
			/* TT3 emulation */
			if(!StopProcess&&(pManagerConfig->PiccMode&TRACK_NFCTYPE3) != 0)
			{
				if (ConfigManager_TagEmulation(PICCEMULATOR_TAG_TYPE_3,500) != TRACK_NOTHING)
					break;
			}
			/* TT4A emulation */
			if(!StopProcess&&(pManagerConfig->PiccMode&TRACK_NFCTYPE4A) != 0)
			{
				if (ConfigManager_TagEmulation(PICCEMULATOR_TAG_TYPE_4A,500) != TRACK_NOTHING)
					break;
			}
			/* TT4B emulation */
			if(!StopProcess&&(pManagerConfig->PiccMode&TRACK_NFCTYPE4B) != 0)
			{
				if (ConfigManager_TagEmulation(PICCEMULATOR_TAG_TYPE_4B,500) != TRACK_NOTHING)
					break;
			}
		}
		
		/** Initiator/Target mode **/
		if(!StopProcess&&(pManagerConfig->SelectedMode&SELECT_P2P) != 0)
		{
			pManagerConfig->Result = ConfigManager_P2P(pManagerConfig->P2pMode);
			if( pManagerConfig->Result & 0x0F)
					break;
		}		
	}	
}


/**  
* @brief  	this function searches if a NFC or RFID tag is in the RF field. 
* @brief  	The method used is this described by the NFC specification
* @param  	tagsToFind : Flags to select the different kinds of tag to track, same as return value
* @retval 	TRACK_NOTHING : No tag in the RF field
* @retval 	TRACK_NFCTYPE1 : A NFC type1 tag is present in the RF field
* @retval 	TRACK_NFCTYPE2 : A NFC type2 tag is present in the RF field
* @retval 	TRACK_NFCTYPE3 : A NFC type3 tag is present in the RF field
* @retval 	TRACK_NFCTYPE4A : A NFC type4A tag is present in the RF field
* @retval 	TRACK_NFCTYPE4B : A NFC type4B tag is present in the RF field
* @retval 	TRACK_NFCTYPE5 : A ISO/IEC 15693 type A tag is present in the RF field
*/
uint8_t ConfigManager_TagHunting ( uint8_t tagsToFind )
{
	ConfigManager_Start();
	
	/*******NFC type 1 ********/
	if (tagsToFind&TRACK_NFCTYPE1)
	{
		PCD_FieldOff();
		delay_ms(5);
		PCD_FieldOn();
		ISO14443A_Init( );
		delay_ms(5);
		if(ISO14443A_IsPresent() == RESULTOK)
		{		
			if(TOPAZ_ID(TagUID) == RESULTOK)
				return TRACK_NFCTYPE1;	
		}
	}
	/*******NFC type 2 and 4A ********/
	if ((tagsToFind&TRACK_NFCTYPE2) || (tagsToFind&TRACK_NFCTYPE4A))
	{
		PCD_FieldOff();
		delay_ms(5);
		PCD_FieldOn();
		ISO14443A_Init( );
		delay_ms(5);
		if(ISO14443A_IsPresent() == RESULTOK)
		{			
			if(ISO14443A_Anticollision() == RESULTOK)
			{	
				if (ISO14443A_Card.SAK == 0x00 && tagsToFind&TRACK_NFCTYPE2) /* TT2 */
					return TRACK_NFCTYPE2;
				else if (ISO14443A_Card.SAK != 0x00 && tagsToFind&TRACK_NFCTYPE4A)/* TT4A */
					return TRACK_NFCTYPE4A;
			}
		}
	}
	/*******NFC type 3 ********/
	if (tagsToFind&TRACK_NFCTYPE3)
	{
		PCD_FieldOff();
		delay_ms(5);
		PCD_FieldOn();
		delay_ms(5);
		FELICA_Initialization();
		if(FELICA_IsPresent() == RESULTOK )
			return TRACK_NFCTYPE3;
	}
	/*******NFC type 4B ********/
	if (tagsToFind&TRACK_NFCTYPE4B)
	{
		PCD_FieldOff();
		delay_ms(5);
		PCD_FieldOn();
		delay_ms(5);
		if(ISO14443B_IsPresent() == RESULTOK )
		{
			delay_us (50);
			if(ISO14443B_Anticollision() == RESULTOK)
			{
				return TRACK_NFCTYPE4B;
			}
		}
	}
	/*******ISO15693 ********/
	if (tagsToFind&TRACK_NFCTYPE5)
	{
		PCD_FieldOff();
		delay_ms(5);
		PCD_FieldOn();
		delay_ms(5);
		if(ISO15693_GetUID (TagUID) == RESULTOK)	
			return TRACK_NFCTYPE5;
	}
	
	PCD_FieldOff();
	
	/* No tag found */
	return TRACK_NOTHING;
}


/**  
* @brief  	this function configure the chip in tag emulation mode 
* @param  	TagEmulated : mode of emulation
* @param  	delay_ms : chip must stay in the tag emulation mode for xx ms
* @retval 	TRACK_NOTHING : No tag in the RF field
*/
uint8_t ConfigManager_TagEmulation (PICCEMULATOR_SELECT_TAG_TYPE TagEmulated, uint16_t delay_ms)
{
	bool readerFound = false;
	PICCEMULATOR_STATE EmulState = PICCSTATE_UNKNOWN;
	
	ConfigManager_Start();
	
	PICCEmul_InitPICCEmulation(TagEmulated);
			
	if( delay_ms != 0)
		StartAppliTimeOut(delay_ms);
	else
	{
		/* don't start timer, but perform infinite loop */
	}
	commandReceived = PICCEMULATOR_TAG_TYPE_UNKNOWN;
	while(!StopProcess && !uAppliTimeOut)
	{	
		EmulState = PICCEmul_ManagePICCEmulation ();
		
		/* if data are exchanged do not let appli timer reach 0 so reload */
		if( EmulState == PICCSTATE_DATA_EXCHANGED && delay_ms != 0)
		{
			StartAppliTimeOut(delay_ms);
		}
		if (commandReceived == TagEmulated && delay_ms != 0)
		{
			readerFound = true;
		}
	}
	
	PICCEmul_DisablePICCEmulation();
	if (readerFound)
		return TRACK_ALL;
	else
		return TRACK_NOTHING;
}

/**  
* @brief  	this function configure the chip in P2P initiator or target 
* @param  	P2Pmode : mode of emulation
* @param  	delay_ms : chip must stay in the tag emulation mode for xx ms
* @retval 	TRACK_NOTHING : No tag in the RF field
*/
uint8_t ConfigManager_P2P(uint8_t P2Pmode)
{
	uint32_t cmpt;
	uint8_t initState = 0;
	PICCEMULATOR_STATE EmulState = PICCSTATE_UNKNOWN;	
	
	ConfigManager_Start(); /* Initialize StopProcess to false */
	
	/* Random state */
	initState = random(0,3);
	
	if (initState == 0)
		goto INFCA;
	else if (initState == 1)
		goto TNFCA;
	else if (initState == 2)
		goto INFCF;
	else if (initState == 3)
		goto TNFCF;
	
	while(!StopProcess)
	{
INFCA: /* Initiator NFCA */
		if( (P2Pmode & INITIATOR_NFCA) == INITIATOR_NFCA)
		{
			PCDNFCDEP_Init(NFCDEP_ON_NFCA, true);
			
			cmpt = 10;
			while(!StopProcess && cmpt )
			{	
				if(PCDNFCDEP_IsPresentNFCA() == PCDNFCDEP_SUCESSSCODE)
					return INITIATOR_NFCA;
				cmpt--;
			}
		}
INFCF: /* Initiator NFCF */
		if( (P2Pmode & INITIATOR_NFCF) == INITIATOR_NFCF)	
		{
			PCDNFCDEP_Init(NFCDEP_ON_NFCF, true);
			
			cmpt = 40;
			while(!StopProcess && cmpt )
			{	
				if( PCDNFCDEP_IsPresentNFCF() == PCDNFCDEP_SUCESSSCODE)
					return INITIATOR_NFCF;
				cmpt--;
			}
		}		
TNFCA: /* Target NFCA */
		if( (P2Pmode & TARGET_NFCA) == TARGET_NFCA)
		{			
			PICCEmul_InitPICCEmulation(PICCEMULATOR_NFCDEP_TYPE_A);	
			cmpt = 100000;
			while(!StopProcess && cmpt )
			{	
				EmulState = PICCEmul_ManagePICCEmulation ();	
				/* if data are exchanged inform appli */
				if( EmulState == PICCSTATE_DATA_EXCHANGED)
					return TARGET_NFCA;
				cmpt--;
			}
		}
TNFCF: /* Target NFCF */
		if( (P2Pmode & TARGET_NFCF) == TARGET_NFCF)
		{		
			PICCEmul_InitPICCEmulation(PICCEMULATOR_NFCDEP_TYPE_F);
			cmpt = 100000;
			while(!StopProcess && cmpt )
			{	
				EmulState = PICCEmul_ManagePICCEmulation ();	
				/* if data are exchanged inform appli */
				if( EmulState == PICCSTATE_DATA_EXCHANGED)
					return TARGET_NFCF;
				cmpt--;
			}
		}
	}
	return P2P_NOTHING;
}
	
	
	
	

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2014 STMicroelectronics *****END OF FILE****/

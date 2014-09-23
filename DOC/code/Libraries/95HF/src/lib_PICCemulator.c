/**
  ******************************************************************************
  * @file    lib_PICCEmulator.c 
  * @author  MMY Application Team
  * @version V4.0.0
  * @date    02/06/2014
  * @brief   This file provides set of firmware functions to manages picc device. 
  * @brief   The commands as defined in RX95HF or ST95HF datasheet
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
#include "lib_PICCEmulator.h"


/** @addtogroup _95HF_Libraries
 * 	@{
 *	@brief  <b>This is the library used by the whole 95HF family (RX95HF, CR95HF, ST95HF) <br />
 *				  You will find ISO libraries ( 14443A, 14443B, 15693, ...) for PICC and PCD <br />
 *				  The libraries selected in the project will depend of the application targetted <br />
 *				  and the product chosen (RX95HF emulate PICC, CR95HF emulate PCD, ST95HF can do both)</b>
 */

/** @addtogroup PICC
 * 	@{
 *	@brief  This part of the library enables PICC capabilities of RX95HF & ST95HF.
 */


/** @addtogroup PICC_Emulator
 * 	@{
 *	@brief  This file is used to emulate a PICC in order to answer PCD's request.
*/


static void 					PICCEmul_ReceiveCommand 						( void );
static int8_t 				PICCEmul_InitializeProtocol 				( void );
static int8_t 				PICCEmul_Init14443APicc							( void );
static void						PICCEmul_Init												( void );
static void						PICCEmul_RFField_CutOff							( void );
static int8_t 				PICCEmul_CheckForPCD								( void );
static __INLINE void 	PICCEmul_Listen											( void );


 /* extern variables ---------------------------------------------------------------- */
uint8_t		UID_TypeA[]={CASCADE_TAG,ST_MANUF_ID,ST95HF_REF_ID,0x74,0x4A,0xEF,0x22,0x80,0x00,0x00};


/* Variables for the different modes */
extern ST95Mode st95mode;
extern ST95TagType st95tagtype;

/**
 * @brief  This buffer contains the data send/received by xx95HF
 */
extern uint8_t									u95HFBuffer [RFTRANS_95HF_MAX_BUFFER_SIZE+3];

/* In case of TT4 we must append CRC, this variable is used to know it */
bool ISO14443A_TT4Used = false;

extern bool RF_DataReady;
extern bool RF_DataExpected;
extern bool uDataReady;
extern PICCEMULATOR_SELECT_TAG_TYPE commandReceived;

extern uint8_t PICCNFCT2_sector;

/** @addtogroup PICC_Emulator_Private_Functions
 *  @{
 */

PICCEMULATOR					CardEmulator;

/**
 * @brief  this function reads from the interface bus the command sent by the reader and returns a response
 */
void PICCEmul_ReceiveCommand( void )
{
	uint8_t *pData = u95HFBuffer;
	int8_t 	status;
	
	errchk( PICC_PollData(pData));
		
	/* the RF field has just turned off*/
	if (pData[PICC_STATUS_OFFSET] == LISTEN_ERRORCODE_NOFIELD && pData[PICC_LENGTH_OFFSET] == 0x00)
	{	
		/* RF field cut off */ 
		CardEmulator.State = PICCSTATE_RFFIELD_CUTOFF;	
	}	
	else 
	{	
		if( pData[PICC_COMMAND_OFFSET] != LISTEN_RESULTSCODE_OK)
			goto Error;
		
		/* according to the Tag Type or protocol selected */
		switch (CardEmulator.TagType)
		{
			case PICCEMULATOR_TAG_TYPE_1:
				/* not supported by ST95HF */
			break;
			
			case PICCEMULATOR_TAG_TYPE_2:
				if (PICCNFCT2_ReplyCommand ( pData ) != PICCNFCT2_ERRORCODE_COMMANDUNKNOWN)
					commandReceived = PICCEMULATOR_TAG_TYPE_2;
			break;
			
			case PICCEMULATOR_TAG_TYPE_3:
				/* not supported by ST95HF */
			break;
			
			case PICCEMULATOR_TAG_TYPE_4A: 
				if (PICCNFCT4_ReplyCommand ( pData ) != PICCNFCT4_ERRORCODE_COMMANDUNKNOWN)
					commandReceived = PICCEMULATOR_TAG_TYPE_4A;
			break;	
				
			case PICCEMULATOR_TAG_TYPE_4B: 
				/* not supported by ST95HF */
			break;	

			case PICCEMULATOR_NFCDEP_TYPE_A:  
			case PICCEMULATOR_LLCP_TYPE_A: 
				PICCNFCDEP_ReplyCommand( pData );
			break;				
			
			case PICCPROTOCOL_UNKNOWN:
			default:
			break;				
		}
							
		/* activate the listen mode */
		PICCEmul_Listen();			
	}	
	
	return;

/* In case of error Framing, CRC...or error result we must go back to listen */
Error:		
	
	/* activate the listen mode */
	PICCEmul_Listen();
}


static __INLINE void PICCEmul_Listen( void)
{
	
	/* Before Enabling Listen mode and waiting for interruption */
	/* Start to check everything is fine with 95HF */
	PICC_Echo(u95HFBuffer);
	
	if (u95HFBuffer[PICC_STATUS_OFFSET] != ECHORESPONSE)
	{
		/* reset the device */
		PICC_PORsequence( );
	}
	
  /* activate the listen mode */
	PICC_Listen(u95HFBuffer);
	/* after field is detected anti-collision process will be managed automatically for ISO14443A */
	/* we just have to wait for 1st interruption to raise to know anticollision is passed */
	/* or field cut off */
				
	if (u95HFBuffer[PICC_STATUS_OFFSET] == LISTEN_ERRORCODE_NOFIELD)
	{	   
		CardEmulator.State = PICCSTATE_WAIT_RFFIELD;
	}
	else if (u95HFBuffer[PICC_STATUS_OFFSET]== LISTEN_RESULTSCODE_OK_0X00)
	{			
		/* In some case data is ready before IT was set */
		/* Take care of this case here by checking GPIO state */
		if(GPIO_ReadInputDataBit(EXTI_GPIO_PORT,EXTI_RFTRANS_95HF_LINE) == Bit_RESET )
		{		
			CardEmulator.State = PICCSTATE_DATA_EXCHANGED;
			RF_DataExpected = false;
		}
		else
		{
			CardEmulator.State = PICCSTATE_ACTIVATED;
		}
	}
	else
	{
		CardEmulator.State = PICCSTATE_UNKNOWN;
	}
	
}

/**
 * @brief  this function configures the PICC as a card emulator
 * @param  Protocol : the RF protocol of the Picc (PICC_PROTOCOL_ISO14443A or PICC_PROTOCOL_ISO18092)
 * @retval 	PICCEMUL_SUCESSSCODE : the function is succesful
 * @retval 	PICCEMUL_ERROR_GENERIC : an error occured
 */
static int8_t PICCEmul_InitializeProtocol ( void )
{
	
	int8_t status;
	
	/* After a RF field cut off Tag must be reinitialized */
	PICCEmul_Init();
	
	/* send a Protocol Select command to configure 95HF device as a card emulator*/	
	switch ( CardEmulator.TagType )
	{
		case PICCEMULATOR_TAG_TYPE_1: 
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_TAG_TYPE_2: 
			errchk(PICCEmul_Init14443APicc	( ));
		break;
		
		case PICCEMULATOR_TAG_TYPE_3: 
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_TAG_TYPE_4A: 
			errchk(PICCEmul_Init14443APicc	( ));
		break;
		
		case PICCEMULATOR_TAG_TYPE_4B: 
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_NFCDEP_TYPE_A: 
		case PICCEMULATOR_LLCP_TYPE_A: 
			errchk(PICCEmul_Init14443APicc() );
		break;
		
		case PICCEMULATOR_NFCDEP_TYPE_F: 
		case PICCEMULATOR_LLCP_TYPE_F: 
			/* not supported by ST95HF */
		break;
		
		
		default:
		break;
		
	}
		
	return(PICCEmul_CheckForPCD());

Error:
	return PICCEMUL_ERROR_GENERIC;		
		
}

/**
 * @brief  this function initializes the Rf transceiver as 14443 type A Picc
 * @retval 	PICCEMUL_SUCESSSCODE : function succesful
 * @retval 	LIB14443APICC_ERRORCODE_CODECOMMAND : the command code doesn't match with this function
 */
static int8_t PICCEmul_Init14443APicc( void )
{
	uint8_t 		ParametersByte=PICC_PARAMETER_BYTE_ISO14443A,	
							ATQAParam[2]={0x44,0x00};
	int8_t 			status;

		
	/* send a Select command to configure 95HF device as a card emulator*/
	errchk(PICC_ProtocolSelect(0x02,PICC_PROTOCOL_ISO14443A,&(ParametersByte),u95HFBuffer));


	/* if Tag Type 2 SAK must be 0x00 => Not compliant to ISO/IEC 14443-4 */
	if( CardEmulator.TagType == PICCEMULATOR_TAG_TYPE_2) 
		errchk(PICC_AcFilter (0x0b,ATQAParam, 0x00,UID_TypeA,u95HFBuffer));
	/* if Tag Type 4 SAK must be 0x20 => Compliant to ISO/IEC 14443-4 */
	else if ( CardEmulator.TagType == PICCEMULATOR_TAG_TYPE_4A || CardEmulator.TagType == PICCEMULATOR_TAG_TYPE_4B) 
		errchk(PICC_AcFilter (0x0b,ATQAParam, 0x20,UID_TypeA,u95HFBuffer));
	/* SAK must be 0x40 => Not compliant to ISO/IEC 14443-4 NFC-DEP supported */
	else if( CardEmulator.TagType == PICCEMULATOR_NFCDEP_TYPE_A || CardEmulator.TagType == PICCEMULATOR_LLCP_TYPE_A)
		errchk(PICC_AcFilter (0x0b,ATQAParam, 0x40,UID_TypeA,u95HFBuffer));
	else // must not happend currently (to add TT4 + NFC-DEP support)
		return PICCEMUL_ERROR_GENERIC;
	
	/* change the load of the backscatering */
	errchk(PICC_SetBackscatteringLoad (0x27));	
	

	return  PICCEMUL_SUCESSSCODE;

Error:
	return PICCEMUL_ERROR_GENERIC;
}

/**
 * @brief  this function initializes the Rf transceiver as ISO/IEC 18092 Picc
 * @return 	PICCEMUL_SUCESSSCODE : function succesful
 * @return 	LIB14443APICC_ERRORCODE_CODECOMMAND : the command code doesn't match with this function
 */
static void	PICCEmul_Init( void )
{
	/* Prepare Memory according to tag emulated */
	switch(CardEmulator.TagType)
	{
		case PICCEMULATOR_TAG_TYPE_1:
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_TAG_TYPE_2:
			PICCNFCT2_Init();
		break;
		
		case PICCEMULATOR_TAG_TYPE_3:
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_TAG_TYPE_4A:
			ISO14443A_TT4Used = true;
			PICCNFCT4_Init(PICCNFCT4_TYPEA);
		break;
		
		case PICCEMULATOR_TAG_TYPE_4B:
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_NFCDEP_TYPE_A: 
			PICCNFCDEP_Init(NFCDEP_ON_NFCA, false);
		break;
		
		case PICCEMULATOR_NFCDEP_TYPE_F: 
			/* not supported by ST95HF */
		break;
	
		case PICCEMULATOR_LLCP_TYPE_A: 
			PICCNFCDEP_Init(NFCDEP_ON_NFCA, true);
		break;
		
		case PICCEMULATOR_LLCP_TYPE_F: 
			/* not supported by ST95HF */
		break;
		
		default:
		break;
	}
}

/**
 * @brief  this function initializes the Rf transceiver as ISO/IEC 18092 Picc
 * @return 	PICCEMUL_SUCESSSCODE : function succesful
 * @return 	LIB14443APICC_ERRORCODE_CODECOMMAND : the command code doesn't match with this function
 */
static void	PICCEmul_RFField_CutOff( void )
{
	uint8_t ParametersByte;
	uint8_t	ATQAParam[2]={0x44,0x00},
		pUIDData[]={CASCADE_TAG,ST_MANUF_ID,ST95HF_REF_ID,0x74,0x4A,0xEF,0x22,0x80};
	
	/* Inform protocol state machine about RF field cut off */
	switch(CardEmulator.TagType)
	{
		case PICCEMULATOR_TAG_TYPE_1:
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_TAG_TYPE_2:
//			PICCNFCT2_DeInit();
			/* After field cut off we should go back to 106kbit/s */
			ParametersByte=PICC_PARAMETER_BYTE_ISO14443A;
			PICC_ProtocolSelect(0x02,PICC_PROTOCOL_ISO14443A,&(ParametersByte),u95HFBuffer);
			/* relaunch anticol */
			PICC_AcFilter (0x0b,ATQAParam, 0x00,pUIDData,u95HFBuffer);
			/* Reset the sector to 0 */
			PICCNFCT2_sector = 0;
		break;
		
		case PICCEMULATOR_TAG_TYPE_3:
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_TAG_TYPE_4A:
			PICCNFCT4_DeInit(PICCNFCT4_TYPEA);
			/* After field cut off we should go back to 106kbit/s */
			ParametersByte=PICC_PARAMETER_BYTE_ISO14443A;
			PICC_ProtocolSelect(0x02,PICC_PROTOCOL_ISO14443A,&(ParametersByte),u95HFBuffer);
			/* relaunch anticol */
			PICC_AcFilter (0x0b,ATQAParam, 0x20,pUIDData,u95HFBuffer);
		break;
		
		case PICCEMULATOR_TAG_TYPE_4B:
			/* not supported by ST95HF */
		break;
		
		case PICCEMULATOR_NFCDEP_TYPE_A: 
		case PICCEMULATOR_LLCP_TYPE_A: 
			/* After field cut off we should go back to 106kbit/s */
			ParametersByte=PICC_PARAMETER_BYTE_ISO14443A;
			PICC_ProtocolSelect(0x02,PICC_PROTOCOL_ISO14443A,&(ParametersByte),u95HFBuffer);
			/* relaunch anticol */
			PICC_AcFilter (0x0b,ATQAParam, 0x40,pUIDData,u95HFBuffer); /* SAK 0x40 => Not compliant to ISO/IEC 14443-4 and NFC-DEP Protocol supported */
		break;
		
		case PICCEMULATOR_NFCDEP_TYPE_F:
		case PICCEMULATOR_LLCP_TYPE_F: 			
			/* Nothing to do */
		break;
	
		default:
		break;
	}
}

/**
 * @brief  this function check if PCD field is present or not
 * @param  none
 * @retval 	PICCEMUL_SUCESSSCODE : the function is succesful
 * @retval 	PICCEMUL_ERROR_GENERIC : an error occured
 */
static int8_t PICCEmul_CheckForPCD( void )
{
	
	PICCEmul_Listen();
		
	return PICCEMUL_SUCESSSCODE;

}

/**
  * @}
  */ 

 /** @addtogroup PICC_Emulator_Public_Functions
 *  @{
 */


/**
 * @brief  This function initializes the PICC 
 * @brief  Physical communication with PICC enabled, RF communication not enabled
 * @param  None
 * @retval None
 */
void PICCEmul_InitPICCEmulation ( PICCEMULATOR_SELECT_TAG_TYPE PICC_Emulated_TagType)
{	
	/* start init phase */
	st95mode = PICC;
	CardEmulator.TagType = PICC_Emulated_TagType;		
	if (PICC_Emulated_TagType == PICCEMULATOR_TAG_TYPE_2) st95tagtype = TT2;
	else if (PICC_Emulated_TagType == PICCEMULATOR_TAG_TYPE_4A) st95tagtype = TT4A;
	
	CardEmulator.State = PICCSTATE_UNKNOWN;

#ifdef SPI_INTERRUPT_MODE_ACTIVATED	
	PICC_Enable_Interrupt();
#endif /* SPI_INTERRUPT_MODE_ACTIVATED */
	
}

/**
 * @brief  This function disables the PICC 
 * @param  None
 * @retval None
 */
void PICCEmul_DisablePICCEmulation ( void )
{
	uint8_t	ATQAParam[2]={0x00, 0x00};
	uint8_t	pUIDData[]={0x00};
	uint8_t ParametersByte=0x00;


  /* If 95HF was in Listen mode, it's mandatory to send echo command to disable it */
	RF_DataExpected = false;

	PICC_Disable_Interrupt();

	/* Need 1 echo command to properly disable ST95HF if in listen mode */
	PICC_Echo(u95HFBuffer);
	
	/* Disable anti-collision if previousely set */ 
  if( CardEmulator.TagType == PICCEMULATOR_TAG_TYPE_4A || CardEmulator.TagType == PICCEMULATOR_TAG_TYPE_2 ||
	    CardEmulator.TagType == PICCEMULATOR_NFCDEP_TYPE_A || CardEmulator.TagType == PICCEMULATOR_LLCP_TYPE_A )
		PICC_AcFilter (0x01,ATQAParam, 0x00,pUIDData,u95HFBuffer);
	
	/* stop detected RF reader */	
	PICC_ProtocolSelect(0x02,PICC_PROTOCOL_FIELDOFF,&(ParametersByte),u95HFBuffer);	
	
	/* In case card emulator was set previousely in Iso14443A */
	ISO14443A_TT4Used = false;
	
	CardEmulator.State = PICCSTATE_DESACTIVATED;
	CardEmulator.Protocol = PICCPROTOCOL_UNKNOWN;
	CardEmulator.TagType = PICCEMULATOR_TAG_TYPE_UNKNOWN;
}

/**
 * @brief  this function manages the PICC 
 * @param  None
 * @retval None
 */
PICCEMULATOR_STATE PICCEmul_ManagePICCEmulation ( void )
{		
	switch ( CardEmulator.State )
	{
		/* the RF field has just turning off*/
		case PICCSTATE_RFFIELD_CUTOFF :
			/* need to inform RF field cut off for different state machine */
			PICCEmul_RFField_CutOff( );
			CardEmulator.State = PICCSTATE_WAIT_RFFIELD;			
		break;	
				
		case PICCSTATE_ACTIVATED :
#ifdef SPI_INTERRUPT_MODE_ACTIVATED
			if( RF_DataReady == true)
			{
				RF_DataReady = false;
				CardEmulator.State = PICCSTATE_DATA_EXCHANGED;
			}
#else	
			
			/* Here we check if RF even occurs, so if GPO of RX95HF is low */
			if(GPIO_ReadInputDataBit(EXTI_GPIO_PORT,EXTI_RFTRANS_95HF_LINE) == Bit_RESET)
			{	
				CardEmulator.State = PICCSTATE_DATA_EXCHANGED;
			}
#endif /* SPI_INTERRUPT_MODE_ACTIVATED */ /* else the treatement is done as soon as IT raises */		
		break;
		
		case PICCSTATE_DATA_EXCHANGED:
			/* an event had occured in the RF field */
			/* can be optimized if directly call in active state when it rises */
			/* Take care of the case data has come whereas we don't have the time to enable interrupt */
			RF_DataExpected = false;
			PICCEmul_ReceiveCommand ();	 	
		break;
			
		case PICCSTATE_WAIT_RFFIELD :	
			PICCEmul_CheckForPCD( );			
		  break;
	
		/* initialize the PICC in the card emulation mode */			
		default : 
			PICCEmul_InitializeProtocol ( );
		break; 		
	}

	
	return CardEmulator.State;
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

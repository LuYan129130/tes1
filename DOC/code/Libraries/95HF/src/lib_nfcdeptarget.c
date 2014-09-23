/**
  ******************************************************************************
  * @file    lib_nfcdeptarget.c 
  * @author  MMY Application Team
  * @version V4.0.0
  * @date    02/06/2014
  * @brief   Manage the NFC DEP commands
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
#include "lib_nfcdeptarget.h"


/** @addtogroup _95HF_Libraries
 * 	@{
 *	@brief  <b>This is the library used by the whole 95HF family (RX95HF, CR95HF, ST95HF) <br />
 *				  You will find ISO libraries ( 14443A, 14443B, 15693, ...) for PICC and PCD <br />
 *				  The libraries selected in the project will depend of the application targetted <br />
 *				  and the product chosen (RX95HF emulate PICC, CR95HF emulate PCD, ST95HF can do both)</b>
 */

/** @addtogroup P2P
 * 	@{
 *	@brief  This part of the library enables P2P capabilities of ST95HF.
 */

/** @addtogroup NFC_Dep
 * 	@{
 *	@brief  This part of the library is used to follow NFC DEP.
 */
 
 /** @addtogroup lib_nfcdeptarget_Private_Functions
 * 	@{
 */

/*********************************************/

/************* NFC A INIT FIELDS ***************/
extern uint8_t		UID_TypeA[];

static uint8_t	NFCDEP_ReceiveBuffer[PICCNFCDEP_MAX_NFCDEP_BUFFER_SIZE];
static uint8_t	NFCDEP_SendBuffer[PICCNFCDEP_MAX_NFCDEP_BUFFER_SIZE];

static PICCNFCDEP_TARGET 				 NFCDEPTarget; 
static PICCNFCDEP_PICC_INITIATOR Initiator;
static PICCNFCDEP_PICC_TARGET    Target;

static uint8_t UsedProtocol = 0;
static uint8_t DataPayloadOffset=0;

void (*NFCDepCallBack)(uc8*pData, uc8*nbByte);
/**
  * @}
  */ 

 /** @addtogroup lib_nfcdeptarget_Public_Functions
 * 	@{
 */

 /**
 * @brief  this function initiliazes the chip for exchange protocol
 * @param  Protocol: must be NFC-A or NFC-F
 * @param  LLCPModeSupported : indicate if caller support LLCP
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */ 
void PICCNFCDEP_Init ( uint8_t Protocol, bool LLCPModeSupported )
{	
	/* To select NFC-A or NFC-F */
	UsedProtocol = Protocol;
		
	if (Protocol == NFCDEP_ON_NFCA)
	{
		DataPayloadOffset = 2;
		
		NFCDEPTarget.TargetType = PICCNFCDEP_TARGET_TYPEA;
		memcpy( NFCDEPTarget.NfcId , UID_TypeA  , PICCNFCDEP_MAX_NFCIDI3_SIZE); 
	}
	else 
	{
		//Error!!!
	}
	
	memcpy( Target.NfcId3t, NFCDEPTarget.NfcId, PICCNFCDEP_MAX_NFCIDI3_SIZE);
	Target.Did = 0x00; /* value will be filled later with the one sent by target */
	Target.BSt = 0x00; /* active mode not supported by NFC forum so far */
	Target.BRt = 0x00; /* active mode not supported by NFC forum so far */
	Target.TO = 0x0E;  /* WT = 14 */
	Target.PPt = 0x20; /* LRt = 192B */

	Target.LLCPModeSupported = LLCPModeSupported;

	Target.PFB = 0x00; /* default value */
	
}


 /**
 * @brief  this function reads from the interface bus the command sent by the initiator and returns a response
 * @param  *pData : pointer on the receive data
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */
int8_t PICCNFCDEP_ReplyCommand( uc8 *pData )
{
	uint8_t	InsCode = pData[PICC_DATA_OFFSET];
	uint8_t nbByte = 0;
	
	InsCode = pData[PICC_DATA_OFFSET+DataPayloadOffset+PICCNFCDEP_CMD1_OFFSET];
	if( InsCode == PICCNFCDEP_COMMAND_REQ_CMD1 )
	{
		/* select instruction */
		InsCode = pData[PICC_DATA_OFFSET+DataPayloadOffset+PICCNFCDEP_CMD2_OFFSET];
		nbByte =  pData[PICC_LENGTH_OFFSET]-PICCNFCDEP_CRC_SIZE;
		pData += (PICC_DATA_OFFSET+DataPayloadOffset+PICCNFCDEP_PAYLOAD_OFFSET);
	}
	else
	{
		return PICCNFCDEP_ERRORCODE_COMMANDUNKNOWN;
	}
	
	switch (InsCode)
	{
		case PICCNFCDEP_COMMAND_ATR_REQ_CMD2:
			PICCNFCDEP_Atr ( pData, nbByte );
			break;
		
		case PICCNFCDEP_COMMAND_PSL_REQ_CMD2:
			PICCNFCDEP_Psl ( pData );
			break;
			
		case PICCNFCDEP_COMMAND_DEP_REQ_CMD2:
			PICCNFCDEP_Dep ( pData, nbByte );
			break;
		
		case PICCNFCDEP_COMMAND_DSL_REQ_CMD2:
			PICCNFCDEP_Dsl ( pData );
			break;
		
		case PICCNFCDEP_COMMAND_RLS_REQ_CMD2:
			PICCNFCDEP_Rls ( pData );
			break;
					
		default:
			return PICCNFCDEP_ERRORCODE_COMMANDUNKNOWN;
	}			

	return PICCNFCDEP_SUCESSSCODE;  

}

 /**
 * @brief  Response to receive ATR command
 * @param  *pData : pointer on the receive data
 * @param  nbByte : number of receive bytes
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */
int8_t PICCNFCDEP_Atr (uc8 *pData, u8 nbBytes)
{
	uint8_t 	Length =0,
						*pDataToToEmit = &(NFCDEP_SendBuffer [PICC_DATA_OFFSET]);
	int8_t 		status;
	/* debug purpose */
	uint8_t LLCP_MagicNumber[0x03]={0x46,0x66,0x6D}; /* LLCP v1.2 chapt 6.2.3 */

	u8 i=0;
	
	/* save Initiator attribute parameters */
	memcpy(Initiator.NfcId3i, &pData[0], PICCNFCDEP_MAX_NFCIDI3_SIZE);
	Initiator.Did = pData[PICCNFCDEP_MAX_NFCIDI3_SIZE];
	Initiator.BSi = pData[PICCNFCDEP_MAX_NFCIDI3_SIZE+1]; /* NFC Digital Protocol v1.0 active mode not supported BSi=0x00 */
	Initiator.BRi = pData[PICCNFCDEP_MAX_NFCIDI3_SIZE+2]; /* NFC Digital Protocol v1.0 active mode not supported BRi=0x00 */
	Initiator.PPi = pData[PICCNFCDEP_MAX_NFCIDI3_SIZE+3]; /* LRi is (Initiator.PPi)& 0x30 */
	
	/* Extract LRi */
	Initiator.LRi = ((Initiator.PPi) & PICCNFCDEP_LR_MASK)>>4;
	
	/* if General bytes available */
	if( (Initiator.PPi)& 0x02)
	{
		nbBytes -= PICCNFCDEP_MIN_BYTE_IN_ATR;
		for(i=0; i<nbBytes;i++)
		{	
			Initiator.Gi[i] = pData[0+PICCNFCDEP_MAX_NFCIDI3_SIZE+4+i];
		}
		
		/* check if LLCP is supported */
		if( memcmp( Initiator.Gi, LLCP_MagicNumber, 0x03) == 0 )
		{
			Initiator.LLCPModeSupported = true;
		}
	}
	
	/* creates the device response */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		pDataToToEmit [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		pDataToToEmit [Length++] = 0x00;
	}		
	
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_RES_CMD1;
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_ATR_RES_CMD2;
	
	memcpy(&pDataToToEmit[Length], Target.NfcId3t, PICCNFCDEP_MAX_NFCIDI3_SIZE);
	Length += PICCNFCDEP_MAX_NFCIDI3_SIZE;
	Target.Did = Initiator.Did; /* we must return the did parameter receive */
	pDataToToEmit [Length++] = Target.Did;	   		
	pDataToToEmit [Length++] = Target.BSt;	 
	pDataToToEmit [Length++] = Target.BRt;	 
	pDataToToEmit [Length++] = Target.TO;	  /* TB1 : timing */
	pDataToToEmit [Length++] = Target.PPt;	  /* TC1 : interface */
	
	/* Extract LRt */
	Target.LRt = ((Target.PPt) & PICCNFCDEP_LR_MASK)>>4;
	
	/* if we support LLCP (LLCP not implemented so far code here for test purpose) */
	if( Target.LLCPModeSupported == true && Initiator.LLCPModeSupported == true)
	{		
		Length -=1;
		pDataToToEmit [Length++] = Target.PPt+2; /* G parameters are present */
		
		/* Add magic Number */
		memcpy(&pDataToToEmit[Length] ,LLCP_MagicNumber,0x03);
		Length += 3;
		
		/* Add TLV version */
		pDataToToEmit[Length++]=0x01;
		pDataToToEmit[Length++]=0x01;
		pDataToToEmit[Length++]=0x11;
		
		/* Add WKS */
		pDataToToEmit[Length++]=0x03;
		pDataToToEmit[Length++]=0x02;
		pDataToToEmit[Length++]=0x00;
		pDataToToEmit[Length++]=0x08;
		
		/* Add LTO */
		pDataToToEmit[Length++]=0x07;
		pDataToToEmit[Length++]=0x01;
		pDataToToEmit[Length++]=0x03;
	}
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		pDataToToEmit [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	
	/* emits the response */
	errchk(PICC_Send (Length,pDataToToEmit));

	return PICCNFCDEP_SUCESSSCODE ;
Error:
	return PICCNFCDEP_ERRORCODE_ATRCOMMAND;

}

 /**
 * @brief  Response to receive PSL command
 * @param  *pData : pointer on the receive data
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */
int8_t PICCNFCDEP_Psl (uc8 *pData )
{
	uint8_t 	Length =0,
						*pDataToToEmit = &(NFCDEP_SendBuffer [PICC_DATA_OFFSET]);
	int8_t 		status;
	uint8_t 	ParametersByte;
	
	if( pData[0] != Initiator.Did )
		return PICCNFCDEP_ERRORCODE_BAD_DID_PARAM;
	
	Initiator.BRS = pData[1];
	
	Initiator.FSL = pData[2];
	
	if( Initiator.FSL > Initiator.LRi || Initiator.FSL > Target.LRt )
		return PICCNFCDEP_ERRORCODE_BAD_FSL_PARAM;
	
	/* creates the device response */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		pDataToToEmit [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		pDataToToEmit [Length++] = 0x00;
	}		
	
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_RES_CMD1;
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_PSL_RES_CMD2;
	
	pDataToToEmit [Length++] = Target.Did;	   		
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		pDataToToEmit [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	
	/* emits the response */
	errchk(PICC_Send (Length,pDataToToEmit));
	
	/* modify speed */
	/* if 212 */
	if (UsedProtocol == NFCDEP_ON_NFCA && Initiator.BRS == PICCNFCDEP_BRS_212 )
	{
		/* protocol setlect to switch to 212 */
		ParametersByte = 0x5A;
		errchk(PICC_ProtocolSelect(0x02,PICC_PROTOCOL_ISO14443A,&(ParametersByte),pDataToToEmit));
	}
	
	/* if 424 */
	if (Initiator.BRS == PICCNFCDEP_BRS_424 )
	{
		/* protocol setlect to switch to 424 */
		if( UsedProtocol == NFCDEP_ON_NFCA)
		{
			ParametersByte = 0xAA;
			errchk(PICC_ProtocolSelect(0x02,PICC_PROTOCOL_ISO14443A,&(ParametersByte),pDataToToEmit));
		}
	}

	return PICCNFCDEP_SUCESSSCODE ;
Error:
	return PICCNFCDEP_ERRORCODE_ATRCOMMAND;
}

 /**
 * @brief  Response to receive DEP command
 * @param  *pData : pointer on the receive data
 * @param  nbByte : number of receive bytes
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */
int8_t PICCNFCDEP_Dep (uc8 *pData, u8 nbByte )
{
	uint8_t 	Length =0,
						*pDataToToEmit = &(NFCDEP_SendBuffer [PICC_DATA_OFFSET]);
	int8_t 		status;
	
	Initiator.PFB = pData[0];
	Length++;
	
	/* Do we have optional DID and NAD bytes in the request */
	if( (Initiator.PFB) & PICCNFCDEP_PFB_DID_MASK)
	{
		Length++;
	}
	
	if( (Initiator.PFB) & PICCNFCDEP_PFB_NAD_MASK )
	{
		Initiator.NAD = pData[Length++];
	}
		
	/* Nb of transport data byte */
	nbByte -= Length;
		
	memcpy(NFCDEP_ReceiveBuffer, &pData[Length], nbByte);
	
	/* Here we should certainly send data to upper layer to retrieve a response */
	/* upper layer not implemented so far, NFCDEP_ReceiveBuffer will receive the response */ 
	PICCNFCDEP_CallBack(NFCDEP_ReceiveBuffer, &nbByte);
	//nbByte = 0;
	
	/* creates the device response */
	Length = 0;
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		pDataToToEmit [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		pDataToToEmit [Length++] = 0x00;
	}		
	
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_RES_CMD1;
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_DEP_RES_CMD2;
	
	pDataToToEmit [Length++] = Target.PFB;
	
	if( Initiator.Did != 0)
	{
		pDataToToEmit [Length++] = Target.Did;	   
	}
	
	/* If Initiator support NAD should not occurs Digital Protocol 1.0 */
	if( Initiator.PPi & 0x01 )
	{
		pDataToToEmit [Length++] = 0x00;	 /* This case must not happen*/  
	}
	
	/* no answer from upper layer for the moment */
	memcpy(&pDataToToEmit [Length], NFCDEP_ReceiveBuffer, nbByte);
	Length += nbByte;
			
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		pDataToToEmit [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	
	/* emits the response */
	errchk(PICC_Send (Length,pDataToToEmit));
	
	return PICCNFCDEP_SUCESSSCODE ;
Error:
	return PICCNFCDEP_ERRORCODE_ATRCOMMAND;	
}

 /**
 * @brief  Response to receive DSL command
 * @param  *pData pointer on the receive data
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */
int8_t PICCNFCDEP_Dsl (uc8 *pData )
{
	uint8_t 	Length =0,
						*pDataToToEmit = &(NFCDEP_SendBuffer [PICC_DATA_OFFSET]);
	int8_t 		status;
	
	/* creates the device response */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		pDataToToEmit [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		pDataToToEmit [Length++] = 0x00;
	}		
	
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_RES_CMD1;
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_DSL_RES_CMD2;
	
	if( Initiator.Did != 0)
	{
		pDataToToEmit [Length++] = Target.Did;	   
	}
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		pDataToToEmit [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	
	/* emits the response */
	errchk(PICC_Send (Length,pDataToToEmit));
	
	return PICCNFCDEP_SUCESSSCODE ;
Error:
	return PICCNFCDEP_ERRORCODE_ATRCOMMAND;	
}

 /**
 * @brief  Response to receive RLS command
 * @param  *pData pointer on the receive data
 * @retval 	PICCNFCDEP_SUCESSSCODE : if answer transmit
 * @retval 	PICCNFCDEP_ERRORCODE_ATRCOMMAND : if answer not transmit
 */
int8_t PICCNFCDEP_Rls (uc8 *pData )
{
	uint8_t 	Length =0,
						*pDataToToEmit = &(NFCDEP_SendBuffer [PICC_DATA_OFFSET]);
	int8_t 		status;
	
	/* creates the device response */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		pDataToToEmit [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		pDataToToEmit [Length++] = 0x00;
	}		
	
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_RES_CMD1;
	pDataToToEmit [Length++] = PICCNFCDEP_COMMAND_RLS_RES_CMD2;
	
	if( Initiator.Did != 0)
	{
		pDataToToEmit [Length++] = Target.Did;	   
	}
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		pDataToToEmit [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	
	/* emits the response */
	errchk(PICC_Send (Length,pDataToToEmit));
	
	return PICCNFCDEP_SUCESSSCODE ;
Error:
	return PICCNFCDEP_ERRORCODE_ATRCOMMAND;	
}


/** 
 * @brief This function is called when an initator send a command to the target. 
 * @brief Thanks to a function pointer the data are provided to the application. 
 * @param pData : Data receive from the target
 * @retval none  
 */
void PICCNFCDEP_CallBack(uc8 *pData, u8* nbByte)
{
	NFCDepCallBack((uint8_t*) pData, nbByte);
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

/**
  ******************************************************************************
  * @file    lib_nfcdepinitiator.c
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
#include "lib_nfcdepinitiator.h"


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
 
 /** @addtogroup lib_nfcdepinitiator_Private_Functions
 * 	@{
 */

/*********************************************/

/************* NFC A INIT FIELDS ***************/
extern uint8_t		UID_TypeA[];

/************* NFC F INIT FIELDS ***************/
// first 2 bytes of NFCID indicate if:
// (0x01,0xFE) 				--> NFC-DEP is supported 
// (0x02,0xFE) 				--> Tag Type 3, with bytes 3 to 8 of NFCID randomly generated
// (all other values)	--> Tag Type 3, with bytes 1 to 8 of NFCID proprietary
static uint8_t		InitNFcFId	[PCDNFCDEP_MAX_NFCIDI3_SIZE] = {0x01,0xFE,0x01,0x10,0x13,0x13,0x0D,0x00,0x00,0x00};

static uint8_t	NFCDEP_ReceiveBuffer[PCDNFCDEP_MAX_NFCDEP_BUFFER_SIZE];
static uint8_t	NFCDEP_SendBuffer[PCDNFCDEP_MAX_NFCDEP_BUFFER_SIZE];

static PCDNFCDEP_PCD_INITIATOR  Initiator;
static PCDNFCDEP_PCD_TARGET 		Target;

static uint8_t UsedProtocol = 0;
static uint8_t DataPayloadOffset=0;

static uint8_t TimeSlot = 0; /* force time slot to 0 */

/**
  * @}
  */ 

/** @addtogroup lib_nfcdepinitiator_Public_Functions
 * 	@{
 */

/**
 * @brief  this function initiliazes the chip for exchange protocol
 * @param  Protocol : To select between NFC-A or  NFC-F protocol
 * @param  LLCPModeSupported : To indicate if caller support LLCP
 * @retval 	None
 */
void PCDNFCDEP_Init ( uint8_t Protocol, bool LLCPModeSupported )
{	
	uint8_t LLCP_MagicNumber[0x03]={0x46,0x66,0x6D}; /* LLCP v1.2 chapt 6.2.3 */

	/* To select NFC-A or NFC-F */
	UsedProtocol = Protocol;
	
	if (Protocol == NFCDEP_ON_NFCA)
	{
		DataPayloadOffset = 2; 
		memcpy( Initiator.NfcId3i , UID_TypeA  , PCDNFCDEP_MAX_NFCIDI3_SIZE);
	}
	else if (Protocol == NFCDEP_ON_NFCF)
	{	
		DataPayloadOffset = 0;
		memcpy( Initiator.NfcId3i , InitNFcFId  , PCDNFCDEP_MAX_NFCIDI3_SIZE);
	}
	else 
	{
		//Error!!! /* NFCB not in NFC forum right now */
	}
	
	/* Information send with ATR_REQ */
	Initiator.Did = 0x00; /* DID parameter not supported right now */
	Initiator.BSi = 0x00; /* active mode not supported right now */
	Initiator.BRi = 0x00; /* active mode not supported right now */
	
	if( LLCPModeSupported == true)
	{
		Initiator.PPi = 0x22; /* LRi = 2 and general byte are present*/
		Initiator.LRi = 2; /* Max payload size is 192B */
		
		/* Add magic Number */
		Initiator.Gi[0]=LLCP_MagicNumber[0];
		Initiator.Gi[1]=LLCP_MagicNumber[1];
		Initiator.Gi[2]=LLCP_MagicNumber[2];
		
		/* Add TLV version */
		Initiator.Gi[3]=0x01;
		Initiator.Gi[4]=0x01;
		Initiator.Gi[5]=0x11;
		
		/* Add WKS */
		Initiator.Gi[6]=0x03;
		Initiator.Gi[7]=0x02;
		Initiator.Gi[8]=0x00;
		Initiator.Gi[9]=0x08;
		
		/* Add LTO */
		Initiator.Gi[10]=0x07;
		Initiator.Gi[11]=0x01;
		Initiator.Gi[12]=0x03;
		
		Initiator.GiSize = 13;
		
		Initiator.LLCPModeSupported = LLCPModeSupported;
	}
	else
	{
		Initiator.PPi = 0x20; /* LRi = 2 no general byte*/
		Initiator.LRi = 2; /* Max payload size is 192B */
		Initiator.GiSize = 0;  
		Initiator.LLCPModeSupported = LLCPModeSupported;
	}

	/* Information send with PSL_REQ */
	if (Protocol == NFCDEP_ON_NFCA)
		Initiator.BRS = 0x00; /* DSI DRI divisor by 1 */
	if (Protocol == NFCDEP_ON_NFCF)
		Initiator.BRS = 0x12; /* DSI DRI divisor by 4 */
	
	Initiator.FSL = 0x02; /* Max payload size is 192B */
	
	/* Information send with DEP_REQ */	
	Initiator.PFB = 0;	

	if (Protocol == NFCDEP_ON_NFCA)
		ISO14443A_Init( );	
	if (Protocol == NFCDEP_ON_NFCF)
		FELICA_Initialization();
}

 /**
 * @brief  this function check if NFC-A target is in the field
 * @param  none
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target present
 * @retval 	PCDNFCDEP_ERRORCODE_GENERIC : if target not present
 */
int8_t PCDNFCDEP_IsPresentNFCA( void)
{

	if(ISO14443A_IsPresent() == RESULTOK)
	{
		if(ISO14443A_Anticollision() == RESULTOK)
		{					
			if(PCDNFCDEP_Atr() == RESULTOK)
			{	
				return PCDNFCDEP_SUCESSSCODE;
			}
		}
	}

	return PCDNFCDEP_ERRORCODE_GENERIC;	
}

 /**
 * @brief  this function check if NFC-F target is in the field
 * @param  none
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target present
 * @retval 	PCDNFCDEP_ERRORCODE_GENERIC : if target not present
 */
int8_t PCDNFCDEP_IsPresentNFCF( void)
{

	if(FELICA_IsPresent() == RESULTOK )
	{
		if(PCDNFCDEP_Atr() == RESULTOK)
		{	
			return PCDNFCDEP_SUCESSSCODE;
		}
	}

	return PCDNFCDEP_ERRORCODE_GENERIC;		
}

 /**
 * @brief  Send ATR command
 * @param  none
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target answer properly
 * @retval 	PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN : if target baddely answer
 */
int8_t PCDNFCDEP_Atr (void)
{
	uint8_t 	Length =0, nbByteAns =0;
	uint8_t LLCP_MagicNumber[0x03]={0x46,0x66,0x6D}; /* LLCP v1.2 chapt 6.2.3 */
	int8_t 		status;

	u8 i=0;
	
	/************************************************************************/
	/* creates the request */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		NFCDEP_SendBuffer [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		NFCDEP_SendBuffer [Length++] = 0x00;
	}		
	
  /* ATR command */	
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_REQ_CMD1;
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_ATR_REQ_CMD2;
	
	/* ID */
	memcpy(&NFCDEP_SendBuffer[Length], Initiator.NfcId3i, PCDNFCDEP_MAX_NFCIDI3_SIZE);
	Length += PCDNFCDEP_MAX_NFCIDI3_SIZE;

	/* ATR parameters */
	NFCDEP_SendBuffer [Length++] = Initiator.Did;	   		
	NFCDEP_SendBuffer [Length++] = Initiator.BSi;	 
	NFCDEP_SendBuffer [Length++] = Initiator.BRi;	 
	NFCDEP_SendBuffer [Length++] = Initiator.PPi;	  /* TC1 : interface */
	
	/* if General bytes to send */
	for(i=0; i<Initiator.GiSize;i++)
	{	
		NFCDEP_SendBuffer [Length + i] = Initiator.Gi[i];
	}
	Length += Initiator.GiSize;
	
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		NFCDEP_SendBuffer [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		NFCDEP_SendBuffer [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	else
	{
		/* Add slot number */
		NFCDEP_SendBuffer [Length++] = TimeSlot;	 
	}
		
	// send the command to the target
	errchk(PCD_SendRecv(Length,NFCDEP_SendBuffer,NFCDEP_ReceiveBuffer))

	/************************************************************************/
	
	/* check the target answer */
	nbByteAns = NFCDEP_ReceiveBuffer[PCD_LENGTH_OFFSET]; 
	
	/* check response command*/	
	Length = RFTRANS_95HF_DATA_OFFSET+ DataPayloadOffset;
	if (NFCDEP_ReceiveBuffer[Length] == PCDNFCDEP_COMMAND_RES_CMD1 && NFCDEP_ReceiveBuffer[Length+1] == PCDNFCDEP_COMMAND_ATR_RES_CMD2)
	{
		/* check the response size is at list the minimum requiered */
		if( nbByteAns >= (PCDNFCDEP_MIN_BYTE_IN_ATR+DataPayloadOffset) )
		{
			Length += 2;
			memcpy(Target.NfcId3t, &NFCDEP_ReceiveBuffer[RFTRANS_95HF_DATA_OFFSET+2], PCDNFCDEP_MAX_NFCIDI3_SIZE);
			Length += PCDNFCDEP_MAX_NFCIDI3_SIZE;
			Target.Did = NFCDEP_ReceiveBuffer[Length++];
			Target.BSt = NFCDEP_ReceiveBuffer[Length++];
			Target.BRt = NFCDEP_ReceiveBuffer[Length++];
			Target.TO = NFCDEP_ReceiveBuffer[Length++];
			Target.PPt = NFCDEP_ReceiveBuffer[Length++];
			
			if( Target.PPt & 0x02)
			{
				nbByteAns -= (2 + PCDNFCDEP_MIN_BYTE_IN_ATR);
				for(i=0; i<nbByteAns; i++)
				{
					Target.Gt[i] = NFCDEP_ReceiveBuffer[Length+i];
				}
				Target.GtSize = nbByteAns;
				
				/* check if LLCP supported */
				if(memcmp(&Target.Gt[0], LLCP_MagicNumber ,3) == 0)
				{
					Target.LLCPModeSupported = true;
				}
			}
			return PCDNFCDEP_SUCESSSCODE;	
		}
		else
			return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	}
	else 
		return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	
Error : 
	return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	
}

 /**
 * @brief  Send PSL command
 * @param  BRS possibilities to force BRS for NFC-DEP A to upper value
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target answer properly
 * @retval 	PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN : if target baddely answer
 */
int8_t PCDNFCDEP_Psl ( PCDNFCDEP_BRS BRS )
{
	uint8_t 	Length =0;
	int8_t 		status;
	u8  ProtocolSelectParametersNFCA []  = {0x00, 0x01, 0xA0};
	u8  ProtocolSelectParametersNFCF []  = {0x51, 0x13, 0x01,0x0D};
	
	/************************************************************************/
	/* creates the request */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		NFCDEP_SendBuffer [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		NFCDEP_SendBuffer [Length++] = 0x00;
	}		
	
  /* PSL command */	
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_REQ_CMD1;
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_PSL_REQ_CMD2;
		
	/* PSL parameters */
	Initiator.BRS = BRS;
	NFCDEP_SendBuffer [Length++] = Initiator.Did;	   		
	NFCDEP_SendBuffer [Length++] = Initiator.BRS;	 
	NFCDEP_SendBuffer [Length++] = Initiator.FSL;	 
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		NFCDEP_SendBuffer [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		NFCDEP_SendBuffer [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	else
	{
		/* Add slot number */
		NFCDEP_SendBuffer [Length++] = TimeSlot;	 
	}
		
	// send the command to the RF transceiver
	errchk(PCD_SendRecv(Length,NFCDEP_SendBuffer,NFCDEP_ReceiveBuffer))
	/************************************************************************/
	
	/* Check the target answer */
	
	/* check response command */	
	Length = RFTRANS_95HF_DATA_OFFSET+ DataPayloadOffset;
	if (NFCDEP_ReceiveBuffer[Length] == PCDNFCDEP_COMMAND_RES_CMD1 && NFCDEP_ReceiveBuffer[Length+1] == PCDNFCDEP_COMMAND_PSL_RES_CMD2)
	{
		Length += 2;
	  if( NFCDEP_ReceiveBuffer[Length] == Initiator.Did)
		{
			/* modify speed if accepted by target */
			/* if 212 */
			if (UsedProtocol == NFCDEP_ON_NFCA && Initiator.BRS == BRS_212 )
			{
				/* protocol setlect to switch to 212 */
				ProtocolSelectParametersNFCA [0]  = 0x50;
				errchk(PCD_ProtocolSelect(0x04,PCD_PROTOCOL_ISO14443A,ProtocolSelectParametersNFCA,NFCDEP_SendBuffer));
			}
	
			/* if 424 */
			if (Initiator.BRS == BRS_424 )
			{
				/* protocol setlect to switch to 424 */
				if( UsedProtocol == NFCDEP_ON_NFCA)
				{
					ProtocolSelectParametersNFCA [0]  = 0xA0;
					errchk(PCD_ProtocolSelect(0x04,PCD_PROTOCOL_ISO14443A,ProtocolSelectParametersNFCA,NFCDEP_SendBuffer));
				}
				/* protocol setlect to switch to 424 */
				if( UsedProtocol == NFCDEP_ON_NFCF)
				{
					ProtocolSelectParametersNFCF [0]  = 0xA1;
					errchk(PCD_ProtocolSelect(0x05,PCD_PROTOCOL_FELICA,ProtocolSelectParametersNFCF,NFCDEP_SendBuffer));
				}
			}
			
			return PCDNFCDEP_SUCESSSCODE;	
		}	
		else
			return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	}
	else 
		return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	
Error : 
	return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
}

 /**
 * @brief  Send DEP command
 * @param  PFB
 * @param  *pData : Data to exchange with target
 * @param  nbByte : number of data byte to exchange
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target answer properly
 * @retval 	PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN : if target baddely answer
 */
int8_t PCDNFCDEP_Dep (uc8 PFB, u8 *pData, u8* nbByte )
{
	uint8_t 	Length =0;
	int8_t 		status;
	
	/************************************************************************/
	/* creates the request */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		NFCDEP_SendBuffer [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		NFCDEP_SendBuffer [Length++] = 0x00;
	}		
	
  /* DEP command */	
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_REQ_CMD1;
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_DEP_REQ_CMD2;
		
	/* DEP parameters */
	Initiator.PFB = PFB;
	NFCDEP_SendBuffer [Length++] = Initiator.PFB;
	//NFCDEP_SendBuffer [Length++] = Initiator.Did;	 /* Optional parameter */
	//NFCDEP_SendBuffer [Length++] = Initiator.NAD;	 /* Optional parameter */ 

	/* Fill the Transport Data Byte */
	memcpy( &NFCDEP_SendBuffer [Length], pData, *nbByte);
	Length += *nbByte;
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		NFCDEP_SendBuffer [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		NFCDEP_SendBuffer [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	else
	{
		/* Add slot number */
		NFCDEP_SendBuffer [Length++] = TimeSlot;	 
	}
		
	// send the command to the RF transceiver
	errchk(PCD_SendRecv(Length,NFCDEP_SendBuffer,NFCDEP_ReceiveBuffer))

	/************************************************************************/
	
	/* Check the target answer */
	*nbByte = NFCDEP_ReceiveBuffer[PCD_LENGTH_OFFSET]; 
	
	/* check response command */	
	Length = RFTRANS_95HF_DATA_OFFSET + DataPayloadOffset;
	if (NFCDEP_ReceiveBuffer[Length++] == PCDNFCDEP_COMMAND_RES_CMD1 && NFCDEP_ReceiveBuffer[Length++] == PCDNFCDEP_COMMAND_DEP_RES_CMD2)
	{
	  Target.PFB = NFCDEP_ReceiveBuffer[Length++];
		
		/* MUST not have DID or NAD parameter in answer as we don't send them in the request */
		*nbByte -= Length; 
		
		/* retrieve the transport Data byte */
		memcpy( pData, &NFCDEP_ReceiveBuffer[Length], *nbByte); 

		return PCDNFCDEP_SUCESSSCODE;	
	}
	else 
		return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	
Error : 
	return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
}

 /**
 * @brief  Send DSL command
 * @param  none
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target answer properly
 * @retval 	PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN : if target baddely answer
 */
int8_t PCDNFCDEP_Dsl ( void )
{
	uint8_t 	Length =0;
	int8_t 		status;
	
	/************************************************************************/
	/* creates the request */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		NFCDEP_SendBuffer [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		NFCDEP_SendBuffer [Length++] = 0x00;
	}		
	
  /* DSL command */	
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_REQ_CMD1;
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_DSL_REQ_CMD2;
		
	/* no DSL parameter as DID is optional */
	//NFCDEP_SendBuffer [Length++] = Initiator.Did;	 /* Optional parameter */
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		NFCDEP_SendBuffer [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		NFCDEP_SendBuffer [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	else
	{
		/* Add slot number */
		NFCDEP_SendBuffer [Length++] = TimeSlot;	 
	}
		
	// send the command to the RF transceiver
	errchk(PCD_SendRecv(Length,NFCDEP_SendBuffer,NFCDEP_ReceiveBuffer))

	/* Here we must start a timer to see if target answer in time */
	/* fgr not implemented */
	
	/************************************************************************/
	
	/* Check the target answer */
	
	/* check response command */	
	Length = RFTRANS_95HF_DATA_OFFSET + DataPayloadOffset;
	if (NFCDEP_ReceiveBuffer[Length] == PCDNFCDEP_COMMAND_RES_CMD1 && NFCDEP_ReceiveBuffer[Length+1] == PCDNFCDEP_COMMAND_DSL_RES_CMD2)
	{
		/* MUST not have DID parameter in answer as we don't send it in the request */
		return PCDNFCDEP_SUCESSSCODE;	
	}
	else 
		return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	
Error : 
	return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
}

 /**
 * @brief  Send RLS command
 * @param  none
 * @retval 	PCDNFCDEP_SUCESSSCODE : if target answer properly
 * @retval 	PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN : if target baddely answer
 */
int8_t PCDNFCDEP_Rls ( void )
{
	uint8_t 	Length =0;
	int8_t 		status;
	
	/************************************************************************/
	/* creates the request */
	if(UsedProtocol == NFCDEP_ON_NFCA)
	{
		/* Add SB byte prefix 0xF0 According to NFC-DEP */ 
		NFCDEP_SendBuffer [Length++] = 0xF0;
		/* reserve space, byte length According to NFC-DEP */ 		
		NFCDEP_SendBuffer [Length++] = 0x00;
	}		
	
  /* DSL command */	
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_REQ_CMD1;
	NFCDEP_SendBuffer [Length++] = PCDNFCDEP_COMMAND_RLS_REQ_CMD2;
		
	/* no DSL parameter as DID is optional */
	//NFCDEP_SendBuffer [Length++] = Initiator.Did;	 /* Optional parameter */
	
	if (UsedProtocol == NFCDEP_ON_NFCA)
	{
		NFCDEP_SendBuffer [1] = Length-2; /* -2 for SB byte and LEN */
		/* control byte append CRC + 8 bits */
		NFCDEP_SendBuffer [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	 
	}
	else
	{
		/* Add slot number */
		NFCDEP_SendBuffer [Length++] = TimeSlot;	 
	}
		
	// send the command to the RF transceiver
	errchk(PCD_SendRecv(Length,NFCDEP_SendBuffer,NFCDEP_ReceiveBuffer))

	/************************************************************************/
	
	/* Check the target answer */
	
	/* check response command */	
	Length = RFTRANS_95HF_DATA_OFFSET + DataPayloadOffset;
	if (NFCDEP_ReceiveBuffer[Length] == PCDNFCDEP_COMMAND_RES_CMD1 && NFCDEP_ReceiveBuffer[Length+1] == PCDNFCDEP_COMMAND_RLS_RES_CMD2)
	{
		/* MUST not have DID parameter in answer as we don't send it in the request */
		return PCDNFCDEP_SUCESSSCODE;	
	}
	else 
		return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
	
Error : 
	return PCDNFCDEP_ERRORCODE_COMMANDUNKNOWN;	
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

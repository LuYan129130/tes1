/**
  ******************************************************************************
  * @file    lib_iso14443Bpcd.c
  * @author  MMY Application Team
  * @version V4.0.0
  * @date    02/06/2014
  * @brief   Manage the iso14443B communication
  ******************************************************************************
  * @copy
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
#include "lib_iso14443Bpcd.h"

extern uint8_t													u95HFBuffer [RFTRANS_95HF_MAX_BUFFER_SIZE+3];

/* Variables for the different modes */
extern ST95Mode st95mode;
extern ST95TagType st95tagtype;

/* private variables 	-----------------------------------------------------------------*/
ISO14443B_CARD 	ISO14443B_Card;

static void ISO14443B_InitStructure							( void );
static void ISO14443B_CompleteStruture 					( uint8_t *pDataRead );
static int8_t ISO14443B_WriteARConfigB 					( void );
static int8_t ISO14443B_WriteAndCheckARConfigB 	( void );
static int8_t ISO14443BReadARConfigB 						( uint8_t *pDataRead );

/** @addtogroup _95HF_Libraries
 * 	@{
 *	@brief  <b>This is the library used by the whole 95HF family (RX95HF, CR95HF, ST95HF) <br />
 *				  You will find ISO libraries ( 14443A, 14443B, 15693, ...) for PICC and PCD <br />
 *				  The libraries selected in the project will depend of the application targetted <br />
 *				  and the product chosen (RX95HF emulate PICC, CR95HF emulate PCD, ST95HF can do both)</b>
 */

/** @addtogroup PCD
 * 	@{
 *	@brief  This part of the library enables PCD capabilities of CR95HF & ST95HF.
 */


 /** @addtogroup ISO14443B_pcd
 * 	@{
 *	@brief  This part of the library is used to follow ISO14443B.
 */


/** @addtogroup lib_iso14443Bpcd_Private_Functions
 *  @{
 */

/**
 * @brief  this functions intializes the ISO14443B data structure
 */
static void ISO14443B_InitStructure ( void )
{
	/* Initializes the data structure used to store results */
	memset(ISO14443B_Card.ATQB , 0x00, ISO14443B_ATQB_SIZE);
	memset(ISO14443B_Card.PUPI , 0x00, ISO14443B_MAX_PUPI_SIZE);
	ISO14443B_Card.IsDetected  = false;
	memset(ISO14443B_Card.LogMsg, 0x00, ISO14443B_MAX_LOG_MSG);
}


/**
 * @brief  this function complete the ISO14443B structure according to the REQB response
 * @param  *pDataRead	: Pointer to the PCD response
 */
static void ISO14443B_CompleteStruture ( uint8_t *pDataRead )
{

		memcpy(ISO14443B_Card.ATQB, &pDataRead[PCD_DATA_OFFSET],  ISO14443B_ATQB_SIZE);
		/* retrieves the PUPI field */
		memcpy (ISO14443B_Card.PUPI,						&(pDataRead[PCD_DATA_OFFSET + 1]),ISO14443B_MAX_PUPI_SIZE);
		/*  retrieves the Application data field */
		memcpy (ISO14443B_Card.ApplicationField,&(pDataRead[PCD_DATA_OFFSET + 1 + 0x04]),0x04);
		/*  retrieves the Application data field */
		memcpy (ISO14443B_Card.ProtocolInfo,		&(pDataRead[PCD_DATA_OFFSET + 1 + 0x04 + 0x04]),0x04);
}


/**
 * @brief  this function writes the  configuration registers according to AFE_SET_ANALOG_CONFIG command
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
static int8_t ISO14443B_WriteARConfigB ( void )
{
	uint8_t SetAnalogConfigParameter []	= { 0x01, 0x51	};
	int8_t	status;
	uint8_t *pDataRead = u95HFBuffer;

	SetAnalogConfigParameter [0] = (ISO14443B_ANALOGCONFIG & 0xFF00) >>8;
	SetAnalogConfigParameter [1] = ISO14443B_ANALOGCONFIG & 0x00FF;

	errchk(PCD_WriteRegister 			( 0x04,0x68,0x01,SetAnalogConfigParameter,pDataRead));

	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 
}

/**
 * @brief  this function reads the configuration registers according to AFE_SET_ANALOG_CONFIG command
 * @param  *pDataRead	: Pointer to the PCD response
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
static int8_t ISO14443BReadARConfigB (uint8_t *pDataRead )
{
	int8_t 	status;
	
	errchk(PCD_ReadRegister	( 0x03,0x68,0x02,0x01,pDataRead));

	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 


}

/**
 * @brief  this function reads ARConfigB registers and check if it matches the requested values(in AFE_SET_ANALOG_CONFIG command)
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
static int8_t ISO14443B_WriteAndCheckARConfigB ( void )
{
	uint8_t *pDataRead = u95HFBuffer;
	int8_t 	status;

	/* Set the Register index to ARConfigB index */
	errchk(ISO14443B_WriteARConfigB( )); 
	/* Read the register value */
	/* Set the Register index to ARConfigB index */
	errchk(ISO14443BReadARConfigB( pDataRead )); 
	/* Check ARConfig value */
	
	if(!( (pDataRead[PCD_DATA_OFFSET]== ( (ISO14443B_ANALOGCONFIG & 0xFF00)>>8)) && (pDataRead[PCD_DATA_OFFSET + 1 ] == ISO14443B_ANALOGCONFIG & 0x00FF )))
		return ISO14443B_SUCCESSCODE;
	else
		return ISO14443B_ERRORCODE_DEFAULT; 
	
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 	
}

/**
  * @}
  */ 


/** @addtogroup lib_iso14443Bpcd_Public_Functions
 *  @{
 */

/**
 * @brief  Initializes the the PCD device for the IS014443B protocol
 * @retval ISO14443B_SUCCESSCODE  : the PCD device is well configured.
 * @retval ISO14443B_ERRORCODE_DEFAULT : Communication issue.
 */
int8_t ISO14443B_Init( void )
{
	uint8_t		*pDataRead = u95HFBuffer;
	int8_t		status;
	
#ifdef USE_QJDDEVICE
uc8 ProtocolSelectParameters[] 	= { 
																		/* Parameters */		
																		PCD_ISO14443B_TRANSMISSION_SPEED_106K     |
																		PCD_ISO14443B_RECEPTION_SPEED_106K        |
																		PCD_ISO14443B_APPEND_CRC,
																		/* Set FDT */
																		0x02                         ,
																		0x00    																	
														};
uc8	Length = 0x04;
#else
uc8 ProtocolSelectParameters[] 	= { 
																		/* Parameters */		
																		PCD_ISO14443B_TRANSMISSION_SPEED_106K     |
																		PCD_ISO14443B_RECEPTION_SPEED_106K        |
																		PCD_ISO14443B_APPEND_CRC,	
																		/* PP MM bytes */
																		0x03,
																		0xFF
																		
														};
uc8	Length = 0x04;
#endif

	ISO14443B_InitStructure( );

	/* sends a protocol Select command to the pcd to configure it */
	errchk(PCD_ProtocolSelect(Length,PCD_PROTOCOL_ISO14443B,ProtocolSelectParameters,pDataRead));
	/* update the AR register */
	errchk(ISO14443B_WriteAndCheckARConfigB ());

	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT;
}

/**
 * @brief  this function emits a REQB command to a PICC device
 * @param  *pDataRead	: Pointer to the response
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
int8_t ISO14443B_ReqB ( uint8_t *pDataRead )
{
	int8_t	status;
	uc8 ReqB[]					= { 			/* APf */
															ISO14443B_ANTICOLLISION_PREFIX_BYTE   , 
															/* AFI */
															ISO14443B_AFI_ALL_FAMILIES            , 
															/* Parameters */
															ISO14443B_EXTENDED_ATQB_NOT_SUPPORTED |  
															ISO14443B_REQB_ATTEMPT                |
															ISO14443B_SLOT_MARKER_1 
														};

	/* sends the command to the PCD device*/
	errchk(PCD_SendRecv(0x03,ReqB,pDataRead));
	/* Filling of the data structure */
	ISO14443B_Card.IsDetected = true;
	/* complete the ISO 14443 type B strucure */
	ISO14443B_CompleteStruture (pDataRead);
		
	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 

}


/**
 * @brief  this function complete the ISO14443B structure according to the AttriB response
 * @param  *pDataRead	: Pointer to the PCD response
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
int8_t ISO14443B_AttriB( uint8_t *pDataRead )
{
	int8_t	status;
	uint8_t AttriB[ISO14443B_ATQB_SIZE] 			= { 
																				/* Start byte */ 
																				ATTRIB_FIRST_BYTE           ,
																				/* PUPI */ 
																				0x00                        ,  
																				/* PUPI */  
																				0x00                        ,
																				/* PUPI */   
																				0x00                        ,
																				/* PUPI */   
																				0x00                        ,
																				/* Parameter 1 */   
																				TR0_64_FS                   | 
																				TR1_64_FS                   |
																				EOF_REQUIRED                |
																				SOF_REQUIRED                ,
																				/* Parameter 2 */                          
																				MAX_FRAME_SIZE_256_BYTES    |		 
																				PCD_TO_PICC_106K            |
																				PICC_TO_PCD_106K            ,
																				/* Parameter 3 */              
																				TR2_32_FS                   |
																				PICC_COMPLIANT_ISO14443_4   ,
																				/* Parameter 4 */                          
																				CID_0	                     
														};
														
	/* copies the PUPI field */													
	memcpy(&AttriB[1], ISO14443B_Card.PUPI, ISO14443B_MAX_PUPI_SIZE);
	/* sends the command to the PCD device*/
	errchk(PCD_SendRecv(0x09,AttriB,pDataRead));
	/* checks the CRC */
	errchk(PCD_IsCRCOk (PCD_PROTOCOL_ISO14443B,pDataRead) );
		
	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 
}


/**
 * @brief  this function checks if a card is in the field
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
int8_t ISO14443B_IsPresent( void )
{
	int8_t	status;
	uint8_t *pDataRead = u95HFBuffer;
	
	/* Init the ISO14443 TypeB communication */
	errchk(ISO14443B_Init( ));

	delay_ms(5);
	/* WakeUp attempt */
	errchk(ISO14443B_ReqB(pDataRead));
	
	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 
}

/**
 * @brief Anticolision state machine for ISO14443B cards
 * @param void
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
int8_t ISO14443B_Anticollision(void)
{
	int8_t	status;
	uint8_t *pDataRead = u95HFBuffer;
	
	/* ATTRIB attempt */
	errchk(ISO14443B_AttriB(pDataRead));
	
	st95mode = PCD;
	st95tagtype = TT4B;
	
	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 
}

/**
 * @brief  this function checks if a card is still in the field
 * @retval ISO14443B_SUCCESSCODE  : the function is successful.
 * @retval ISO14443B_ERRORCODE_DEFAULT  : an error occured
 */
int8_t ISO14443B_IsCardIntheField ( void )
{
	uint8_t *pDataRead = u95HFBuffer;
	uc8			 Parameter			= 0xB2;
	int8_t	status;
	
	/* sends the command to the PCD device*/
	errchk(PCD_SendRecv(0x01,&Parameter,pDataRead));

	return ISO14443B_SUCCESSCODE;
Error:
	return ISO14443B_ERRORCODE_DEFAULT; 
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


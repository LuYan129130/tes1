/**
  ******************************************************************************
  * @file    lib_iso7816picc.c 
  * @author  MMY Application Team
  * @version V4.0.0
  * @date    02/06/2014
  * @brief   Manage the Iso/IEC 7816 communication in the PICC mode emulator.
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
#include "lib_iso7816picc.h"

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
 

 /** @addtogroup ISO7816_picc
 * 	@{
 *	@brief  This part of the library is used to follow ISO7816.
 */
 
extern bool					ISO14443A_TT4Used;
 /**
 * @brief  This buffer contains the data send/received by xx95HF
 */
extern uint8_t					u95HFBuffer [RFTRANS_95HF_MAX_BUFFER_SIZE+3];

/** @addtogroup lib_iso7816picc_Private_Functions
 * 	@{
 *	@brief  No private function
 */
 
 /**
  * @}
  */

/** @addtogroup lib_iso7816picc_Public_Functions
 * 	@{
 */

/**
 * @brief  this function decodes and returns a response to Deselectcommand
 * @param  pData : RF command received by the PICC
 */
void PICC7816_Deselect ( uc8 *pData )
{
	uint8_t Length =0,
				*pDataToToEmit = &(u95HFBuffer [PICC_DATA_OFFSET]);

	/* creates the card response  */
	pDataToToEmit [Length++] = COMMAND_SBLOCK;	    /* T1 : historical byte */

	/* control byte append CRC + 8 bits	*/
	if( ISO14443A_TT4Used == true )
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;	
		
	/* emits the response */ 
	PICC_Send (Length,pDataToToEmit);

}

/**
 * @brief  this function decodes and returns a response to Ack command
 * @param  pData : RF command received by the PICC
 */
void PICC7816_ACK ( uc8 *pData )
{
	uint8_t Length =0,
				*pDataToToEmit = &(u95HFBuffer [PICC_DATA_OFFSET]);

	/* creates the card response  */
	if ((pData[PICC_DATA_OFFSET] & 0x01) == 0)
	{
		pDataToToEmit [Length++] = COMMAND_ACKBLOCK_A2;	    
	}
	else 
	{	
		pDataToToEmit [Length++] = COMMAND_ACKBLOCK_A3;	 
	}

	/* control byte append CRC + 8 bits	*/
	if( ISO14443A_TT4Used == true )
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;
		
	/* emits the response */
	PICC_Send (Length,pDataToToEmit);

}

/**
 * @brief  This function send a status to PCD
 * @param  pData : RF command received by the PICC
 * @param  statusCode : Status that must be send 
 */
void PICC7816_SendAStatusCode ( uc8 *pData , uc16 statusCode )
{
	uint8_t Length =0,
					*pDataToToEmit = &(u95HFBuffer [PICC_DATA_OFFSET]);

	 	
	/* creates the card response  */
	/* add the IBklock */
	pDataToToEmit [Length++] = pData[PICC_DATA_OFFSET];
	/* add the command status */
	pDataToToEmit [Length++] = (uint8_t) GETMSB(statusCode);
	pDataToToEmit [Length++] = (uint8_t) GETLSB(statusCode);
	
	/* control byte append CRC + 8 bits	*/
	if( ISO14443A_TT4Used == true )
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;
	
	/* emits the card response */
	PICC_Send (Length,pDataToToEmit);
	
}

/**
 * @brief  This function send data to PCD
 * @param  pData : RF command received by the PICC
 * @param  pDataTosend : Data to send
 * @param  NbByte : number of byte to send
 * @param  statusCode : Status to provide to PCD
 */
void PICC7816_SendBuffer ( uc8 *pData ,uc8 *pDataTosend,uc8 NbByte, uc16 statusCode )
{
	uint8_t Length =0,
					*pDataToToEmit = &(u95HFBuffer [PICC_DATA_OFFSET]);

	 	
	/* creates the card response  */
	/* add the IBklock */
	pDataToToEmit [Length++] = pData[PICC_DATA_OFFSET]; /* USELESS */
	/* add the buffer */
	memcpy(&(pDataToToEmit [Length]),pDataTosend,NbByte);
	Length+=NbByte;
	/* add the command status */
	pDataToToEmit [Length++] = (uint8_t) GETMSB(statusCode);
	pDataToToEmit [Length++] = (uint8_t) GETLSB(statusCode);
	
	/* control byte append CRC + 8 bits	*/
	if( ISO14443A_TT4Used == true )
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;
	
	/* emits the card response */
	PICC_Send (Length,pDataToToEmit);
	
}


/**
 * @brief  This function decodes and returns a response to Nack command
 * @param  pData : RF command received by the PICC
 */
void PICC7816_SendANotAck ( uc8 *pData  )
{
	uint8_t Length =0,
				*pDataToToEmit = &(u95HFBuffer [PICC_DATA_OFFSET]);

	/* creates the card response  */
	if ( (pData[PICC_DATA_OFFSET] & 0x01) == 0x01 ) 
		pDataToToEmit [Length++] = COMMAND_NACKBLOCK_B2;
	else
		pDataToToEmit [Length++] = COMMAND_NACKBLOCK_B3;
	
	/* control byte append CRC + 8 bits	*/
	if( ISO14443A_TT4Used == true )
		pDataToToEmit [Length++] = SEND_MASK_APPENDCRC | SEND_MASK_8BITSINFIRSTBYTE;
	
	/* emits the card response */
	PICC_Send (Length,pDataToToEmit);
	
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

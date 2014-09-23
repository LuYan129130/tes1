/**
  ******************************************************************************
  * @file    lib_pcd.c 
  * @author  MMY Application Team
  * @version V4.0.0
  * @date    02/06/2014
  * @brief   This file provides set of firmware functions to manages the PCD device device. 
  * @brief   The commands as defined in the PCD device datasheet
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

/* Includes ------------------------------------------------------------------------------ */
#include "lib_PCD.h"

extern drv95HF_ConfigStruct							drv95HFConfig;
extern uint8_t													u95HFBuffer [RFTRANS_95HF_MAX_BUFFER_SIZE+3];

/* Variables for the different modes */
extern ST95Mode st95mode;
extern ST95TagType st95tagtype;

static uint8_t IsAnAvailableProtocol 		(uint8_t Protocol);


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


/** @addtogroup PCD_Device
 * 	@{
 *	@brief  This set of function use the device commands to control both CR95HF & ST95HF <br />
 *					This layer is the bridge beetween the library and the 95HF driver. 
 */


/** @addtogroup lib_PCD_Private_Functions
 *  @{
 */

#ifdef USE_CR95HF_DEVICE
static uint32_t PCD_ComputeUARTBaudRate (uc8 BaudRateCommandParameter);

/**
 *	@brief  This function computes the UART baud rate according to Baudrate command parameter
 *  @param  BaudRateCommandParameter  : Baudrate command parameter (1 byte)
 *  @retval the UART baud rate value
 */
static uint32_t PCD_ComputeUARTBaudRate (uc8 BaudRateCommandParameter)
{
	return (13.56e6/ (2*BaudRateCommandParameter+2));
}

#endif /* USE_CR95HF_DEVICE */

/**
 *	@brief  this functions returns PCD_SUCCESSCODE if the protocol is available, otherwise PCD_ERRORCODE_PARAMETER
 *  @param  Protocol : RF protocol (ISO 14443 A or 14443 B or 15 693 or Fellica)
 *  @return PCD_SUCCESSCODE	: the protocol is available
 *  @return PCD_ERRORCODE_PARAMETER : the protocol isn't available
 */
static uint8_t IsAnAvailableProtocol (uint8_t Protocol) 
{
	switch(Protocol)
	{
		case PCD_PROTOCOL_FIELDOFF:
			return PCD_SUCCESSCODE;
		case PCD_PROTOCOL_ISO15693:
			return PCD_SUCCESSCODE;
		case PCD_PROTOCOL_ISO14443A:
			return PCD_SUCCESSCODE;
		case PCD_PROTOCOL_ISO14443B:
			return PCD_SUCCESSCODE;
		case PCD_PROTOCOL_FELICA:
			return PCD_SUCCESSCODE;
		default: return PCD_ERRORCODE_PARAMETER;
	}	
}
 
/**
  * @}
  */ 


/** @addtogroup lib_PCD_Public_Functions
 *  @{
 */


#ifdef USE_POLL_FIELD
/**
 *	@brief  this function send an POLL_FIELD command to the PCD device device
 *  @param  pResponse : pointer on the PCD device reply
 *  @retval PCD_SUCCESSCODE : the function is succesful 
 */
int8_t PCD_POLL_FIELD(uint8_t *pResponse)
{
uc8 DataToSend[] = {POLL_FIELD	,0x00};
	
	drv95HF_SendReceive(DataToSend, pResponse);

return PCD_SUCCESSCODE;

}
#endif /*USE_POLL_FIELD*/

/**
 *	@brief  this function send an EOF pulse to a contacless tag
 *  @param  pResponse : pointer on the PCD device reply
 *  @retval PCD_SUCCESSCODE : the function is succesful 
 */
int8_t PCD_SendEOF(uint8_t *pResponse)
{
uc8 DataToSend[] = {SEND_RECEIVE	,0x00};

	drv95HF_SendReceive(DataToSend, pResponse);

return PCD_SUCCESSCODE;
}



/**
 *	@brief  this functions turns the field off
 *  @param  none
 *  @retval PCD_SUCCESSCODE : the function is succesful 
 */
void PCD_FieldOff( void )
{
	uint8_t ParametersByte=0x00;
	uint8_t pResponse[10];

	PCD_ProtocolSelect(0x02, PCD_PROTOCOL_FIELDOFF, &ParametersByte,pResponse);

}

/**
 * @brief  this functions turns the field on
 * @param  none
 * @return none
 */
void PCD_FieldOn ( void )
{
	uint8_t ParametersByte=0x00;
	uint8_t pResponse[10];

	PCD_ProtocolSelect(0x02, PCD_PROTOCOL_ISO15693, &ParametersByte,pResponse);

}

/**
 *	@brief  this function send Echo command to the PCD device 
 *  @param  pResponse : pointer on the PCD device reply
 *  @return PCD_SUCCESSCODE 
 */
int8_t PCD_Echo(uint8_t *pResponse)
{
uc8 command[]= {ECHO};

	drv95HF_SendReceive(command, pResponse);

return PCD_SUCCESSCODE;

}

/**
 *	@brief  this function send a ProtocolSeclect command to the PCD device
 *  @param  Length  : number of byte of protocol select command parameters
 *  @param  Protocol : RF protocol (ISO 14443 A or B or 15 693 or Fellica)
 *  @param  Parameters: prococol parameters (see reader datasheet)
 *  @param  pResponse : pointer on the PCD device response
 *  @return PCD_SUCCESSCODE : the command was succedfully send
 *  @return PCD_ERRORCODE_PARAMETERLENGTH : the Length parameter is erroneous
 *  @return PCD_ERRORCODE_PARAMETER : a parameter is erroneous
 */
int8_t PCD_ProtocolSelect(uc8 Length,uc8 Protocol,uc8 *Parameters,uint8_t *pResponse)
{
	uint8_t DataToSend[SELECT_BUFFER_SIZE];
	int8_t	status; 
	
	/* check ready to receive Protocol select command */
	PCD_Echo(u95HFBuffer);
	
	if (u95HFBuffer[0] != ECHORESPONSE)
	{
		/* reset the device */
		PCD_PORsequence( );
	}

		/* initialize the result code to 0xFF and length to in case of error 	*/
	*pResponse = PCD_ERRORCODE_DEFAULT;
	*(pResponse+1) = 0x00;
	
	/* check the function parameters */
	errchk(IsAnAvailableProtocol(Protocol));


	DataToSend[PCD_COMMAND_OFFSET ] = PROTOCOL_SELECT;
	DataToSend[PCD_LENGTH_OFFSET  ]	= Length;
	DataToSend[PCD_DATA_OFFSET    ]	= Protocol;

	/* DataToSend CodeCmd Length Data	*/
	/* Parameters[0] first byte to emmit	*/
	memcpy(&(DataToSend[PCD_DATA_OFFSET +1]),Parameters,Length-1);
	
	/* Send the command the Rf transceiver	*/
 	drv95HF_SendReceive(DataToSend, pResponse);

	return PCD_SUCCESSCODE;

Error:
	return PCD_ERRORCODE_PARAMETER;	
}


/**
 *	@brief  this function send a SendRecv command to the PCD device. the command to contacless device is embeded in Parameters.
 *  @param  Length 		: Number of bytes
 *  @param	Parameters 	: data depenps on protocl selected
 *  @param  pResponse : pointer on the PCD device response
 *  @return PCD_SUCCESSCODE : the command was succedfully send
 *  @return PCD_ERRORCODE_DEFAULT : the PCD device returned an error code
 *  @return PCD_ERRORCODE_PARAMETERLENGTH : Length parameter is erroneous
 */
int8_t PCD_SendRecv(uc8 Length,uc8 *Parameters,uint8_t *pResponse)
{
	uint8_t DataToSend[SENDRECV_BUFFER_SIZE];

	/*initialize the result code to 0xFF and length to 0 */
	*pResponse = PCD_ERRORCODE_DEFAULT;
	*(pResponse+1) = 0x00;
	

	/* check the function parameters	*/
	if (CHECKVAL (Length,1,255)==false)
		return PCD_ERRORCODE_PARAMETERLENGTH; 

	DataToSend[PCD_COMMAND_OFFSET ] = SEND_RECEIVE;
	DataToSend[PCD_LENGTH_OFFSET  ]	= Length;

	/* DataToSend CodeCmd Length Data*/
	/* Parameters[0] first byte to emmit	*/
	memcpy(&(DataToSend[PCD_DATA_OFFSET]),Parameters,Length);

	/* Send the command the Rf transceiver	*/
	drv95HF_SendReceive(DataToSend, pResponse);


	if (PCD_IsReaderResultCodeOk (SEND_RECEIVE,pResponse) != PCD_SUCCESSCODE)
		return PCD_ERRORCODE_DEFAULT;

	return PCD_SUCCESSCODE;
}

/**
 *	@brief  this function send a Idle command to the PCD device
 *  @param  Length 	: Number of bytes
 *  @param	Data	: pointer on data. Data depends on protocl selected
 *  @param  pResponse : pointer on the PCD device response
 *  @return PCD_SUCCESSCODE : the command was succedfully send
 *  @return PCD_ERRORCODE_DEFAULT : the PCD device returned an error code
 *  @return PCD_ERRORCODE_PARAMETERLENGTH : Length parameter is erroneous
 */
int8_t PCD_Idle( uc8 Length, uc8 *Data )
{
	uint8_t DataToSend[IDLE_BUFFER_SIZE];

	/* check the function parameters	*/
	if (Length != 0x0E)
		return PCD_ERRORCODE_PARAMETERLENGTH; 

	DataToSend[PCD_COMMAND_OFFSET ] = IDLE;
	DataToSend[PCD_LENGTH_OFFSET  ]	= Length;

	
	memcpy(&(DataToSend[PCD_DATA_OFFSET]),Data,Length );

	drv95HF_SendSPICommand(DataToSend);

	return PCD_SUCCESSCODE;	
}

/**
 *	@brief  this function send a RdReg command to the PCD device 
 *  @param  Length 		: Number of bytes
 *  @param	Address		: address of first register to read
 *  @param	RegCount	: number of register to read
 *  @param	Flags		: whether to increment address after register read
 *  @param  pResponse : pointer on the PCD device response
 *  @return PCD_SUCCESSCODE : the command was succedfully send
 *  @return PCD_ERRORCODE_DEFAULT : the PCD device returned an error code
 *  @return PCD_ERRORCODE_PARAMETERLENGTH : Length parameter is erroneous
 */
int8_t PCD_ReadRegister (uc8 Length,uc8 Address,uc8 RegCount,uc8 Flags,uint8_t *pResponse)
{
	uint8_t DataToSend[RDREG_BUFFER_SIZE];


	DataToSend[PCD_COMMAND_OFFSET ] = READ_REGISTER;
	DataToSend[PCD_LENGTH_OFFSET  ]	= Length;
	DataToSend[PCD_DATA_OFFSET  ]	= Address;
	DataToSend[PCD_DATA_OFFSET +1 ]	= RegCount;
	DataToSend[PCD_DATA_OFFSET +2 ]	= Flags;
	/* Send the command the Rf transceiver	*/
	drv95HF_SendReceive(DataToSend, pResponse);
	
	if (PCD_IsReaderResultCodeOk (IDLE,pResponse) != PCD_SUCCESSCODE)
		return PCD_ERRORCODE_DEFAULT;

	return PCD_SUCCESSCODE;

}

/**
 *	@brief  this function send a WriteRegister command to the PCD device
 *  @param  Length 		: Number of bytes of WrReg parameters
 *  @param	Address		: address of first register to write
 *  @param	pData 		: pointer data to be write
 *  @param	Flags		: whether to increment address after register read
 *  @param  pResponse : pointer on the PCD device response
 *  @return PCD_SUCCESSCODE : the command was succedfully send
 *  @return PCD_ERRORCODE_PARAMETERLENGTH : Length parameter is erroneous
 */
int8_t PCD_WriteRegister (uc8 Length,uc8 Address,uc8 Flags,uc8 *pData,uint8_t *pResponse)
{
	uint8_t DataToSend[WRREG_BUFFER_SIZE];

	DataToSend[PCD_COMMAND_OFFSET ] = WRITE_REGISTER;
	DataToSend[PCD_LENGTH_OFFSET  ]	= Length;
	DataToSend[PCD_DATA_OFFSET  ]	= Address;
	DataToSend[PCD_DATA_OFFSET +1 ]	= Flags;

	/* Parameters[0] first byte to emmit	*/
	memcpy(&(DataToSend[PCD_DATA_OFFSET + 2]),pData,Length - 2 );
	
	/* Send the command the Rf transceiver	*/
	drv95HF_SendReceive(DataToSend, pResponse);
	return PCD_SUCCESSCODE;
}


/**
 *	@brief  This function sends POR sequence. It can be use to initialize the PCD device after a POR.
 *  @param  none
 *  @return PCD_ERRORCODE_PORERROR : the POR sequence doesn't succeded
 *  @return PCD_SUCCESSCODE : the RF transceiver is ready
 */
int8_t PCD_PORsequence( void )
{
	uint8_t NthAttempt=0;
	
	if(drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_SPI)
	{
		drv95HF_ResetSPI();		
	}	
	
	do{
	
		/* send an ECHO command and checks the PCD device response */
		PCD_Echo(u95HFBuffer);
		if (u95HFBuffer[0]==ECHORESPONSE)
			return PCD_SUCCESSCODE;	

		/* if the SPI interface is selected then send a reset command*/
		if(drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_SPI)
		{	
			drv95HF_ResetSPI();				
		}
		/* if the UART interface is selected then send 255 ECHO commands*/
		else if(drv95HFConfig.uInterface == RFTRANS_95HF_INTERFACE_UART)
		{
			do {
				PCD_Echo(u95HFBuffer);
				if (u95HFBuffer[0] == ECHORESPONSE)
					return PCD_SUCCESSCODE;	
			}while(NthAttempt++ < RFTRANS_95HF_MAX_BUFFER_SIZE);
		}
		
	} while (u95HFBuffer[0]!=ECHORESPONSE && NthAttempt++ <5);

return PCD_ERRORCODE_PORERROR;
}

/**
*	 @brief  this function send a command to the PCD device device over SPI or UART bus and receive its response.
*	 @brief  the returned value is PCD_SUCCESSCODE 	
*  @param  *pCommand  : pointer on the buffer to send to the the PCD device ( Command | Length | Data)
*  @param  *pResponse : pointer on the the PCD device response ( Command | Length | Data)
*  @return PCD_SUCCESSCODE : the the PCD device returns an success code
*  @return PCD_ERRORCODE_DEFAULT : the the PCD device returns an error code
 */
int8_t PCD_CheckSendReceive(uc8 *pCommand, uint8_t *pResponse) 
{
	
	drv95HF_SendReceive(pCommand, pResponse);

	if (PCD_IsReaderResultCodeOk (SEND_RECEIVE,pResponse) != PCD_SUCCESSCODE)
		return PCD_ERRORCODE_DEFAULT;

	return PCD_SUCCESSCODE;
}

/**
 *	@brief  this function returns the inferface bus choosed 
 *  @param  *pResponse : pointer on the the PCD device response
 *  @param  : 0x00 : status ok
 *  @param  : 0x01 : length
 *  @param  : 0x0X : with X = 0 => UART Or X = 0 => SPI Or X = 2 => TWI
 *  @retval 
 */
void PCD_GetInterfaceBus (uint8_t *pResponse)
{
   	*pResponse 		= 	SUCCESFUL_COMMAND_0x80;
		*(pResponse+1) 	= 0x01;
		*(pResponse+2) 	= drv95HF_GetInterfacePinState ();
}


/**  
* @brief  	this function returns PCD_SUCCESSCODE is the reader reply is a succesful code.
* @param  	CmdCode		:  	code command send to the reader
* @param  	ReaderReply	:  	pointer on the PCD device response	
* @retval  	PCD_SUCCESSCODE :  the CRC is Ok
* @retval  	PCD_NOREPLY_CODE : the CRC is erroenous
*/
int8_t PCD_IsCRCOk (uc8 Protocol ,uc8 *pReaderReply)
{
uint8_t NbByte = pReaderReply[PCD_LENGTH_OFFSET];
	
	
 	switch (Protocol)
	{
		case PCD_PROTOCOL_ISO15693: 
			if ( (pReaderReply [PCD_DATA_OFFSET + NbByte - CONTROL_15693_NBBYTE] & CONTROL_15693_CRCMASK) == CONTROL_15693_CRCMASK )
			{
				return PCD_ERRORCODE_DEFAULT;
			}
			else 
			{	
				return PCD_SUCCESSCODE;
			}
			
		case PCD_PROTOCOL_ISO14443A: 
			if ( (pReaderReply [PCD_DATA_OFFSET + NbByte - ISO14443A_NBBYTE] & ISO14443A_CRCMASK) == ISO14443A_CRCMASK )
			{
				return PCD_ERRORCODE_DEFAULT;
			}
			else 
			{	
				return PCD_SUCCESSCODE;
			}
			
		case PCD_PROTOCOL_ISO14443B: 
			if ( (pReaderReply [PCD_DATA_OFFSET + NbByte - CONTROL_14443B_NBBYTE] & CONTROL_14443B_CRCMASK) == CONTROL_14443B_CRCMASK )
			{
				return PCD_ERRORCODE_DEFAULT;
			}
			else 
			{	
				return PCD_SUCCESSCODE;
			}
					
		case PCD_PROTOCOL_FELICA: 
				if ( (pReaderReply [PCD_DATA_OFFSET + NbByte - CONTROL_FELICA_NBBYTE] & CONTROL_FELICA_CRCMASK) == CONTROL_FELICA_CRCMASK )
			{
				return PCD_ERRORCODE_DEFAULT;
			}
			else 
			{	
				return PCD_SUCCESSCODE;
			}
			
		default: 
			return ERRORCODE_GENERIC;
	}
}

/**  
* @brief  	this function returns PCD_SUCCESSCODE is the reader reply is a succesful code.
* @param  	CmdCode		:  	code command send to the reader
* @param  	ReaderReply	:  	pointer on the PCD device response	
* @retval  	PCD_SUCCESSCODE :  the PCD device returned a succesful code
* @retval  	PCD_ERRORCODE_DEFAULT  :  the PCD device didn't return a succesful code
* @retval  	PCD_NOREPLY_CODE : no the PCD device response
*/
int8_t PCD_IsReaderResultCodeOk (uint8_t CmdCode,uc8 *ReaderReply)
{

	if (ReaderReply[READERREPLY_STATUSOFFSET] == PCD_ERRORCODE_DEFAULT)
		return PCD_NOREPLY_CODE;

  	switch (CmdCode)
	{
		case ECHO: 
			if (ReaderReply[PSEUDOREPLY_OFFSET] == ECHO)
				return PCD_SUCCESSCODE;
			else 
				return PCD_ERRORCODE_DEFAULT;
		case IDN: 
			if (ReaderReply[READERREPLY_STATUSOFFSET] == IDN_RESULTSCODE_OK)
				return PCD_SUCCESSCODE;
			else 
				return PCD_ERRORCODE_DEFAULT;
		case PROTOCOL_SELECT: 
			switch (ReaderReply[READERREPLY_STATUSOFFSET])
			{
				case IDN_RESULTSCODE_OK :
					return PCD_SUCCESSCODE;
				case PROTOCOLSELECT_ERRORCODE_CMDLENGTH :
					return PCD_ERRORCODE_DEFAULT;
				case PROTOCOLSELECT_ERRORCODE_INVALID :
					return PCD_ERRORCODE_DEFAULT;
				default : return PCD_ERRORCODE_DEFAULT;
			}
		case SEND_RECEIVE: 
			switch (ReaderReply[READERREPLY_STATUSOFFSET])
			{
				case SENDRECV_RESULTSCODE_OK :
					if (ReaderReply[READERREPLY_STATUSOFFSET+1] != 0)
						return PCD_SUCCESSCODE;
					else
						return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_RESULTSRESIDUAL :
					return PCD_SUCCESSCODE;
				case SENDRECV_ERRORCODE_COMERROR :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_FRAMEWAIT :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_SOF :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_OVERFLOW :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_FRAMING :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_EGT :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_LENGTH :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_CRC :
					return PCD_ERRORCODE_DEFAULT;
				case SENDRECV_ERRORCODE_RECEPTIONLOST :
					return PCD_ERRORCODE_DEFAULT;
				default :
					return PCD_ERRORCODE_DEFAULT;
			}			
		case IDLE: 
			switch (ReaderReply[READERREPLY_STATUSOFFSET])
			{
				case IDLE_RESULTSCODE_OK :
					return PCD_SUCCESSCODE;
				case IDLE_ERRORCODE_LENGTH :
					return PCD_ERRORCODE_DEFAULT;
				default : return PCD_ERRORCODE_DEFAULT;
			}
		case READ_REGISTER: 
			switch (ReaderReply[READERREPLY_STATUSOFFSET])
			{
				case READREG_RESULTSCODE_OK :
					return PCD_SUCCESSCODE;
				case READREG_ERRORCODE_LENGTH :
					return PCD_ERRORCODE_DEFAULT;
				default : return PCD_ERRORCODE_DEFAULT;
			}
		case WRITE_REGISTER: 
			switch (ReaderReply[READERREPLY_STATUSOFFSET])
			{
				case WRITEREG_RESULTSCODE_OK :
					return PCD_SUCCESSCODE;
				default : return PCD_ERRORCODE_DEFAULT;
			}
		case BAUD_RATE: 
			return PCD_ERRORCODE_DEFAULT;
		default: 
			return ERRORCODE_GENERIC;
	}
}

#ifdef USE_CR95HF_DEVICE
/**
 *	@brief  this function send a BaudRate command to the PCD device
 *  @param	BaudRate	: new baudrate
 *  @param	pResponse 	: pseudo reply (shall be 0x55)
 *  @return PCD_SUCCESSCODE : the command was succedfully send
 */
int8_t PCD_BaudRate (uc8 BaudRate,uint8_t *pResponse)
{
	uint8_t DataToSend[BAUDRATE_BUFFER_SIZE];

	DataToSend[PCD_COMMAND_OFFSET ] = BAUD_RATE;
	DataToSend[PCD_LENGTH_OFFSET  ]	= BAUDRATE_LENGTH;
	DataToSend[PCD_DATA_OFFSET  ]	= BaudRate;
	/* Send the command the Rf transceiver	*/
	drv95HF_SendReceive(DataToSend, pResponse);
	return PCD_SUCCESSCODE;
}

/**
 *	@brief  This function run a process to change UARt baud rate. It'll check the baud rate is
 *	@brief   compatible with this MCU and configurates MCU and the PCD device baud rate.
 *  @param  *pCommand  : pointer on the buffer to send to the the PCD device ( Command | Length | Data)
 *  @param  *pResponse : pointer on the the PCD device response ( Command | Length | Data)
 *  @retval  PCD_SUCCESSCODE : the UART baud rate has been succesfuly changed 
 *  @retval  PCD_ERRORCODE_UARTDATARATEUNCHANGED : the UART baud rate has not changed 
 *  @retval  PCD_ERRORCODE_UARTDATARATEPROCESS : command DaubRate was send to the PCD device but the MCU was not able to comunicatewith the PCD device
 */
int8_t PCD_ChangeUARTBaudRate (uc8 *pCommand, uint8_t *pResponse)
{
	/* initialize the response	*/
	*pResponse =PCD_ERRORCODE_DEFAULT;
	*(pResponse+1) = 0x00;

	/* if(PCD_Interface == SPI)	*/
	if(drv95HFConfig.uInterface != RFTRANS_95HF_INTERFACE_UART)
	{	*pResponse =PCD_ERRORCODE_UARTDATARATEUNCHANGED;
		return PCD_ERRORCODE_UARTDATARATEUNCHANGED; 
	}

// 	/* some UART data rates are not compatible between MCU and the PCD device	*/
// 	if (PCD_IsUartMCUAndCR95HFAvailable(pCommand) != PCD_SUCCESSCODE)
// 	{	*pResponse =PCD_ERRORCODE_UARTDATARATEUNCHANGED;
// 		return PCD_ERRORCODE_UARTDATARATEUNCHANGED; 
// 	}

	/* send baud rate command to the PCD device*/
	drv95HF_SendReceive(pCommand,pResponse);

	/* udpate UART config of MCU	*/
	drv95HF_InitializeUART (PCD_ComputeUARTBaudRate (pCommand[PCD_DATA_OFFSET]));

  /* check if the MCU can communicate with the PCD device	*/
	PCD_Echo(pResponse);
	if (pResponse[ECHOREPLY_OFFSET]!=0x55)
	{	*pResponse =PCD_ERRORCODE_UARTDATARATEPROCESS;
		return PCD_ERRORCODE_UARTDATARATEPROCESS; 
	}
	/* the baud rate has been succesful modified*/
	*pResponse =PCD_SUCCESSCODE;
	
	return PCD_SUCCESSCODE; 
}

#endif /* USE_CR95HF_DEVICE */

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


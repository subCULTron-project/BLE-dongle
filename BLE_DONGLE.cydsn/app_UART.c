/*******************************************************************************
 * File Name: app_UART.c
 *
 * Description:
 *  Common BLE application code for client devices.
 *
 *******************************************************************************
 * Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
 * You may use this file only in accordance with the license, terms, conditions,
 * disclaimers, and limitations in the end user license agreement accompanying
 * the software package with which this file was provided.
 *******************************************************************************/

#include "app_UART.h"
#include "main.h"


/*******************************************************************************
 * Function Name: HandleUartRxTraffic
 ********************************************************************************
 *
 * Summary:
 *  This function takes data from received notfications and pushes it to the 
 *  UART TX buffer. 
 *
 * Parameters:
 *  CYBLE_GATTC_HANDLE_VALUE_NTF_PARAM_T * - the notification parameter as  
 *                                           recieved by the BLE stack
 *
 * Return:
 *   None.
 *
 *******************************************************************************/
void HandleUartRxTraffic(CYBLE_GATTC_HANDLE_VALUE_NTF_PARAM_T *uartRxDataNotification)
{
    if(uartRxDataNotification->handleValPair.attrHandle == txCharHandle)
    {
        UART_SpiUartPutArray(uartRxDataNotification->handleValPair.value.val, \
			     (uint32) uartRxDataNotification->handleValPair.value.len);
    }
}


/*******************************************************************************
 * Function Name: HandleUartTxTraffic
 ********************************************************************************
 *
 * Summary:
 *  This function takes data from UART RX buffer and pushes it to the server 
 *  as Write Without Response command.
 *
 * Parameters:
 *  None.
 *
 * Return:
 *   None.
 *
 *******************************************************************************/
void HandleUartTxTraffic(void)
{
    uint8   index;
    uint8   uartTxData[mtuSize - 3];
    uint16  uartTxDataLength;
    
    static uint16 uartIdleCount = UART_IDLE_TIMEOUT;
    
    CYBLE_API_RESULT_T              bleApiResult;
    CYBLE_GATTC_WRITE_CMD_REQ_T     uartTxDataWriteCmd;

    //UART_UartPutString("+");
    
    if (mode == SINGLE)
	uartTxDataLength = UART_SpiUartGetRxBufferSize();
    else
	uartTxDataLength = commandLength - commandIdx;

    
#ifdef FLOW_CONTROL
    if(uartTxDataLength >= (UART_UART_RX_BUFFER_SIZE - (UART_UART_RX_BUFFER_SIZE/2)))
    {
	DisableUartRxInt();
    }
    else
    {
	EnableUartRxInt();
    }
#endif
    
    if((uartTxDataLength != 0))
    {
        if(uartTxDataLength >= (mtuSize - 3))
        {
            uartIdleCount       = UART_IDLE_TIMEOUT;
            uartTxDataLength    = mtuSize - 3;
        }
        else
        {
            if(--uartIdleCount == 0)
            {
                /*uartTxDataLength remains unchanged */;
            }
            else
            {
                uartTxDataLength = 0;
            }
        }
        
        if(0 != uartTxDataLength)
        {
            uartIdleCount       = UART_IDLE_TIMEOUT;

	    // fill data structure "uartTxDataWriteCmd" with data entered by user on UART
            for(index = 0; index < uartTxDataLength; index++)
            {
		if (mode == SINGLE)
		{
		    uartTxData[index] = (uint8) UART_UartGetByte();
		}
		else
		{
		    uartTxData[index] = command[commandIdx];
            UART_UartPutChar(command[commandIdx]);
		    commandIdx++;
		}
            }
            
            uartTxDataWriteCmd.attrHandle = rxCharHandle;
            uartTxDataWriteCmd.value.len  = uartTxDataLength;
            uartTxDataWriteCmd.value.val  = uartTxData;           
            
#ifdef FLOW_CONTROL
	    DisableUartRxInt();
#endif

	    // feed BLE with data stored in structure "uartTxDataWriteCmd"
            do
            {
                bleApiResult = CyBle_GattcWriteWithoutResponse(cyBle_connHandle, &uartTxDataWriteCmd);
                CyBle_ProcessEvents();
            }
            while((CYBLE_ERROR_OK != bleApiResult) && (CYBLE_STATE_CONNECTED == cyBle_state));	   
        }

	// in multi or group mode, reached the end of command ?
	if (mode == GROUP)
	{
	    // finished sending command ? 
	    if (commandIdx == commandLength)
	    {
                    // wait for a while
         UART_UartPutString("Command transmitted to ");              
		 UART_UartPutChar(deviceName[0]); 
		 UART_UartPutChar(deviceName[1]); 
		 UART_UartPutChar(deviceName[2]); 
		 UART_UartPutChar(deviceName[3]); 
		 UART_UartPutChar(deviceName[4]); 
		 UART_UartPutString("\n"); 

        
        int i;
        for (i = 0; i < 10; i++)
        {
            CyDelay(50);
            CyBle_ProcessEvents();
        }
        
       deviceDisconnected();     
            
		done[currentId] = 1;
		commandIdx = 0;
		int nextId = getNextId();

		// reached the end of ids list ? return to prompt
		if (nextId == -1)
		{
		    mainPromptActive = 1;
		    resetIdStatus();
		}
		// or just cycle to next id
		else
		{
		    setCurrentId(nextId);
		}
		
        
		// disconnect current device
		CyBle_GapDisconnect(cyBle_connHandle.bdHandle);
        
	    }
	}
    }
}

/*****************************************************************************************
 * Function Name: DisableUartRxInt
 ******************************************************************************************
 *
 * Summary:
 *  This function disables the UART RX interrupt.
 *
 * Parameters:
 *   None.
 *
 * Return:
 *   None.
 *
 *****************************************************************************************/
void DisableUartRxInt(void)
{
    UART_INTR_RX_MASK_REG &= ~UART_RX_INTR_MASK;  
}

/*****************************************************************************************
 * Function Name: EnableUartRxInt
 ******************************************************************************************
 *
 * Summary:
 *  This function enables the UART RX interrupt.
 *
 * Parameters:
 *   None.
 *
 * Return:
 *   None.
 *
 *****************************************************************************************/
void EnableUartRxInt(void)
{
    UART_INTR_RX_MASK_REG |= UART_RX_INTR_MASK;  
}

/* [] END OF FILE */

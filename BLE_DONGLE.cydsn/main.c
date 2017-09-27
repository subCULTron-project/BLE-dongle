/*******************************************************************************
* File Name: main.c
*
* Version: 1.0
*
* Description:
*  BLE example project that works as a BLE to UART bridge, using 
*  Cypress's BLE component APIs and application layer callback. 
*  This project demostrates a custom service usage for BLE to UART  
*  bridge in a CENTRAL role.
*
* References:
*  BLUETOOTH SPECIFICATION Version 4.1
*
* Hardware Dependency:
*  (1) CY8CKIT-042 BLE
*  (2) An external UART transciever (if flow control is enabled) 
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "main.h"
#include "app_Ble.h"
#include <stdio.h>

volatile Mode mode;
         unsigned char ids[256];
         unsigned char done[256];
volatile int numIds;
volatile int currentId;
         char command[512];
volatile int commandLength;
volatile int commandIdx;
volatile int mainPromptActive = 1;
         char deviceName[5];
volatile int ready = 0;        

int getNextId()
{
    int i = 0;
    for (i = 0; i < numIds; i++)
    {
	if (done[i] == 0)
	{
	    return i;
	}
    }
    return -1;
}

int findId (int label)
{
        int i;
    for (i = 0; i < numIds; i++)
    {        
	if (ids[i] == label)
	{
	    if (done[i] == 0)
	    {
		return i;
	    }
	    else
	    {
		return 0;
	    }
	}
    }
    return 0;    
}

void resetIdStatus()
{
    int i;
    for (i = 0; i < numIds; i++)
    {
	done[i] = 0;
    }
    setCurrentId(0);
}
    
void setCurrentId (int i)
{
    currentId = i;
    int label = ids[i];
    deviceName[4] = '0' + label % 10;
    label /= 10;
    deviceName[3] = '0' + label % 10;
    label /= 10;
    deviceName[2] = '0' + label % 10;
    ready = 0;
}
        
void resetState()
{
    currentId = 0;
    numIds = 0;
    mode = UNDEFINED;
    commandIdx = 0;
    commandLength = 0;
    mainPromptActive = 1;
    
    UART_Stop();
    UART_SpiUartClearTxBuffer();
    UART_SpiUartClearRxBuffer();
    UART_Start();
    
    CyBle_Stop();
    CyBle_Start(AppCallBack);
}

CY_ISR(vUserPin_IRQ)
{
    resetState();
}

void sendIntToUART(uint16 num)
{
    char string[30];
    sprintf(string, "%d", num);
    UART_UartPutString(string);
}

int main()
{
    
    #ifdef LOW_POWER_MODE    
        CYBLE_LP_MODE_T         lpMode;
        CYBLE_BLESS_STATE_T     blessState;
    #endif
    
    
    CyGlobalIntDisable;
    
    // start interrupt handling from UserPIN
    isr_UserPin_StartEx(vUserPin_IRQ);
    isr_UserPin_ClearPending();
    
    CyGlobalIntEnable; 
    
    /* Start UART and BLE component and display project information */
    UART_Start();   

    resetState();

    
    /***************************************************************************
    * Main polling loop
    ***************************************************************************/

    // init device name
    deviceName[0] = 'a';
    deviceName[1] = 'M';
   
    while(1)
    {                  
        UserLED_Write(!mainPromptActive);
        
        if(mainPromptActive)
	{
	    // Ask user to set operating mode
	    if (mode == UNDEFINED)
	    {
		UART_UartPutString("Need to chat with robots? Another day in paradise!\n");
		UART_UartPutString("\n");
		UART_UartPutString("\n");
		UART_UartPutString("---------------------------------------------------------\n");
		UART_UartPutString("Transmission mode ('s'=single, 'g'=group) : ");
		
		while (mode == UNDEFINED)
		{
		    char ch;
		    ch = UART_UartGetChar();
		    
		    switch (ch)
		    {
		    case 's' :
			mode = SINGLE;
			break;
		    case 'g' :
			mode = GROUP;
			break;
		    }		
		}

		// echo user choice
		if (mode == SINGLE) UART_UartPutString("SINGLE\n");
		if (mode == GROUP) UART_UartPutString("GROUP\n");
	    }

	    // Ask user to define ids to talk to
	    // only if max one id already defined,
	    // otherwise cycle through the previously defined list
	    if (numIds <= 1)
	    {	    
		UART_UartPutString("---------------------------------------------------------\n");
		UART_UartPutString("Enter robot IDs (finish with 'backslash_n' or 'backslash_r') : ");

		int id = 0;
		int decoding = 0;
		numIds = 0;

		while(1)
		{
		    unsigned char ch;
		    ch = UART_UartGetChar();
		    
		    // got a digit
		    if (ch >= '0' && ch <= '9')
		    {		    
			id *= 10;
			id += ch - '0';
			
			// flag to tell an id is being read
			decoding = 1;
		    }
		    
		    // end of the list, record last id if needed
		    else if ((ch == '\n' || ch == '\r') && (numIds > 0 || decoding))
		    {
			if (decoding && id > 0 && id < 256)
			{			    
			    ids[numIds] = id;
			    numIds++;
			    
			    // echo recorded id
			    sendIntToUART(id);
			}
			UART_UartPutChar('\n');
			break;
		    }
		    
		    // any non digit is the end of current decoded id
		    else if (ch != 0)
		    {
			if (id > 0 && id < 256)
			{
			    ids[numIds] = id;
			    numIds++;
                
			    // echo recorded id
			    sendIntToUART(id);
			    UART_UartPutChar(' ');		    			    
			}
			id = 0;
			decoding = 0;			
		    }
		} 

		resetIdStatus();

		UART_UartPutChar('\n');
	    }

	    // Prompt for command to send to robots
	    // in single mode, chars will be directly read from UART
	    if (mode == SINGLE)
	    {
		UART_UartPutString("---------------------------------------------------------\n");
	    }
	    if (mode == GROUP)
	    {
		UART_UartPutString("\n---------------------------------------------------------\n");
		UART_UartPutString("Enter character string to send (finish with 'backslash_n' or 'backslash_r') : \n");

		commandIdx = 0;
		while (1)
		{
		    char ch;
		    ch = UART_UartGetChar();

		    if (ch == '\n' || ch == '\r')
			break;
		    
		    if (ch != 0)
		    {
			command[commandIdx] = ch;
			commandIdx++;			
		    }
		}
		command[commandIdx] = '\0';
		commandLength = commandIdx;
		commandIdx = 0;
		
		UART_UartPutString("Ok, will transmit following command : ");
		UART_UartPutString(command);
		UART_UartPutChar('\n');
				
		UART_UartPutString("---------------------------------------------------------\n");	
        
		// deviceDisconnected();
		CyBle_Stop();
		CyBle_Start(AppCallBack);

	    }
	    
	    mainPromptActive = 0;
        }
        /*******************************************************************
        *  Process all pending BLE events in the stack
        *******************************************************************/      
        HandleBleProcessing();
        CyBle_ProcessEvents();        
    }
}




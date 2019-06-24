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

uint32  UartRxDataSim;  // Character Input Received from the UART Terminal for Initiating Connection / Disconnection
uint8 devIndex;         // Index of the devices detected in Scanning
uint8 Periph_Selected;  // The Index of the Peropheral which the user wants to connect the Central 
uint8 IsSelected;       // Whether user has given Periph_Selected
uint8 IsConnected;      // Whether the Central is in connected state or not.
uint8 IsUserReset = 0;          // checking whether user reset button was pressed
CYBLE_CONN_HANDLE_T			connHandle; // Handle of the device connected.
CYBLE_API_RESULT_T 		    apiResult;



CY_ISR(vUserPin_IRQ)
{
    if (IsConnected) 
    {
        CyBle_GapDisconnect(connHandle.bdHandle);
    }
    else
    {
        CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
    }
    IsConnected = 0;
    IsSelected = 0;
    IsUserReset = 1;
    //devIndex= 0;
}

void print_help() 
{
    UART_UartPutString ("\n"); 
    UART_UartPutString ("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    UART_UartPutString ("* Commands to connect to aMussels:\n");
    UART_UartPutString ("*\th - print this help\n");
    UART_UartPutString ("*\n");
    UART_UartPutString ("*\tl - list all seen aMussels\n");
    UART_UartPutString ("*\n");
    UART_UartPutString ("*\ta - pick a specific aMussel. After character 'a' send a ID byte of the aMussel.\n");
    UART_UartPutString ("*\n");
    UART_UartPutString ("*\tn - pick a specific aMussel from the list of all seen aMussels.\n");
    UART_UartPutString ("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
}


int main()
{
    #ifdef LOW_POWER_MODE    
        CYBLE_LP_MODE_T         lpMode;
        CYBLE_BLESS_STATE_T     blessState;
    #endif
    
    CYBLE_API_RESULT_T      bleApiResult;
    
    int i;
    uint8_t ch;
    char buffer[30];
    
    CyGlobalIntDisable;
    
    // start interrupt handling from UserPIN
    isr_UserPin_StartEx(vUserPin_IRQ);
    isr_UserPin_ClearPending();
    
    CyGlobalIntEnable; 
    
    /* Start UART and BLE component and display project information */
    UART_Start();   
    bleApiResult = CyBle_Start(AppCallBack); 
    
    /* initialize default BLE peripheral name */
    BlName[0] = 'a'; BlName[1] = 'M';

    if(bleApiResult != CYBLE_ERROR_OK)
    {
        #ifdef PRINT_MESSAGE_LOG   
            UART_UartPutString("\n\r\t\tCyBle stack initilization FAILED!!! \n\r ");
        #endif
        
        /* Enter infinite loop */
        while(1);
    }
    
    // print help
    print_help();
    
    /***************************************************************************
    * Main polling loop
    ***************************************************************************/
    while(1)
    {                  
        
        CyBle_ProcessEvents();
        
        if(IsUserReset) 
        {
            IsUserReset = 0;
            UART_UartPutString ("\nCurrent device list:\n");
            if(devIndex == 0) 
            {
                UART_UartPutString ("\tEmpty!\n");
            }
            for(i = 0; i < devIndex; i++) 
            {
                sprintf(buffer,"\tDevice No: %d\tName: %s\r\n", i, scannedPeriphDevice[i].name);
        		UART_UartPutString(buffer);
            }
        }
        
        if(!IsSelected)
        {
            if(UART_SpiUartGetRxBufferSize())
    		{
                UartRxDataSim = UART_UartGetChar();

                switch(UartRxDataSim) 
                {
                    case 'l': 
                    {
                        UART_UartPutString ("\nCurrent device list:\n");
                        if(devIndex == 0) 
                        {
                            UART_UartPutString ("\tEmpty!\n");
                        }
                        for(i = 0; i < devIndex; i++) 
                        {
                            
                            sprintf(buffer,"\tDevice No: %d\tName: %s\r\n", i, scannedPeriphDevice[i].name);
                    		UART_UartPutString(buffer);
                        }
                        break;
                    }
                    case 'h': 
                    {
                        print_help();
                        break;
                    }
                    case 'a': 
                    {
                        // wait for next character
                        while(!UART_SpiUartGetRxBufferSize());
                        
                        UartRxDataSim = UART_UartGetChar();
                        
                        // check if aMussel is in the table
                        char name[6] = {'a','M','0','0','0','\0'};
                        
                        name[2] = UartRxDataSim / 100 + '0';
                        name[3] = (UartRxDataSim % 100) / 10 + '0';
                        name[4] = (UartRxDataSim % 10) + '0';
                        
                        int ind = 0, found = 0;
                        
                        for(i = 0; i < devIndex; i++) 
                        {
                            if(!strcmp(name, (char*)scannedPeriphDevice[i].name)) 
                            {
                                ind = i;
                                found = 1;
                                break;
                            }
                        }
                        
                        if(found == 1) 
                        {
                            Periph_Selected = i;
                            IsSelected = 1;
                            //Stop the scanning before connecting to the preferred peripheral
                            if(CyBle_GetState() != CYBLE_STATE_CONNECTING) 
                            {
                                CyBle_GapcStopScan();
                            }
                            sprintf(buffer,"Connecting to aMussel %s\r\n", name);
                            UART_UartPutString(buffer);
                        }
                        else 
                        {
                            sprintf(buffer,"aMussel %s not found in the list! Try again!\r\n",name);
                            UART_UartPutString(buffer);
                        }
                        break;
                    }
                    case 'n': 
                    {
                        // wait for next character
                        while(!UART_SpiUartGetRxBufferSize());
                        
                        UartRxDataSim = UART_UartGetChar();
                        
                        // Check if a Valid Device index has been received
                        if (UartRxDataSim < devIndex)
                        {
                            Periph_Selected = (uint8)(UartRxDataSim);
                            IsSelected = 1;
                            //Stop the scanning before connecting to the preferred peripheral
                            if(CyBle_GetState() != CYBLE_STATE_CONNECTING) 
                            {
                                CyBle_GapcStopScan();
                            }
                            
                            sprintf(buffer,"Connecting to aMussel %s\r\n", scannedPeriphDevice[Periph_Selected].name);
                            UART_UartPutString(buffer);
                        }
                        break;
                    }
                    default: 
                    {
                        UART_UartPutString("Invalid command!\r\n");
                        break;
                    }
                }
    		}
        }
        
        /*******************************************************************
        *  Process all pending BLE events in the stack
        *******************************************************************/      
        HandleBleProcessing();
    }
}




/* [] END OF FILE */

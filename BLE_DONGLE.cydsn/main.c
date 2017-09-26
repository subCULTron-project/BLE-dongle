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

uint8_t bootloaderStart = 0;

CY_ISR(vUserPin_IRQ) {

    bootloaderStart = 0;
    CyBle_Stop();
    CyBle_Start(AppCallBack);
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
    
    CyGlobalIntDisable;
    
    // start interrupt handling from UserPIN
    isr_UserPin_StartEx(vUserPin_IRQ);
    isr_UserPin_ClearPending();
    
    CyGlobalIntEnable; 
    
    /* Start UART and BLE component and display project information */
    UART_Start();   
    bleApiResult = CyBle_Start(AppCallBack); 
    
    /* initialize default BLE peripheral name */
    BlName[0] = 'a'; BlName[1] = 'M'; BlName[2] = '0'; BlName[3] = '0'; BlName[4] = '1';
    
    if(bleApiResult == CYBLE_ERROR_OK)
    {
        #ifdef PRINT_MESSAGE_LOG
            UART_UartPutString("\n\r************************************************************");
            UART_UartPutString("\n\r***************** BLE UART example project *****************");
            UART_UartPutString("\n\r************************************************************\n\r");
            UART_UartPutString("\n\rDevice role \t: CENTRAL");
            
            #ifdef LOW_POWER_MODE
                UART_UartPutString("\n\rLow Power Mode \t: ENABLED");
            #else
                UART_UartPutString("\n\rLow Power Mode \t: DISABLED");
            #endif
            
            #ifdef FLOW_CONTROL
                UART_UartPutString("\n\rFlow Control \t: ENABLED");  
            #else
                UART_UartPutString("\n\rFlow Control \t: DISABLED");
            #endif
            
        #endif
    }
    else
    {
        #ifdef PRINT_MESSAGE_LOG   
            UART_UartPutString("\n\r\t\tCyBle stack initilization FAILED!!! \n\r ");
        #endif
        
        /* Enter infinite loop */
        while(1);
    }
    
    //CyBle_ProcessEvents();
    
    /***************************************************************************
    * Main polling loop
    ***************************************************************************/
    while(1)
    {                  
        UserLED_Write(bootloaderStart);
        
        if(bootloaderStart == 0) {
            
            UART_UartPutString("\n Enter aMussel number [000 - 999]: ");
            
            for(i=0;i<3;i++) {
                do {
                    ch = UART_UartGetChar();
                } while(ch == 0);
                
                if(ch >= '0' && ch <= '9') {
                    BlName[i+2] = ch;
                }
                else {
                    UART_UartPutString("\n Index out of range. \n Repeat number sequence [000 - 999]");
                    i = 0;
                }
                
                if(i == 2) 
                    bootloaderStart = 1;
            }
            for(i=0; i<5; i++){
                UART_UartPutChar(BlName[i]);
            }
        }
        /*******************************************************************
        *  Process all pending BLE events in the stack
        *******************************************************************/      
        HandleBleProcessing();
        CyBle_ProcessEvents();
        
    }
}




/* [] END OF FILE */

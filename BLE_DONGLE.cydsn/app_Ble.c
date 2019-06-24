/*******************************************************************************
* File Name: app_Ble.c
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

#include "app_Ble.h"

uint8 devIndex;
uint8 Periph_Selected;

/* 'connHandle' is a varibale of type 'CYBLE_CONN_HANDLE_T' (defined in 
* BLE_StackGatt.h) and is used to store the connection handle parameters after
* connecting with the peripheral device. */
CYBLE_CONN_HANDLE_T			connHandle;

	/* 'apiResult' is a varibale of type 'CYBLE_API_RESULT_T' (defined in 
* BLE_StackTypes.h) and is used to store the return value from BLE APIs. */
	
CYBLE_API_RESULT_T 		    apiResult;
	
/* 'connectPeriphDevice' is a varibale of type 'CYBLE_GAP_BD_ADDR_T' (defined in 
* BLE_StackGap.h) and is used to store address of the connected device. */
//CYBLE_GAP_BD_ADDR_T     connectPeriphDevice[MAX_BLE_DEVICE_NUM];




////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////OLD///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


uint16 txCharHandle             = 0;                /* Handle for the TX data characteristic */
uint16 rxCharHandle             = 0;                /* Handle for the RX data characteristic */
uint16 txCharDescHandle         = 0;                /* Handle for the TX data characteristic descriptor */
uint16 bleUartServiceHandle     = 0;                /* Handle for the BLE UART service */
uint16 bleUartServiceEndHandle  = 0;                /* End handle for the BLE UART service */
uint16 mtuSize                  = CYBLE_GATT_MTU;   /* MTU size to be used by Client and Server after MTU exchange */

const uint8 enableNotificationParam[2] = {0x01, 0x00};

volatile static bool notificationEnabled     = false;

static INFO_EXCHANGE_STATE_T    infoExchangeState   = INFO_EXCHANGE_START;

CYBLE_GATT_ATTR_HANDLE_RANGE_T  attrHandleRange;
CYBLE_GATTC_FIND_INFO_REQ_T     charDescHandleRange;

/* UUID of the custom BLE UART service */
const uint8 bleUartServiceUuid[16]    = {
                                            0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9,0xe0, \
                                            0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e \
                                        };


/* UUID of the TX attribute of the custom BLE UART service */
const uint8 uartTxAttrUuid[16]    = {
                                            0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9,0xe0, \
                                            0x93, 0xf3, 0xa3, 0xb5, 0x03, 0x00, 0x40, 0x6e \
                                        };

/* UUID of the RX attribute of the custom BLE UART service */
const uint8 uartRxAttrUuid[16]    = {
                                            0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9,0xe0, \
                                            0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e \
                                        };

/* UUID of the custom BLE UART service */
const uint8 bleUartServiceUuid_old[16]    = {
                                            0x31, 0x01, 0x9b, 0x5f, 0x80, 0x00, 0x00,0x80, \
                                            0x00, 0x10, 0x00, 0x00, 0xd0, 0xcd, 0x03, 0x00 \
                                        };

/* UUID of the TX attribute of the custom BLE UART service */
const uint8 uartTxAttrUuid_old[16]        = {
                                            0x31, 0x01, 0x9b, 0x5f, 0x80, 0x00, 0x00,0x80, \
                                            0x00, 0x10, 0x00, 0x00, 0xd1, 0xcd, 0x03, 0x00 \
                                        };

/* UUID of the RX attribute of the custom BLE UART service */
const uint8 uartRxAttrUuid_old[16]        = {
                                            0x31, 0x01, 0x9b, 0x5f, 0x80, 0x00, 0x00,0x80, \
                                            0x00, 0x10, 0x00, 0x00, 0xd2, 0xcd, 0x03, 0x00 \
};

/* structure to be passed for discovering service by UUID */
const CYBLE_GATT_VALUE_T    bleUartServiceUuidInfo = { 
                                                        (uint8 *) bleUartServiceUuid, \
                                                        CYBLE_GATT_128_BIT_UUID_SIZE,\
                                                        CYBLE_GATT_128_BIT_UUID_SIZE \
                                                      };

/* structure to be passed for the enable notification request */
CYBLE_GATTC_WRITE_REQ_T     enableNotificationReqParam   = {
                                                                {(uint8*)enableNotificationParam, 2, 2},
                                                                0
                                                            };


/*******************************************************************************
* Function Name: Get_Adv_Scan_Packets
********************************************************************************
*
* Summary:
* This function gets information from Advertisement and Scan Response packets
*
* Parameters:  
* scanReport - Advertisement report received by the Central
*
* Return: 
*  None
*
*******************************************************************************/

void Get_Adv_Scan_Packets(CYBLE_GAPC_ADV_REPORT_T* scanReport)
{
 uint8 RepIndex; //Index for Bytes in Scan Response Packet
    {
        char buffer[40], name[20];
        
        if (devIndex < MAX_BLE_DEVICE_NUM)
        { 
            // check if event type is scan report
            if ((scanReport->eventType == CYBLE_GAPC_CONN_UNDIRECTED_ADV)||
                (scanReport->eventType == CYBLE_GAPC_CONN_DIRECTED_ADV)||
                (scanReport->eventType == CYBLE_GAPC_SCAN_UNDIRECTED_ADV)||
                (scanReport->eventType == CYBLE_GAPC_NON_CONN_UNDIRECTED_ADV)||
                (scanReport->eventType == CYBLE_GAPC_SCAN_RSP) )
            {
                // indicator of next parameter length
                int ind = 0, msg_id, success_flag = 0b00;

                // loop until all data is looked through
                while(success_flag != 0b11) 
                {
                    // get msg id
                    msg_id = scanReport->data[ind+1];
                    switch(msg_id) 
                    {
                        case 0x09:
                            // check if name larger than 2 characters
                            if(scanReport->data[ind] > 2 + 1) 
                            {
                                if(scanReport->data[ind+2] == BlName[0] && scanReport->data[ind+3] == BlName[1]) 
                                {
                                    // copy name for printing
                                    //int i;
                                    //for(i = 0; i < scanReport->data[ind] - 1; i++) {
                                    //    name[i] = scanReport->data[ind+2+i];
                                    //}
                                    memcpy(name, &(scanReport->data[ind+2]), scanReport->data[ind] - 1);
                                    name[scanReport->data[ind] - 1] = '\0';
                                    
                                    success_flag = success_flag | 0b01;
                                }
                            }
                            break;
                        
                        case 0xFF:
                            // check if data not 5 bytes
                            if(scanReport->data[ind] == 5 &&(scanReport->data[ind+1] == 0xff) && \
                              (scanReport->data[ind+2] == 0x31) && (scanReport->data[ind+3] == 0x01) && \
                              (scanReport->data[ind+4] == 0x3b) && (scanReport->data[ind+5] == 0x04) ) 
                            {
                                success_flag = success_flag | 0b10;
                            }
                            break;
                        
                        default:
                            break;
                    }

                    // check if all data read
                    ind = ind + scanReport->data[ind] + 1;
                    if(ind >= scanReport->dataLen) 
                    {
                        break;
                    }
                }
                // it is one of the mussels
                if(success_flag == 0b11) 
                {
                    int i, found = 0;
                    // check if already in device list
                    for(i = 0; i < devIndex; i++) 
                    {
                        if( !strncmp((char*) scanReport->peerBdAddr, (char*)scannedPeriphDevice[i].connectPeriphDevice.bdAddr, 6) ) 
                        {
                            found = 1;
                            break;
                        }
                    }
                    // if not yet added, add
                    if(found == 0) 
                    { 
                        memcpy(scannedPeriphDevice[devIndex].connectPeriphDevice.bdAddr, scanReport->peerBdAddr,
                               sizeof(scannedPeriphDevice[devIndex].connectPeriphDevice.bdAddr));
                        memcpy(scannedPeriphDevice[devIndex].name, name, 10);
                        
                        UART_UartPutString ("\n");  
                        sprintf(buffer,"Found Device No: %d\r\n",devIndex);
                		UART_UartPutString(buffer);
                        
                        sprintf(buffer,"Name: %s\r\n",name);
                        UART_UartPutString(buffer);
                        
                        sprintf(buffer, "RSSI: %d \r\n", scanReport->rssi);
                        UART_UartPutString(buffer);
                        
                        devIndex++;
                    }
                }
            }
        }
    }
}

/*******************************************************************************
* Function Name: HandleBleProcessing
********************************************************************************
*
* Summary:
*   Handles the BLE state machine for intiating different procedures
*   during different states of BLESS.
*
* Parameters:
*   None.
*
* Return:
*   None.
*
*******************************************************************************/
void HandleBleProcessing(void)
{    
    CYBLE_API_RESULT_T      cyble_api_result;
    
    //UART_UartPutChar(cyBle_state);
    switch (cyBle_state)
    {

        case CYBLE_STATE_SCANNING:
            if(IsSelected)
            {
                CyBle_GapcStopScan();
            }
            break;
    
            
        case CYBLE_STATE_CONNECTED:
            
            /* if Client does not has all the information about attribute handles 
             * call procedure for getting it */
            if((INFO_EXCHANGE_COMPLETE != infoExchangeState))
            {
                attrHandleInit();
            }
            
            /* enable notifications if not enabled already */
            else if(false == notificationEnabled)
            {
                enableNotifications();
            }
            
            /* if client has all required info and stack is free, handle UART traffic */
            else if(CyBle_GattGetBusStatus() != CYBLE_STACK_STATE_BUSY)
            {
                HandleUartTxTraffic();
            }
            
            break;
                
        case CYBLE_STATE_DISCONNECTED:
        {
            if(IsSelected)
            {
                //UART_UartPutString("\n Found device...");
                //cyble_api_result = CyBle_GapcConnectDevice(&connectPeriphDevice[Periph_Selected]);
                cyble_api_result = CyBle_GapcConnectDevice(&scannedPeriphDevice[Periph_Selected].connectPeriphDevice);
                
            }
            else
            {
                CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
                UART_UartPutString("Start scanning...\n");
            }
            break;
        }
        
        default:
            //UART_UartPutString("\nDefault rsp");
            break;       
    }
}


/*******************************************************************************
* Function Name: AppCallBack
********************************************************************************
*
* Summary:
*   Call back function for BLE stack to handle BLESS events
*
* Parameters:
*   event       - the event generated by stack
*   eventParam  - the parameters related to the corresponding event
*
* Return:
*   None.
*
*******************************************************************************/
void AppCallBack(uint32 event, void *eventParam)
{
    
    CYBLE_GAPC_ADV_REPORT_T		            *advReport;
    CYBLE_GATTC_READ_BY_TYPE_RSP_PARAM_T    *readResponse;
    CYBLE_GATTC_FIND_BY_TYPE_RSP_PARAM_T    *findResponse;
    CYBLE_GATTC_FIND_INFO_RSP_PARAM_T       *findInfoResponse;
    int i;
    
    //UART_UartPutChar(event);
    switch (event)
    {
        case CYBLE_EVT_STACK_ON:
        {
            
            UART_UartPutString("BLE ON\r\n");
           
	        CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
            break;
        }
        
        case CYBLE_EVT_TIMEOUT: 
        {
            if( CYBLE_GAP_SCAN_TO ==*(uint16*) eventParam)
            {
                // Start Scanning again when timeout occurs
                CyBle_GapcStartScan(CYBLE_SCANNING_SLOW);
            }
        }

        case CYBLE_EVT_GAPC_SCAN_PROGRESS_RESULT:
        
            
            /*Get the information from advertisement and scan response packets*/
            Get_Adv_Scan_Packets((CYBLE_GAPC_ADV_REPORT_T *)eventParam);
            advReport = (CYBLE_GAPC_ADV_REPORT_T *) eventParam;
            
            break;    
            
        case CYBLE_EVT_GAP_AUTH_REQ:
            UART_UartPutString("\nAUTH\n");
            break;
            
            
        case CYBLE_EVT_GATT_CONNECT_IND:
            
            #ifdef PRINT_MESSAGE_LOG   
                UART_UartPutString("Connection established. Press button for disconnection\n\r");             
            #endif
            
            /* When the peripheral device is connected, store the connection handle.*/
            IsConnected = 1;

            connHandle = *(CYBLE_CONN_HANDLE_T *)eventParam;
            
            break;
            
        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            break;
            
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            
            /* RESET all flags */
            IsConnected = 0;
            notificationEnabled     = false;
            infoExchangeState       = INFO_EXCHANGE_START;
            
            if (!(IsSelected))
            {
                if(CYBLE_ERROR_OK != CyBle_GapcStartScan(CYBLE_SCANNING_FAST))
                {
                    UART_UartPutString ("Start Scanning Failed\n");
                }
            }
            
            #ifdef PRINT_MESSAGE_LOG   
                UART_UartPutString("\n\r\tDISCONNECTED!!! \n\r");
                while(0 != (UART_SpiUartGetTxBufferSize() + UART_GET_TX_FIFO_SR_VALID));
            #endif
            
            /* RESET Uart and flush all buffers */
            UART_Stop();
            UART_SpiUartClearTxBuffer();
            UART_SpiUartClearRxBuffer();
            UART_Start();
            
            break;
            
        case CYBLE_EVT_GAPC_SCAN_START_STOP:
            
            switch(CyBle_GetState())
            {
                case CYBLE_STATE_SCANNING:
                    UART_UartPutString("Scanning for BLE devices...\n\r");
                    break;
            
                default:
                    UART_UartPutString("Stopped scanning for BLE devices...\n\r");
                    break;
            }        
            break;

        case CYBLE_EVT_GATTC_READ_BY_TYPE_RSP:
            
            readResponse = (CYBLE_GATTC_READ_BY_TYPE_RSP_PARAM_T *) eventParam;
            
            if(0 == memcmp((uint8 *)&(readResponse->attrData.attrValue[5]), (uint8 *)uartTxAttrUuid, 16))
            {
                txCharHandle = readResponse->attrData.attrValue[3];
                txCharHandle |= (readResponse->attrData.attrValue[4] << 8);
                
                infoExchangeState |= TX_ATTR_HANDLE_FOUND;
            }
            else if(0 == memcmp((uint8 *)&(readResponse->attrData.attrValue[5]), (uint8 *)uartRxAttrUuid, 16))
            {
                rxCharHandle = readResponse->attrData.attrValue[3];
                rxCharHandle |= (readResponse->attrData.attrValue[4] << 8);
                
                infoExchangeState |= RX_ATTR_HANDLE_FOUND;
               
            }
            
            break;
            
        case CYBLE_EVT_GATTC_FIND_INFO_RSP:
            
            findInfoResponse = (CYBLE_GATTC_FIND_INFO_RSP_PARAM_T *) eventParam;
            
            if((0x29 == findInfoResponse->handleValueList.list[3]) && \
                                (0x02 == findInfoResponse->handleValueList.list[2]))
            {
                txCharDescHandle = findInfoResponse->handleValueList.list[0];
                txCharDescHandle |= findInfoResponse->handleValueList.list[1] << 8;
            
                infoExchangeState |= TX_CCCD_HANDLE_FOUND;
            }
           
            break;
            
        case CYBLE_EVT_GATTC_XCHNG_MTU_RSP:   
            
            /*set the 'mtuSize' variable based on the minimum MTU supported by both devices */
            if(CYBLE_GATT_MTU > ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu)
            {
                mtuSize = ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu;
            }
            else
            {
                mtuSize = CYBLE_GATT_MTU;
            }
            
            infoExchangeState |= MTU_XCHNG_COMPLETE;
            
            break;
            
        case CYBLE_EVT_GATTC_HANDLE_VALUE_NTF:
            
            HandleUartRxTraffic((CYBLE_GATTC_HANDLE_VALUE_NTF_PARAM_T *)eventParam);
			
            break;
        
        case CYBLE_EVT_GATTC_FIND_BY_TYPE_VALUE_RSP:
            
            findResponse            = (CYBLE_GATTC_FIND_BY_TYPE_RSP_PARAM_T *) eventParam;
            
            bleUartServiceHandle    = findResponse->range->startHandle;
            bleUartServiceEndHandle = findResponse->range->endHandle;
            
            infoExchangeState |= BLE_UART_SERVICE_HANDLE_FOUND;
            
            break;
        
        case CYBLE_EVT_GATTS_XCNHG_MTU_REQ:
            
            /*set the 'mtuSize' variable based on the minimum MTU supported by both devices */
            if(CYBLE_GATT_MTU > ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu)
            {
                mtuSize = ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu;
            }
            else
            {
                mtuSize = CYBLE_GATT_MTU;
            }
            
            break;    
        
        case CYBLE_EVT_GATTC_WRITE_RSP:
            
            notificationEnabled = true;
            
            #ifdef PRINT_MESSAGE_LOG   
                UART_UartPutString("\n\rStart entering data:\n\r");
            #endif
            
            break;

        default:            
            break;
    }
}


/*******************************************************************************
* Function Name: attrHandleInit
********************************************************************************
*
* Summary:
*  This function gathhers all the information like attribute handles and MTU size
*  from the server.
*
* Parameters:
*  None.
*
* Return:
*   None.
*
*******************************************************************************/
void attrHandleInit()
{
    switch(infoExchangeState)
    {
        case INFO_EXCHANGE_START:        
            CyBle_GattcDiscoverPrimaryServiceByUuid(cyBle_connHandle, bleUartServiceUuidInfo);
            break;
        
        case BLE_UART_SERVICE_HANDLE_FOUND:
            attrHandleRange.startHandle    = bleUartServiceHandle;
            attrHandleRange.endHandle      = bleUartServiceEndHandle;

            CyBle_GattcDiscoverAllCharacteristics(cyBle_connHandle, attrHandleRange);
            break;
        
        case (SERVICE_AND_CHAR_HANDLES_FOUND):
            charDescHandleRange.startHandle = txCharHandle + 1;
            charDescHandleRange.endHandle   = bleUartServiceEndHandle;

            CyBle_GattcDiscoverAllCharacteristicDescriptors(cyBle_connHandle, &charDescHandleRange);
            break;
        
        case (ALL_HANDLES_FOUND):
            CyBle_GattcExchangeMtuReq(cyBle_connHandle, CYBLE_GATT_MTU);
            break;    
            
        default:
            break;    
    }
    
    CyBle_ProcessEvents();
}

/*******************************************************************************
* Function Name: enableNotifications
********************************************************************************
*
* Summary:
*  This function enables notfications from servers. 
*
* Parameters:
*  None.
*
* Return:
*   None.
*
*******************************************************************************/
void enableNotifications()
{     
    enableNotificationReqParam.attrHandle = txCharDescHandle;   
    CyBle_GattcWriteCharacteristicDescriptors(cyBle_connHandle, (CYBLE_GATTC_WRITE_REQ_T *)(&enableNotificationReqParam));
}
/* [] END OF FILE */

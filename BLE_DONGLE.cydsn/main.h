/*******************************************************************************
* File Name: main.h
*
* Description:
*  Contains the function prototypes and constants available to the example
*  project.
*
********************************************************************************
* Copyright 2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if !defined(MAIN_H)

    #define MAIN_H
    
    #include <project.h>
    #include "app_Ble.h"
    #include "app_LED.h"
    #include "stdbool.h"    

// global vars
typedef enum
{
    UNDEFINED,
    SINGLE,
    GROUP
} Mode;

extern volatile Mode mode;
extern          unsigned char ids[256];
extern          unsigned char done[256];
extern volatile int numIds;
extern volatile int currentId;
extern          char command[512];
extern volatile int commandLength;
extern volatile int commandIdx;
extern volatile int mainPromptActive;
extern          char deviceName[5];

/***************************************
 *   Conditional compilation parameters
 ***************************************/      
//#define     FLOW_CONTROL
#define     PRINT_MESSAGE_LOG
//#define     LOW_POWER_MODE

/***************************************
 *       Function Prototypes
 ***************************************/
void AppCallBack(uint32 , void *);  
void setDeviceNameFromId (int currentId);
void sendIntToUART(uint16 num);

int getNextId();
int findId (int label);
void resetIdStatus();
void setCurrentId (int i);

#endif

/* [] END OF FILE */

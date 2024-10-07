/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for MQTT Client Example for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Header file includes
 ******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_log.h"
#include "cy_retarget_io.h"
#include "mqtt_task.h"
#include "FreeRTOS.h"
#include "task.h"

#include "optiga/pal/pal_os_event.h"
#include "optiga/pal/pal_i2c.h"
#include "optiga_trust_helpers.h"

/******************************************************************************
* Global Variables
******************************************************************************/
/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/* Extract the certificate and use it for the actual communication */
extern void use_optiga_certificate(void);


/******************************************************************************
 * Function Name: optiga_client_task
 ******************************************************************************
 * Summary:
 *  A task to properly initialize the chip and use the certificate installed
 *  to the OPTIGA Secure Element. After this start the MQTT task
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/

void optiga_client_task(void *pvParameters)
{
    char optiga_cert_pem[1024];
    uint16_t optiga_cert_pem_size = 1024;

    printf("\x1b[2J\x1b[;H");
    optiga_trust_init();
        /* This is the place where the certificate is initialized. This is a function
     * which will allow to read it out and populate internal mbedtls settings wit it*/
    read_certificate_from_optiga(0xe0e0, optiga_cert_pem, &optiga_cert_pem_size);
    printf("Your certificate is:\n%s\n",optiga_cert_pem);

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen. */
    printf("===============================================================\n");
    printf("CE229889 - AnyCloud Example: MQTT Client\n");
    printf("===============================================================\n\n");

    /* Create the MQTT Client task. */
    xTaskCreate(mqtt_client_task, "MQTT Client task", MQTT_CLIENT_TASK_STACK_SIZE,
                NULL, MQTT_CLIENT_TASK_PRIORITY, NULL);

    while(1);
}

#ifdef ENABLE_SECURE_SOCKETS_LOGS
 int app_log_output_callback(CY_LOG_FACILITY_T facility, CY_LOG_LEVEL_T level, char *logmsg)
 {
     (void)facility;     // Can be used to decide to reduce output or send output to remote logging
     (void)level;        // Can be used to decide to reduce output, although the output has already been
                         // limited by the log routines
 
     return printf( "%s\n", logmsg);   // print directly to console
 }
 
/*
  Log time callback - get the current time for the log message timestamp in millseconds
 */
 cy_rslt_t app_log_time(uint32_t* time)
 {
     if (time != NULL)
     {
         *time =  xTaskGetTickCount(); // get system time (in milliseconds)
     }
     return CY_RSLT_SUCCESS;
 }
#endif

/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes retarget IO, sets up 
 *  the MQTT client task, and then starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main()
{
    cy_rslt_t result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    /* Initialize the board support package. */
    result = cybsp_init();
    CY_ASSERT(CY_RSLT_SUCCESS == result);

    /* To avoid compiler warnings. */
    (void) result;

    /* Enable global interrupts. */
    __enable_irq();

#ifdef ENABLE_SECURE_SOCKETS_LOGS
    /* 
        This function is required only if there are some problems with the 
        secure sockets (pkcs11 interface to the optiga chip) and you would
        like to understand better what is happening. Additionally add the
        following Macros for the Makefile list of DEFINES
        CY_SECURE_SOCKETS_PKCS_SUPPORT MBEDTLS_VERBOSE=4
    */
    cy_rslt_t rs = cy_log_init(CY_LOG_MAX, app_log_output_callback, app_log_time);
    if (rs != CY_RSLT_SUCCESS)
    {
        printf("cy_log_init() FAILED %ld\n", result);
    }
#endif    

    /* Initialize retarget-io to use the debug UART port. */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                        CY_RETARGET_IO_BAUDRATE);

    /* Create an OPTIGA task to make sure everything related to
     * the OPTIGA stack will be called from the scheduler */
    xTaskCreate(optiga_client_task, "OPTIGA", 1024 * 12, NULL, 2, NULL);

    /* Start the FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never get here. */
    CY_ASSERT(0);
}

/* [] END OF FILE */

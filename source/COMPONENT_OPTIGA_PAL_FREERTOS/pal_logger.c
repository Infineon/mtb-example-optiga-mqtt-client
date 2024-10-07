/******************************************************************************
* File Name:   pal_logger.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. This file provides
*              the prototypes declarations for pal logger.
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

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

#include "optiga/pal/pal_logger.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
#ifdef DEVELOPER_MODE_LOGGER
// Macro Definitions
#define BAUD_RATE       115200
#define INT_PRIORITY    3
#define DATA_BITS_8     8
#define STOP_BITS_1     1
#define PAL_LOGGER_UART_INTR_PRIO    (5U)
#endif
/// @cond hidden

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
//lint --e{552,714} suppress "Accessed by user of this structure" 
pal_logger_t logger_console =
{
        .logger_config_ptr = NULL,
        .logger_rx_flag = 1,
        .logger_tx_flag = 1
};

#ifdef DEVELOPER_MODE_LOGGER
cyhal_uart_t pal_logger_uart_obj;
uint8_t cy_hal_uart_event_status = CYHAL_UART_IRQ_NONE;
volatile bool event_status_busy = true;

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
// Event handler callback function
static void pal_logger_uart_event_handler(void* handler_arg, cyhal_uart_event_t event)
{
    event_status_busy = false;
    if ((event & CYHAL_UART_IRQ_TX_DONE) == CYHAL_UART_IRQ_TX_DONE)
    {
        // All Tx data has been transmitted
      logger_console.logger_tx_flag = 0;
    }
    else if ((event & CYHAL_UART_IRQ_RX_DONE) == CYHAL_UART_IRQ_RX_DONE)
    {
        // All Rx data has been received
        logger_console.logger_rx_flag = 0;
    }
    else if ((event & CYHAL_UART_IRQ_TX_ERROR) == CYHAL_UART_IRQ_TX_ERROR)
    {
        // Transmit error
        cy_hal_uart_event_status = CYHAL_UART_IRQ_TX_ERROR;
    }
    else if ((event & CYHAL_UART_IRQ_RX_ERROR) == CYHAL_UART_IRQ_RX_ERROR)
    {
        // Receive error
        cy_hal_uart_event_status = CYHAL_UART_IRQ_RX_ERROR;
    }
}

static void pal_logger_uart_rst_events(void)
{
    cy_hal_uart_event_status = CYHAL_UART_IRQ_NONE;
    event_status_busy = true;
}

#endif

/// @endcond

pal_status_t pal_logger_read_byte_length(void * p_logger_context, uint8_t * p_log_data, uint32_t log_data_length)
{
    return 1;
}

pal_status_t pal_logger_init(void * p_logger_context)
{
#ifdef DEVELOPER_MODE_LOGGER
    pal_status_t return_status = PAL_STATUS_FAILURE;
    cy_rslt_t cy_hal_status = CY_RSLT_SUCCESS;
    uint32_t actualbaud = 0;

    do
    {
            // Initialize the UART configuration structure
            cyhal_uart_cfg_t uart_config =
            {
                .data_bits      = DATA_BITS_8,
                .stop_bits      = STOP_BITS_1,
                .parity         = CYHAL_UART_PARITY_NONE,
                .rx_buffer      = NULL,
                .rx_buffer_size = 0
            };
            // Initialize the UART Block
            cy_hal_status =    cyhal_uart_init(&pal_logger_uart_obj, CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, NULL, &uart_config);
            if(CY_RSLT_SUCCESS != cy_hal_status)
            {
                break;
            }
            // Set the baud rate
            cy_hal_status =    cyhal_uart_set_baud(&pal_logger_uart_obj, BAUD_RATE, &actualbaud);
            if(CY_RSLT_SUCCESS != cy_hal_status)
            {
                   break;
            }
            // The UART callback handler registration
            cyhal_uart_register_callback(&pal_logger_uart_obj, (cyhal_uart_event_callback_t)pal_logger_uart_event_handler, NULL);
            // Enable required UART events
            cyhal_uart_enable_event(&pal_logger_uart_obj,
                                    (cyhal_uart_event_t)(CYHAL_UART_IRQ_TX_DONE | CYHAL_UART_IRQ_TX_ERROR |
                                                         CYHAL_UART_IRQ_RX_DONE),
                PAL_LOGGER_UART_INTR_PRIO, true);

            return_status = PAL_STATUS_SUCCESS;
    }while(FALSE);
    return return_status;
#endif
    return PAL_STATUS_SUCCESS;
}


pal_status_t pal_logger_deinit(void * p_logger_context)
{
#ifdef DEVELOPER_MODE_LOGGER
    cyhal_uart_free(&pal_logger_uart_obj);
#endif
    return PAL_STATUS_SUCCESS;
}


pal_status_t pal_logger_write(void * p_logger_context, const uint8_t * p_log_data, uint32_t log_data_length)
{
#ifdef DEVELOPER_MODE_LOGGER
    int32_t return_status = PAL_STATUS_FAILURE;
    cy_rslt_t cy_hal_status = CY_RSLT_SUCCESS;

    do
    {
        if(0 == log_data_length || NULL == p_log_data)
        {
            break;
        }
        // Begin asynchronous TX transfer
        pal_logger_uart_rst_events();

        cy_hal_status = cyhal_uart_write_async(&pal_logger_uart_obj, (void*)p_log_data, log_data_length);

        if(CY_RSLT_SUCCESS != cy_hal_status)
        {
               break;
        }

        while(event_status_busy);

        if(CYHAL_UART_IRQ_TX_ERROR == cy_hal_uart_event_status)
        {
            break;
        }

        return_status = PAL_STATUS_SUCCESS;

    } while(0);

    return (return_status);
#endif
    printf("%.*s", (int)log_data_length, p_log_data);
    return PAL_STATUS_SUCCESS;
}

pal_status_t pal_logger_read(void * p_logger_context, uint8_t * p_log_data, uint32_t log_data_length)
{
#ifdef DEVELOPER_MODE_LOGGER
    int32_t return_status = PAL_STATUS_FAILURE;
    cy_rslt_t cy_hal_status = CY_RSLT_SUCCESS;
    do
    {
        // Begin asynchronous RX transfer
        pal_logger_uart_rst_events();

        cy_hal_status = cyhal_uart_read_async(&pal_logger_uart_obj, p_log_data, log_data_length);
        if(CY_RSLT_SUCCESS != cy_hal_status)
        {
               break;
        }

        while(event_status_busy);

        if(CYHAL_UART_IRQ_RX_ERROR == cy_hal_uart_event_status)
        {
            break;
        }

        return_status = PAL_STATUS_SUCCESS;

    } while(0);

    return (return_status);
#endif
    char str[8] = "%";
    char str_1[7] = "";
    
    if (sprintf(str_1, "%ds", (int)log_data_length) <= 0)
    {
        return PAL_STATUS_FAILURE;
    }

    strcat(str, str_1);
    if (scanf(str, p_log_data) <= 0)
    {
        return PAL_STATUS_FAILURE;
    }

    return PAL_STATUS_SUCCESS;
}

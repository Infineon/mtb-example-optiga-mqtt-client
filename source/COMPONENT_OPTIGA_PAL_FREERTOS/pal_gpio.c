/******************************************************************************
* File Name:   pal_i2c.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. This file implements 
*              the platform abstraction layer APIs for GPIO.
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
#include "optiga/pal/pal_ifx_i2c_config.h"
#include "optiga/pal/pal_gpio.h"
#include "pal_psoc_gpio_mapping.h"


#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
//lint --e{714,715} suppress "This is implemented for overall completion of API"
pal_status_t pal_gpio_init(const pal_gpio_t * p_gpio_context)
{
    pal_status_t cy_hal_status = PAL_STATUS_SUCCESS;
    pal_psoc_gpio_t* pin_config = (pal_psoc_gpio_t *)p_gpio_context->p_gpio_hw;
    cy_hal_status = cyhal_gpio_init(pin_config->gpio,
                                       CYHAL_GPIO_DIR_OUTPUT, 
                                       CYHAL_GPIO_DRIVE_STRONG, 
                                       pin_config->init_state);
    if (CY_RSLT_SUCCESS != cy_hal_status)
    {
        cy_hal_status = PAL_STATUS_FAILURE;
    }

    return (cy_hal_status);
}

//lint --e{714,715} suppress "This is implemented for overall completion of API"
pal_status_t pal_gpio_deinit(const pal_gpio_t * p_gpio_context)
{
    pal_psoc_gpio_t* pin_config = (pal_psoc_gpio_t *)p_gpio_context->p_gpio_hw;
    cyhal_gpio_free(pin_config->gpio);
    return (PAL_STATUS_SUCCESS);
}

void pal_gpio_set_high(const pal_gpio_t * p_gpio_context)
{
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
        pal_psoc_gpio_t* pin_config = (pal_psoc_gpio_t *)p_gpio_context->p_gpio_hw;
        cyhal_gpio_write((cyhal_gpio_t)(pin_config->gpio), true);
    }
}

void pal_gpio_set_low(const pal_gpio_t * p_gpio_context)
{
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL))
    {
        pal_psoc_gpio_t* pin_config = (pal_psoc_gpio_t *)p_gpio_context->p_gpio_hw;
        cyhal_gpio_write((cyhal_gpio_t)(pin_config->gpio), false);
    }
}

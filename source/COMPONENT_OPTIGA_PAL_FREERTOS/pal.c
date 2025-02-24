/******************************************************************************
* File Name:   pal_i2c.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. This file implements 
*              the platform abstraction layer APIs.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2025, Cypress Semiconductor Corporation (an Infineon company) or
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
#include "include/pal/pal.h"
#include "include/pal/pal_gpio.h"
#include "include/pal/pal_i2c.h"
#include "include/pal/pal_os_event.h"
#include "include/pal/pal_os_timer.h"
#include "pal_psoc_gpio_mapping.h"
#include "optiga_lib_config.h"

/*******************************************************************************
 * Global variables
 ******************************************************************************/
#ifdef OPTIGA_TRUSTM_VDD
extern pal_gpio_t optiga_vdd_0;
#endif

#ifdef OPTIGA_TRUSTM_RST
extern pal_gpio_t optiga_reset_0;
#endif

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
pal_status_t pal_init(void)
{
    // This function call is used to create a semaphore outside of the ISR
    pal_i2c_init(NULL);

    #ifdef OPTIGA_TRUSTM_VDD
    pal_gpio_init(&optiga_vdd_0);
    #endif

    #ifdef OPTIGA_TRUSTM_RST
    pal_gpio_init(&optiga_reset_0);
    #endif
    return PAL_STATUS_SUCCESS;
}


pal_status_t pal_deinit(void)
{
    // This function call is used to destroy a semaphore outside of the ISR
    pal_i2c_deinit(NULL);
    return PAL_STATUS_SUCCESS;
}

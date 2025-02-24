/******************************************************************************
* File Name:   pal_os_datastore.c
*
* Description: This file contains part of the Platform Abstraction Layer.
*              This is a platform specific file. This file contains A
*              default Platform Binding Secret (PBS). In case you have your 
*              owned PBS, past it into the 
*              optiga_platform_binding_shared_secret where two first bytes
*              indicate the length
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
#include "include/pal/pal_os_datastore.h"

/*******************************************************************************
 * Macros
 ******************************************************************************/
/// Size of length field 
#define LENGTH_SIZE                (0x02)
/// Size of data store buffer to hold the shielded connection manage context information (2 bytes length field + 64(0x40) bytes context)
#define MANAGE_CONTEXT_BUFFER_SIZE      (0x42)

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
//Internal buffer to store the shielded connection manage context information (length field + Data)
uint8_t data_store_manage_context_buffer [LENGTH_SIZE + MANAGE_CONTEXT_BUFFER_SIZE];

//Internal buffer to store the optiga application context data during hibernate(length field + Data)
uint8_t data_store_app_context_buffer [LENGTH_SIZE + APP_CONTEXT_SIZE];

//Internal buffer to store the generated platform binding shared secret on Host (length field + shared secret)
uint8_t optiga_platform_binding_shared_secret [LENGTH_SIZE + OPTIGA_SHARED_SECRET_MAX_LENGTH] = 
{
    // Length of the shared secret, followed after the length information
    0x00 ,0x40, 
    // Shared secret. Buffer is defined to the maximum supported length [64 bytes]. 
    // But the actual size used is to be specified in the length field.
    0x01 ,0x02 ,0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 ,0x0A ,0x0B ,0x0C ,0x0D ,0x0E ,0x0F ,0x10,
    0x11 ,0x12 ,0x13 ,0x14 ,0x15 ,0x16 ,0x17 ,0x18 ,0x19 ,0x1A ,0x1B ,0x1C ,0x1D ,0x1E ,0x1F ,0x20,
    0x21 ,0x22 ,0x23 ,0x24 ,0x25 ,0x26 ,0x27 ,0x28 ,0x29 ,0x2A ,0x2B ,0x2C ,0x2D ,0x2E ,0x2F ,0x30,
    0x31 ,0x32 ,0x33 ,0x34 ,0x35 ,0x36 ,0x37 ,0x38 ,0x39 ,0x3A ,0x3B ,0x3C ,0x3D ,0x3E ,0x3F ,0x40
};

/*******************************************************************************
 * Function Definitions
 ******************************************************************************/
pal_status_t pal_os_datastore_write(uint16_t datastore_id,
                                    const uint8_t * p_buffer,
                                    uint16_t length)
{
    pal_status_t return_status = PAL_STATUS_FAILURE;
    uint8_t offset = 0;

    switch(datastore_id)
    {
        case OPTIGA_PLATFORM_BINDING_SHARED_SECRET_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only, in case of updating
            // the platform binding shared secret during the runtime into NVM.
            // In current implementation, platform binding shared secret is 
            // stored in RAM.
            if (length <= OPTIGA_SHARED_SECRET_MAX_LENGTH)
            {
                optiga_platform_binding_shared_secret[offset++] = (uint8_t)(length>>8);
                optiga_platform_binding_shared_secret[offset++] = (uint8_t)(length);
                memcpy(&optiga_platform_binding_shared_secret[offset], p_buffer, length);
                return_status = PAL_STATUS_SUCCESS;
            }
            break;
        }
        case OPTIGA_COMMS_MANAGE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only, in case of storing 
            // the manage context information in non-volatile memory 
            // to reuse for later during hard reset scenarios where the 
            // RAM gets flushed out.
            data_store_manage_context_buffer[offset++] = (uint8_t)(length>>8);
            data_store_manage_context_buffer[offset++] = (uint8_t)(length);
            memcpy(&data_store_manage_context_buffer[offset],p_buffer,length);
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_HIBERNATE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only, in case of storing 
            // the application context information in non-volatile memory 
            // to reuse for later during hard reset scenarios where the 
            // RAM gets flushed out.
            data_store_app_context_buffer[offset++] = (uint8_t)(length>>8);
            data_store_app_context_buffer[offset++] = (uint8_t)(length);
            memcpy(&data_store_app_context_buffer[offset],p_buffer,length);
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        default:
        {
            break;
        }
    }
    return return_status;
}


pal_status_t pal_os_datastore_read(uint16_t datastore_id, 
                                   uint8_t * p_buffer, 
                                   uint16_t * p_buffer_length)
{
    pal_status_t return_status = PAL_STATUS_FAILURE;
    uint16_t data_length;
    uint8_t offset = 0;

    switch(datastore_id)
    {
        case OPTIGA_PLATFORM_BINDING_SHARED_SECRET_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only,
            // if the platform binding shared secret is stored in non-volatile 
            // memory with a specific location and not as a context segment 
            // else updating the share secret content is good enough.

            data_length = (uint16_t) (optiga_platform_binding_shared_secret[offset++] << 8);
            data_length |= (uint16_t)(optiga_platform_binding_shared_secret[offset++]);
            if (data_length <= OPTIGA_SHARED_SECRET_MAX_LENGTH)
            {
                memcpy(p_buffer,&optiga_platform_binding_shared_secret[offset], data_length);
                *p_buffer_length = data_length;
                return_status = PAL_STATUS_SUCCESS;
            }
            break;
        }
        case OPTIGA_COMMS_MANAGE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only,
            // if manage context information is stored in NVM during the hibernate, 
            // else this is not required to be enhanced.
            data_length = (uint16_t) (data_store_manage_context_buffer[offset++] << 8);
            data_length |= (uint16_t)(data_store_manage_context_buffer[offset++]);
            memcpy(p_buffer, &data_store_manage_context_buffer[offset], data_length);
            *p_buffer_length = data_length;
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_HIBERNATE_CONTEXT_ID:
        {
            // !!!OPTIGA_LIB_PORTING_REQUIRED
            // This has to be enhanced by user only,
            // if application context information is stored in NVM during the hibernate, 
            // else this is not required to be enhanced.
            data_length = (uint16_t) (data_store_app_context_buffer[offset++] << 8);
            data_length |= (uint16_t)(data_store_app_context_buffer[offset++]);
            memcpy(p_buffer, &data_store_app_context_buffer[offset], data_length);
            *p_buffer_length = data_length;
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        default:
        {
            *p_buffer_length = 0;
            break;
        }
    }

    return return_status;
}

/******************************************************************************
* File Name:   optiga_trust_helpers.c
*
* Description: This file contains helping fucntions to read a certificate or 
*              change some default parameter
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

#include <stdlib.h>
#include <stdio.h>
#include "optiga/optiga_util.h"
#include "optiga/common/optiga_lib_logger.h"
#include "optiga/pal/pal_os_event.h"
#include "optiga/pal/pal_gpio.h"
#include "optiga/ifx_i2c/ifx_i2c_config.h"
#include "optiga/pal/pal_ifx_i2c_config.h"
#include "mbedtls/base64.h"
#include "optiga_trust_helpers.h"

/**
 * Callback when optiga_util_xxxx operation is completed asynchronously
 */
static volatile optiga_lib_status_t optiga_lib_status;
static void optiga_util_callback(void * context, optiga_lib_status_t return_status)
{
    optiga_lib_status = return_status;
}

void read_certificate_from_optiga(uint16_t optiga_oid, char * cert_pem, uint16_t * cert_pem_length)
{
    size_t  ifx_cert_b64_len = 0;
    uint8_t ifx_cert_b64_temp[1200];
    uint16_t offset_to_write = 0, offset_to_read = 0;
    uint16_t size_to_copy = 0;
    optiga_lib_status_t return_status;

    optiga_util_t * me_util = NULL;
    uint8_t ifx_cert_hex[1024];
    uint16_t  ifx_cert_hex_len = sizeof(ifx_cert_hex);
    
    do
    {
        //Create an instance of optiga_util to read the certificate from OPTIGA.
        me_util = optiga_util_create(0, optiga_util_callback, NULL);
        if(!me_util)
        {
            optiga_lib_print_message("optiga_util_create failed !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }
        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_util_read_data(me_util, optiga_oid, 0, ifx_cert_hex, &ifx_cert_hex_len);
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            //optiga_util_read_data api returns error !!!
            optiga_lib_print_message("optiga_util_read_data api returns error !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }
            
        while (optiga_lib_status == OPTIGA_LIB_BUSY);
        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //optiga_util_read_data failed
            optiga_lib_print_message("optiga_util_read_data failed",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }
        
        //convert to PEM format
        // If the first byte is TLS Identity Tag, than we need to skip 9 first bytes
        offset_to_read = ifx_cert_hex[0] == 0xc0? 9: 0;
        mbedtls_base64_encode((unsigned char *)ifx_cert_b64_temp, sizeof(ifx_cert_b64_temp),
                              &ifx_cert_b64_len,
                              ifx_cert_hex + offset_to_read, ifx_cert_hex_len - offset_to_read);

        memcpy(cert_pem, "-----BEGIN CERTIFICATE-----\n", 28);
        offset_to_write += 28;
       
        //Properly copy certificate and format it as pkcs expects
        for (offset_to_read = 0; offset_to_read < ifx_cert_b64_len;)
        {
            // The last block of data usually is less than 64, thus we need to find the leftover
            if ((offset_to_read + 64) >= ifx_cert_b64_len)
                size_to_copy = ifx_cert_b64_len - offset_to_read;
            else
                size_to_copy = 64;
            memcpy(cert_pem + offset_to_write, ifx_cert_b64_temp + offset_to_read, size_to_copy);
            offset_to_write += size_to_copy;
            offset_to_read += size_to_copy;
            cert_pem[offset_to_write] = '\n';
            offset_to_write++;
        }

        memcpy(cert_pem + offset_to_write, "-----END CERTIFICATE-----\n\0", 27);

        *cert_pem_length = offset_to_write + 27;
   
    } while(0);

    //me_util instance to be destroyed 
    if (me_util)
    {
        optiga_util_destroy(me_util);
    }
}

void read_trust_anchor_from_optiga(uint16_t oid, char * cert_pem, uint16_t * cert_pem_length)
{
    size_t  ifx_cert_b64_len = 0;
    uint8_t ifx_cert_b64_temp[1200];
    uint16_t offset_to_write = 0, offset_to_read = 0;
    uint16_t size_to_copy = 0;
    optiga_lib_status_t return_status;

    optiga_util_t * me_util = NULL;
    uint8_t ifx_cert_hex[1300];
    uint16_t  ifx_cert_hex_len = sizeof(ifx_cert_hex);

    do
    {
        //Create an instance of optiga_util to read the certificate from OPTIGA.
        me_util = optiga_util_create(0, optiga_util_callback, NULL);
        if(!me_util)
        {
            optiga_lib_print_message("optiga_util_create failed !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }
        optiga_lib_status = OPTIGA_LIB_BUSY;   
        return_status = optiga_util_read_data(me_util, oid, 0, ifx_cert_hex, &ifx_cert_hex_len);
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            //optiga_util_read_data api returns error !!!
            optiga_lib_print_message("optiga_util_read_data api for trust anchor returns error !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }
            
        while (optiga_lib_status == OPTIGA_LIB_BUSY)
        {
            pal_os_timer_delay_in_milliseconds(30);
        }
        
        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //optiga_util_read_data failed
            optiga_lib_print_message("optiga_util_read_data failed for reading trust anchor",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }

        //ESP_LOG_BUFFER_HEX("OPTIGA Certificate", ifx_cert_hex, ifx_cert_hex_len);
        
        //convert to PEM format
       // printf("read ifx cer\r\n");
        mbedtls_base64_encode((unsigned char *)ifx_cert_b64_temp, sizeof(ifx_cert_b64_temp),
                              &ifx_cert_b64_len,
        //in case of TLS chain format of ceritificate
        //                    ifx_cert_hex + 9, ifx_cert_hex_len - 9);
                              ifx_cert_hex, ifx_cert_hex_len);

        memcpy(cert_pem, "-----BEGIN CERTIFICATE-----\n", 28);
        offset_to_write += 28;
        
        //TBD: Verify the given cert_pem length against the ifx_cert_b64_len after conversion
        
        //Properly copy certificate and format it as pkcs expects
        for (offset_to_read = 0; offset_to_read < ifx_cert_b64_len;)
        {
            // The last block of data usually is less than 64, thus we need to find the leftover
            if ((offset_to_read + 64) >= ifx_cert_b64_len)
                size_to_copy = ifx_cert_b64_len - offset_to_read;
            else
                size_to_copy = 64;
            memcpy(cert_pem + offset_to_write, ifx_cert_b64_temp + offset_to_read, size_to_copy);
            offset_to_write += size_to_copy;
            offset_to_read += size_to_copy;
            cert_pem[offset_to_write] = '\n';
            offset_to_write++;
        }

        memcpy(cert_pem + offset_to_write, "-----END CERTIFICATE-----\n\0", 27);

        *cert_pem_length = offset_to_write + 27;
       /* for(int i=0; i<1200; i++)
        {
            printf("%c", cert_pem[i]);
        }*/       
    } while(0);

    //me_util instance to be destroyed 
    if (me_util)
    {
        optiga_util_destroy(me_util);
    }
}

void write_data_object (uint16_t oid, const uint8_t * p_data, uint16_t length)
{
    optiga_util_t * me_util = NULL;
    optiga_lib_status_t return_status;
    
    do
    {
        //Create an instance of optiga_util to open the application on OPTIGA.
        me_util = optiga_util_create(0, optiga_util_callback, NULL);
        if(!me_util)
        {
            optiga_lib_print_message("optiga_util_create failed !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }

        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_util_write_data(me_util,
                                               oid,
                                               OPTIGA_UTIL_ERASE_AND_WRITE,
                                               0,
                                               p_data,
                                               length);
        {
            if (OPTIGA_LIB_SUCCESS != return_status)
            {
                optiga_lib_print_message("optiga_util_wirte_data api returns error !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
                break;
            }

            while (OPTIGA_LIB_BUSY == optiga_lib_status)
            {
                //Wait until the optiga_util_write_data operation is completed
            }

            if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
            {
                optiga_lib_print_message("optiga_util_write_data failed",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
                return_status = optiga_lib_status;
                break;
            }
            else
            {
                optiga_lib_print_message("optiga_util_write_data successful",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            }
        }
    } while (0);

    //me_util instance can be destroyed 
    //if no close_application w.r.t hibernate is required to be performed
    if (me_util)
    {
        optiga_util_destroy(me_util);
    }
}

// static void write_device_certificate (uint16_t certificate_oid)
// {
//     //Device certificate
//     const uint8_t certificate [] = {
//         0x30, 0x82, 0x02, 0x3E, 0x30, 0x82, 0x01, 0xE4, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x08, 0x66, 
//         0xAD, 0x36, 0x73, 0xED, 0xA4, 0x34, 0x24, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 
//         0x04, 0x03, 0x02, 0x30, 0x77, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 
//         0x44, 0x45, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x18, 0x49, 0x6E, 0x66, 
//         0x69, 0x6E, 0x65, 0x6F, 0x6E, 0x20, 0x54, 0x65, 0x63, 0x68, 0x6E, 0x6F, 0x6C, 0x6F, 0x67, 0x69, 
//         0x65, 0x73, 0x20, 0x41, 0x47, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x0A, 
//         0x4F, 0x50, 0x54, 0x49, 0x47, 0x41, 0x28, 0x54, 0x4D, 0x29, 0x31, 0x30, 0x30, 0x2E, 0x06, 0x03, 
//         0x55, 0x04, 0x03, 0x0C, 0x27, 0x49, 0x6E, 0x66, 0x69, 0x6E, 0x65, 0x6F, 0x6E, 0x20, 0x4F, 0x50, 
//         0x54, 0x49, 0x47, 0x41, 0x28, 0x54, 0x4D, 0x29, 0x20, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x4D, 
//         0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x43, 0x41, 0x20, 0x30, 0x30, 0x30, 0x30, 0x1E, 0x17, 0x0D, 
//         0x31, 0x39, 0x30, 0x38, 0x30, 0x33, 0x31, 0x31, 0x33, 0x39, 0x30, 0x30, 0x5A, 0x17, 0x0D, 0x32, 
//         0x39, 0x30, 0x38, 0x30, 0x33, 0x31, 0x31, 0x33, 0x39, 0x30, 0x30, 0x5A, 0x30, 0x50, 0x31, 0x0B, 
//         0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x49, 0x4E, 0x31, 0x0B, 0x30, 0x09, 0x06, 
//         0x03, 0x55, 0x04, 0x07, 0x13, 0x02, 0x4C, 0x4E, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 
//         0x0A, 0x13, 0x02, 0x4F, 0x4E, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x13, 0x02, 
//         0x4F, 0x55, 0x31, 0x1A, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x11, 0x49, 0x6E, 0x66, 
//         0x69, 0x6E, 0x65, 0x6F, 0x6E, 0x5F, 0x49, 0x6F, 0x54, 0x5F, 0x4E, 0x6F, 0x64, 0x65, 0x30, 0x59, 
//         0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 
//         0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x55, 0x1E, 0x0A, 0xD9, 0x01, 0x19, 0xD0, 
//         0x44, 0x3E, 0xBD, 0xE4, 0x4B, 0xEC, 0xA3, 0xA2, 0xE9, 0x07, 0x08, 0xD2, 0x0A, 0x39, 0x20, 0xE1, 
//         0x0C, 0x69, 0xD7, 0xD6, 0xAE, 0xA5, 0xDD, 0x6F, 0x41, 0x42, 0xE2, 0x73, 0x51, 0x0C, 0x6D, 0xD0, 
//         0x05, 0x02, 0x60, 0x5F, 0x6D, 0x45, 0x35, 0x4F, 0xCC, 0x7F, 0x0C, 0xDB, 0x1E, 0xBB, 0xDD, 0x4D, 
//         0x8E, 0x40, 0xBC, 0x55, 0x65, 0xD2, 0x7A, 0x2F, 0x81, 0xA3, 0x81, 0x80, 0x30, 0x7E, 0x30, 0x0C, 
//         0x06, 0x03, 0x55, 0x1D, 0x13, 0x01, 0x01, 0xFF, 0x04, 0x02, 0x30, 0x00, 0x30, 0x1D, 0x06, 0x03, 
//         0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0xB9, 0x46, 0xB7, 0x00, 0x01, 0xD9, 0x5E, 0xFC, 0x80, 
//         0x42, 0x0E, 0xED, 0x6A, 0xF9, 0x0B, 0x53, 0x79, 0xA7, 0x4F, 0xAE, 0x30, 0x1F, 0x06, 0x03, 0x55, 
//         0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x53, 0x1B, 0x46, 0x32, 0xF2, 0xBA, 0x1B, 0xEC, 
//         0x35, 0x23, 0xB0, 0xC6, 0x84, 0xE2, 0xBC, 0x7F, 0x11, 0xDA, 0xA2, 0x2E, 0x30, 0x0E, 0x06, 0x03, 
//         0x55, 0x1D, 0x0F, 0x01, 0x01, 0xFF, 0x04, 0x04, 0x03, 0x02, 0x07, 0x80, 0x30, 0x1E, 0x06, 0x09, 
//         0x60, 0x86, 0x48, 0x01, 0x86, 0xF8, 0x42, 0x01, 0x0D, 0x04, 0x11, 0x16, 0x0F, 0x78, 0x63, 0x61, 
//         0x20, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x30, 0x0A, 0x06, 0x08, 
//         0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02, 0x21, 0x00, 
//         0x98, 0x00, 0x55, 0x8D, 0x58, 0xE9, 0x24, 0xB9, 0x69, 0x1B, 0x12, 0x0D, 0x4E, 0xE0, 0xAB, 0xF3, 
//         0x00, 0xDA, 0x14, 0x3A, 0x39, 0x05, 0xE7, 0xC8, 0xCE, 0xBD, 0x07, 0x0F, 0x7D, 0x03, 0xEA, 0x54, 
//         0x02, 0x20, 0x45, 0x77, 0x6C, 0x77, 0xD0, 0xCC, 0x51, 0xF8, 0xD5, 0x77, 0x5C, 0xE7, 0xBC, 0xED, 
//         0x56, 0xD8, 0x39, 0xF9, 0x98, 0x8C, 0x06, 0xFF, 0x56, 0x73, 0x79, 0x04, 0x1E, 0x37, 0xAB, 0x5F, 
//         0x8B, 0x73, 
//     };
    
//     write_data_object (certificate_oid, certificate, sizeof(certificate));
// }

// static void write_optiga_trust_anchor(void)
// {

// #if 0
//     /* DigiCert Baltimore Root --Used Globally--*/
//     // This cert should be used when connecting to Azure IoT on the Azure Cloud available globally. 
//     const uint8_t trust_anchor[] = {
//         0x30, 0x82, 0x03, 0x77, 0x30, 0x82, 0x02, 0x5F, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x04, 0x02, 
//         0x00, 0x00, 0xB9, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05, 
//         0x05, 0x00, 0x30, 0x5A, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x49, 
//         0x45, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x09, 0x42, 0x61, 0x6C, 0x74, 
//         0x69, 0x6D, 0x6F, 0x72, 0x65, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x13, 0x0A, 
//         0x43, 0x79, 0x62, 0x65, 0x72, 0x54, 0x72, 0x75, 0x73, 0x74, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 
//         0x55, 0x04, 0x03, 0x13, 0x19, 0x42, 0x61, 0x6C, 0x74, 0x69, 0x6D, 0x6F, 0x72, 0x65, 0x20, 0x43, 
//         0x79, 0x62, 0x65, 0x72, 0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x30, 0x1E, 
//         0x17, 0x0D, 0x30, 0x30, 0x30, 0x35, 0x31, 0x32, 0x31, 0x38, 0x34, 0x36, 0x30, 0x30, 0x5A, 0x17, 
//         0x0D, 0x32, 0x35, 0x30, 0x35, 0x31, 0x32, 0x32, 0x33, 0x35, 0x39, 0x30, 0x30, 0x5A, 0x30, 0x5A, 
//         0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x49, 0x45, 0x31, 0x12, 0x30, 
//         0x10, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x09, 0x42, 0x61, 0x6C, 0x74, 0x69, 0x6D, 0x6F, 0x72, 
//         0x65, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x13, 0x0A, 0x43, 0x79, 0x62, 0x65, 
//         0x72, 0x54, 0x72, 0x75, 0x73, 0x74, 0x31, 0x22, 0x30, 0x20, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 
//         0x19, 0x42, 0x61, 0x6C, 0x74, 0x69, 0x6D, 0x6F, 0x72, 0x65, 0x20, 0x43, 0x79, 0x62, 0x65, 0x72, 
//         0x54, 0x72, 0x75, 0x73, 0x74, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 
//         0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 
//         0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xA3, 0x04, 0xBB, 0x22, 0xAB, 
//         0x98, 0x3D, 0x57, 0xE8, 0x26, 0x72, 0x9A, 0xB5, 0x79, 0xD4, 0x29, 0xE2, 0xE1, 0xE8, 0x95, 0x80, 
//         0xB1, 0xB0, 0xE3, 0x5B, 0x8E, 0x2B, 0x29, 0x9A, 0x64, 0xDF, 0xA1, 0x5D, 0xED, 0xB0, 0x09, 0x05, 
//         0x6D, 0xDB, 0x28, 0x2E, 0xCE, 0x62, 0xA2, 0x62, 0xFE, 0xB4, 0x88, 0xDA, 0x12, 0xEB, 0x38, 0xEB, 
//         0x21, 0x9D, 0xC0, 0x41, 0x2B, 0x01, 0x52, 0x7B, 0x88, 0x77, 0xD3, 0x1C, 0x8F, 0xC7, 0xBA, 0xB9, 
//         0x88, 0xB5, 0x6A, 0x09, 0xE7, 0x73, 0xE8, 0x11, 0x40, 0xA7, 0xD1, 0xCC, 0xCA, 0x62, 0x8D, 0x2D, 
//         0xE5, 0x8F, 0x0B, 0xA6, 0x50, 0xD2, 0xA8, 0x50, 0xC3, 0x28, 0xEA, 0xF5, 0xAB, 0x25, 0x87, 0x8A, 
//         0x9A, 0x96, 0x1C, 0xA9, 0x67, 0xB8, 0x3F, 0x0C, 0xD5, 0xF7, 0xF9, 0x52, 0x13, 0x2F, 0xC2, 0x1B, 
//         0xD5, 0x70, 0x70, 0xF0, 0x8F, 0xC0, 0x12, 0xCA, 0x06, 0xCB, 0x9A, 0xE1, 0xD9, 0xCA, 0x33, 0x7A, 
//         0x77, 0xD6, 0xF8, 0xEC, 0xB9, 0xF1, 0x68, 0x44, 0x42, 0x48, 0x13, 0xD2, 0xC0, 0xC2, 0xA4, 0xAE, 
//         0x5E, 0x60, 0xFE, 0xB6, 0xA6, 0x05, 0xFC, 0xB4, 0xDD, 0x07, 0x59, 0x02, 0xD4, 0x59, 0x18, 0x98, 
//         0x63, 0xF5, 0xA5, 0x63, 0xE0, 0x90, 0x0C, 0x7D, 0x5D, 0xB2, 0x06, 0x7A, 0xF3, 0x85, 0xEA, 0xEB, 
//         0xD4, 0x03, 0xAE, 0x5E, 0x84, 0x3E, 0x5F, 0xFF, 0x15, 0xED, 0x69, 0xBC, 0xF9, 0x39, 0x36, 0x72, 
//         0x75, 0xCF, 0x77, 0x52, 0x4D, 0xF3, 0xC9, 0x90, 0x2C, 0xB9, 0x3D, 0xE5, 0xC9, 0x23, 0x53, 0x3F, 
//         0x1F, 0x24, 0x98, 0x21, 0x5C, 0x07, 0x99, 0x29, 0xBD, 0xC6, 0x3A, 0xEC, 0xE7, 0x6E, 0x86, 0x3A, 
//         0x6B, 0x97, 0x74, 0x63, 0x33, 0xBD, 0x68, 0x18, 0x31, 0xF0, 0x78, 0x8D, 0x76, 0xBF, 0xFC, 0x9E, 
//         0x8E, 0x5D, 0x2A, 0x86, 0xA7, 0x4D, 0x90, 0xDC, 0x27, 0x1A, 0x39, 0x02, 0x03, 0x01, 0x00, 0x01, 
//         0xA3, 0x45, 0x30, 0x43, 0x30, 0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0xE5, 
//         0x9D, 0x59, 0x30, 0x82, 0x47, 0x58, 0xCC, 0xAC, 0xFA, 0x08, 0x54, 0x36, 0x86, 0x7B, 0x3A, 0xB5, 
//         0x04, 0x4D, 0xF0, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x01, 0x01, 0xFF, 0x04, 0x08, 0x30, 
//         0x06, 0x01, 0x01, 0xFF, 0x02, 0x01, 0x03, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x1D, 0x0F, 0x01, 0x01, 
//         0xFF, 0x04, 0x04, 0x03, 0x02, 0x01, 0x06, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 
//         0x0D, 0x01, 0x01, 0x05, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x85, 0x0C, 0x5D, 0x8E, 0xE4, 
//         0x6F, 0x51, 0x68, 0x42, 0x05, 0xA0, 0xDD, 0xBB, 0x4F, 0x27, 0x25, 0x84, 0x03, 0xBD, 0xF7, 0x64, 
//         0xFD, 0x2D, 0xD7, 0x30, 0xE3, 0xA4, 0x10, 0x17, 0xEB, 0xDA, 0x29, 0x29, 0xB6, 0x79, 0x3F, 0x76, 
//         0xF6, 0x19, 0x13, 0x23, 0xB8, 0x10, 0x0A, 0xF9, 0x58, 0xA4, 0xD4, 0x61, 0x70, 0xBD, 0x04, 0x61, 
//         0x6A, 0x12, 0x8A, 0x17, 0xD5, 0x0A, 0xBD, 0xC5, 0xBC, 0x30, 0x7C, 0xD6, 0xE9, 0x0C, 0x25, 0x8D, 
//         0x86, 0x40, 0x4F, 0xEC, 0xCC, 0xA3, 0x7E, 0x38, 0xC6, 0x37, 0x11, 0x4F, 0xED, 0xDD, 0x68, 0x31, 
//         0x8E, 0x4C, 0xD2, 0xB3, 0x01, 0x74, 0xEE, 0xBE, 0x75, 0x5E, 0x07, 0x48, 0x1A, 0x7F, 0x70, 0xFF, 
//         0x16, 0x5C, 0x84, 0xC0, 0x79, 0x85, 0xB8, 0x05, 0xFD, 0x7F, 0xBE, 0x65, 0x11, 0xA3, 0x0F, 0xC0, 
//         0x02, 0xB4, 0xF8, 0x52, 0x37, 0x39, 0x04, 0xD5, 0xA9, 0x31, 0x7A, 0x18, 0xBF, 0xA0, 0x2A, 0xF4, 
//         0x12, 0x99, 0xF7, 0xA3, 0x45, 0x82, 0xE3, 0x3C, 0x5E, 0xF5, 0x9D, 0x9E, 0xB5, 0xC8, 0x9E, 0x7C, 
//         0x2E, 0xC8, 0xA4, 0x9E, 0x4E, 0x08, 0x14, 0x4B, 0x6D, 0xFD, 0x70, 0x6D, 0x6B, 0x1A, 0x63, 0xBD, 
//         0x64, 0xE6, 0x1F, 0xB7, 0xCE, 0xF0, 0xF2, 0x9F, 0x2E, 0xBB, 0x1B, 0xB7, 0xF2, 0x50, 0x88, 0x73, 
//         0x92, 0xC2, 0xE2, 0xE3, 0x16, 0x8D, 0x9A, 0x32, 0x02, 0xAB, 0x8E, 0x18, 0xDD, 0xE9, 0x10, 0x11, 
//         0xEE, 0x7E, 0x35, 0xAB, 0x90, 0xAF, 0x3E, 0x30, 0x94, 0x7A, 0xD0, 0x33, 0x3D, 0xA7, 0x65, 0x0F, 
//         0xF5, 0xFC, 0x8E, 0x9E, 0x62, 0xCF, 0x47, 0x44, 0x2C, 0x01, 0x5D, 0xBB, 0x1D, 0xB5, 0x32, 0xD2, 
//         0x47, 0xD2, 0x38, 0x2E, 0xD0, 0xFE, 0x81, 0xDC, 0x32, 0x6A, 0x1E, 0xB5, 0xEE, 0x3C, 0xD5, 0xFC, 
//         0xE7, 0x81, 0x1D, 0x19, 0xC3, 0x24, 0x42, 0xEA, 0x63, 0x39, 0xA9, 
//     };
//         write_data_object(CONFIG_OPTIGA_TRUST_M_TRUSTANCHOR_SLOT, trust_anchor, sizeof(trust_anchor));
// #endif //Baltimore
// #if 0
//     /* DigiCert Global Root CA */
//     // This cert should be used when connecting to Azure IoT on the https://portal.azure.cn Cloud address.
    
//     const uint8_t trust_anchor[] = {
//         0x30, 0x82, 0x03, 0xAF, 0x30, 0x82, 0x02, 0x97, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x08, 
//         0x3B, 0xE0, 0x56, 0x90, 0x42, 0x46, 0xB1, 0xA1, 0x75, 0x6A, 0xC9, 0x59, 0x91, 0xC7, 0x4A, 0x30, 
//         0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x61, 
//         0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x15, 0x30, 
//         0x13, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x0C, 0x44, 0x69, 0x67, 0x69, 0x43, 0x65, 0x72, 0x74, 
//         0x20, 0x49, 0x6E, 0x63, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x13, 0x10, 0x77, 
//         0x77, 0x77, 0x2E, 0x64, 0x69, 0x67, 0x69, 0x63, 0x65, 0x72, 0x74, 0x2E, 0x63, 0x6F, 0x6D, 0x31, 
//         0x20, 0x30, 0x1E, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x17, 0x44, 0x69, 0x67, 0x69, 0x43, 0x65, 
//         0x72, 0x74, 0x20, 0x47, 0x6C, 0x6F, 0x62, 0x61, 0x6C, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 
//         0x41, 0x30, 0x1E, 0x17, 0x0D, 0x30, 0x36, 0x31, 0x31, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 
//         0x30, 0x5A, 0x17, 0x0D, 0x33, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 
//         0x5A, 0x30, 0x61, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 
//         0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x0C, 0x44, 0x69, 0x67, 0x69, 0x43, 
//         0x65, 0x72, 0x74, 0x20, 0x49, 0x6E, 0x63, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04, 0x0B, 
//         0x13, 0x10, 0x77, 0x77, 0x77, 0x2E, 0x64, 0x69, 0x67, 0x69, 0x63, 0x65, 0x72, 0x74, 0x2E, 0x63, 
//         0x6F, 0x6D, 0x31, 0x20, 0x30, 0x1E, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x17, 0x44, 0x69, 0x67, 
//         0x69, 0x43, 0x65, 0x72, 0x74, 0x20, 0x47, 0x6C, 0x6F, 0x62, 0x61, 0x6C, 0x20, 0x52, 0x6F, 0x6F, 
//         0x74, 0x20, 0x43, 0x41, 0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 
//         0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 
//         0x02, 0x82, 0x01, 0x01, 0x00, 0xE2, 0x3B, 0xE1, 0x11, 0x72, 0xDE, 0xA8, 0xA4, 0xD3, 0xA3, 0x57, 
//         0xAA, 0x50, 0xA2, 0x8F, 0x0B, 0x77, 0x90, 0xC9, 0xA2, 0xA5, 0xEE, 0x12, 0xCE, 0x96, 0x5B, 0x01, 
//         0x09, 0x20, 0xCC, 0x01, 0x93, 0xA7, 0x4E, 0x30, 0xB7, 0x53, 0xF7, 0x43, 0xC4, 0x69, 0x00, 0x57, 
//         0x9D, 0xE2, 0x8D, 0x22, 0xDD, 0x87, 0x06, 0x40, 0x00, 0x81, 0x09, 0xCE, 0xCE, 0x1B, 0x83, 0xBF, 
//         0xDF, 0xCD, 0x3B, 0x71, 0x46, 0xE2, 0xD6, 0x66, 0xC7, 0x05, 0xB3, 0x76, 0x27, 0x16, 0x8F, 0x7B, 
//         0x9E, 0x1E, 0x95, 0x7D, 0xEE, 0xB7, 0x48, 0xA3, 0x08, 0xDA, 0xD6, 0xAF, 0x7A, 0x0C, 0x39, 0x06, 
//         0x65, 0x7F, 0x4A, 0x5D, 0x1F, 0xBC, 0x17, 0xF8, 0xAB, 0xBE, 0xEE, 0x28, 0xD7, 0x74, 0x7F, 0x7A, 
//         0x78, 0x99, 0x59, 0x85, 0x68, 0x6E, 0x5C, 0x23, 0x32, 0x4B, 0xBF, 0x4E, 0xC0, 0xE8, 0x5A, 0x6D, 
//         0xE3, 0x70, 0xBF, 0x77, 0x10, 0xBF, 0xFC, 0x01, 0xF6, 0x85, 0xD9, 0xA8, 0x44, 0x10, 0x58, 0x32, 
//         0xA9, 0x75, 0x18, 0xD5, 0xD1, 0xA2, 0xBE, 0x47, 0xE2, 0x27, 0x6A, 0xF4, 0x9A, 0x33, 0xF8, 0x49, 
//         0x08, 0x60, 0x8B, 0xD4, 0x5F, 0xB4, 0x3A, 0x84, 0xBF, 0xA1, 0xAA, 0x4A, 0x4C, 0x7D, 0x3E, 0xCF, 
//         0x4F, 0x5F, 0x6C, 0x76, 0x5E, 0xA0, 0x4B, 0x37, 0x91, 0x9E, 0xDC, 0x22, 0xE6, 0x6D, 0xCE, 0x14, 
//         0x1A, 0x8E, 0x6A, 0xCB, 0xFE, 0xCD, 0xB3, 0x14, 0x64, 0x17, 0xC7, 0x5B, 0x29, 0x9E, 0x32, 0xBF, 
//         0xF2, 0xEE, 0xFA, 0xD3, 0x0B, 0x42, 0xD4, 0xAB, 0xB7, 0x41, 0x32, 0xDA, 0x0C, 0xD4, 0xEF, 0xF8, 
//         0x81, 0xD5, 0xBB, 0x8D, 0x58, 0x3F, 0xB5, 0x1B, 0xE8, 0x49, 0x28, 0xA2, 0x70, 0xDA, 0x31, 0x04, 
//         0xDD, 0xF7, 0xB2, 0x16, 0xF2, 0x4C, 0x0A, 0x4E, 0x07, 0xA8, 0xED, 0x4A, 0x3D, 0x5E, 0xB5, 0x7F, 
//         0xA3, 0x90, 0xC3, 0xAF, 0x27, 0x02, 0x03, 0x01, 0x00, 0x01, 0xA3, 0x63, 0x30, 0x61, 0x30, 0x0E, 
//         0x06, 0x03, 0x55, 0x1D, 0x0F, 0x01, 0x01, 0xFF, 0x04, 0x04, 0x03, 0x02, 0x01, 0x86, 0x30, 0x0F, 
//         0x06, 0x03, 0x55, 0x1D, 0x13, 0x01, 0x01, 0xFF, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 
//         0x1D, 0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x03, 0xDE, 0x50, 0x35, 0x56, 0xD1, 
//         0x4C, 0xBB, 0x66, 0xF0, 0xA3, 0xE2, 0x1B, 0x1B, 0xC3, 0x97, 0xB2, 0x3D, 0xD1, 0x55, 0x30, 0x1F, 
//         0x06, 0x03, 0x55, 0x1D, 0x23, 0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x03, 0xDE, 0x50, 0x35, 0x56, 
//         0xD1, 0x4C, 0xBB, 0x66, 0xF0, 0xA3, 0xE2, 0x1B, 0x1B, 0xC3, 0x97, 0xB2, 0x3D, 0xD1, 0x55, 0x30, 
//         0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05, 0x05, 0x00, 0x03, 0x82, 
//         0x01, 0x01, 0x00, 0xCB, 0x9C, 0x37, 0xAA, 0x48, 0x13, 0x12, 0x0A, 0xFA, 0xDD, 0x44, 0x9C, 0x4F, 
//         0x52, 0xB0, 0xF4, 0xDF, 0xAE, 0x04, 0xF5, 0x79, 0x79, 0x08, 0xA3, 0x24, 0x18, 0xFC, 0x4B, 0x2B, 
//         0x84, 0xC0, 0x2D, 0xB9, 0xD5, 0xC7, 0xFE, 0xF4, 0xC1, 0x1F, 0x58, 0xCB, 0xB8, 0x6D, 0x9C, 0x7A, 
//         0x74, 0xE7, 0x98, 0x29, 0xAB, 0x11, 0xB5, 0xE3, 0x70, 0xA0, 0xA1, 0xCD, 0x4C, 0x88, 0x99, 0x93, 
//         0x8C, 0x91, 0x70, 0xE2, 0xAB, 0x0F, 0x1C, 0xBE, 0x93, 0xA9, 0xFF, 0x63, 0xD5, 0xE4, 0x07, 0x60, 
//         0xD3, 0xA3, 0xBF, 0x9D, 0x5B, 0x09, 0xF1, 0xD5, 0x8E, 0xE3, 0x53, 0xF4, 0x8E, 0x63, 0xFA, 0x3F, 
//         0xA7, 0xDB, 0xB4, 0x66, 0xDF, 0x62, 0x66, 0xD6, 0xD1, 0x6E, 0x41, 0x8D, 0xF2, 0x2D, 0xB5, 0xEA, 
//         0x77, 0x4A, 0x9F, 0x9D, 0x58, 0xE2, 0x2B, 0x59, 0xC0, 0x40, 0x23, 0xED, 0x2D, 0x28, 0x82, 0x45, 
//         0x3E, 0x79, 0x54, 0x92, 0x26, 0x98, 0xE0, 0x80, 0x48, 0xA8, 0x37, 0xEF, 0xF0, 0xD6, 0x79, 0x60, 
//         0x16, 0xDE, 0xAC, 0xE8, 0x0E, 0xCD, 0x6E, 0xAC, 0x44, 0x17, 0x38, 0x2F, 0x49, 0xDA, 0xE1, 0x45, 
//         0x3E, 0x2A, 0xB9, 0x36, 0x53, 0xCF, 0x3A, 0x50, 0x06, 0xF7, 0x2E, 0xE8, 0xC4, 0x57, 0x49, 0x6C, 
//         0x61, 0x21, 0x18, 0xD5, 0x04, 0xAD, 0x78, 0x3C, 0x2C, 0x3A, 0x80, 0x6B, 0xA7, 0xEB, 0xAF, 0x15, 
//         0x14, 0xE9, 0xD8, 0x89, 0xC1, 0xB9, 0x38, 0x6C, 0xE2, 0x91, 0x6C, 0x8A, 0xFF, 0x64, 0xB9, 0x77, 
//         0x25, 0x57, 0x30, 0xC0, 0x1B, 0x24, 0xA3, 0xE1, 0xDC, 0xE9, 0xDF, 0x47, 0x7C, 0xB5, 0xB4, 0x24, 
//         0x08, 0x05, 0x30, 0xEC, 0x2D, 0xBD, 0x0B, 0xBF, 0x45, 0xBF, 0x50, 0xB9, 0xA9, 0xF3, 0xEB, 0x98, 
//         0x01, 0x12, 0xAD, 0xC8, 0x88, 0xC6, 0x98, 0x34, 0x5F, 0x8D, 0x0A, 0x3C, 0xC6, 0xE9, 0xD5, 0x95, 
//         0x95, 0x6D, 0xDE, 
//     };
//         write_data_object(CONFIG_OPTIGA_TRUST_M_TRUSTANCHOR_SLOT, trust_anchor, sizeof(trust_anchor));
// #endif //DigiCert Global Root CA
// #if 0
//     /* D-TRUST Root Class 3 CA 2 2009 */
//     // This cert should be used when connecting to Azure IoT on the https://portal.microsoftazure.de Cloud address.    
//     const uint8_t trust_anchor[] ={
//         0x30, 0x82, 0x04, 0x33, 0x30, 0x82, 0x03, 0x1B, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x03, 0x09, 
//         0x83, 0xF3, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x05, 
//         0x00, 0x30, 0x4D, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x44, 0x45, 
//         0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x0C, 0x44, 0x2D, 0x54, 0x72, 0x75, 
//         0x73, 0x74, 0x20, 0x47, 0x6D, 0x62, 0x48, 0x31, 0x27, 0x30, 0x25, 0x06, 0x03, 0x55, 0x04, 0x03, 
//         0x0C, 0x1E, 0x44, 0x2D, 0x54, 0x52, 0x55, 0x53, 0x54, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 
//         0x6C, 0x61, 0x73, 0x73, 0x20, 0x33, 0x20, 0x43, 0x41, 0x20, 0x32, 0x20, 0x32, 0x30, 0x30, 0x39, 
//         0x30, 0x1E, 0x17, 0x0D, 0x30, 0x39, 0x31, 0x31, 0x30, 0x35, 0x30, 0x38, 0x33, 0x35, 0x35, 0x38, 
//         0x5A, 0x17, 0x0D, 0x32, 0x39, 0x31, 0x31, 0x30, 0x35, 0x30, 0x38, 0x33, 0x35, 0x35, 0x38, 0x5A, 
//         0x30, 0x4D, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x44, 0x45, 0x31, 
//         0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x0C, 0x44, 0x2D, 0x54, 0x72, 0x75, 0x73, 
//         0x74, 0x20, 0x47, 0x6D, 0x62, 0x48, 0x31, 0x27, 0x30, 0x25, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 
//         0x1E, 0x44, 0x2D, 0x54, 0x52, 0x55, 0x53, 0x54, 0x20, 0x52, 0x6F, 0x6F, 0x74, 0x20, 0x43, 0x6C, 
//         0x61, 0x73, 0x73, 0x20, 0x33, 0x20, 0x43, 0x41, 0x20, 0x32, 0x20, 0x32, 0x30, 0x30, 0x39, 0x30, 
//         0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01, 
//         0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00, 0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 
//         0xD3, 0xB2, 0x4A, 0xCF, 0x7A, 0x47, 0xEF, 0x75, 0x9B, 0x23, 0xFA, 0x3A, 0x2F, 0xD6, 0x50, 0x45, 
//         0x89, 0x35, 0x3A, 0xC6, 0x6B, 0xDB, 0xFE, 0xDB, 0x00, 0x68, 0xA8, 0xE0, 0x03, 0x11, 0x1D, 0x37, 
//         0x50, 0x08, 0x9F, 0x4D, 0x4A, 0x68, 0x94, 0x35, 0xB3, 0x53, 0xD1, 0x94, 0x63, 0xA7, 0x20, 0x56, 
//         0xAF, 0xDE, 0x51, 0x78, 0xEC, 0x2A, 0x3D, 0xF3, 0x48, 0x48, 0x50, 0x3E, 0x0A, 0xDF, 0x46, 0x55, 
//         0x8B, 0x27, 0x6D, 0xC3, 0x10, 0x4D, 0x0D, 0x91, 0x52, 0x43, 0xD8, 0x87, 0xE0, 0x5D, 0x4E, 0x36, 
//         0xB5, 0x21, 0xCA, 0x5F, 0x39, 0x40, 0x04, 0x5F, 0x5B, 0x7E, 0xCC, 0xA3, 0xC6, 0x2B, 0xA9, 0x40, 
//         0x1E, 0xD9, 0x36, 0x84, 0xD6, 0x48, 0xF3, 0x92, 0x1E, 0x34, 0x46, 0x20, 0x24, 0xC1, 0xA4, 0x51, 
//         0x8E, 0x4A, 0x1A, 0xEF, 0x50, 0x3F, 0x69, 0x5D, 0x19, 0x7F, 0x45, 0xC3, 0xC7, 0x01, 0x8F, 0x51, 
//         0xC9, 0x23, 0xE8, 0x72, 0xAE, 0xB4, 0xBC, 0x56, 0x09, 0x7F, 0x12, 0xCB, 0x1C, 0xB1, 0xAF, 0x29, 
//         0x90, 0x0A, 0xC9, 0x55, 0xCC, 0x0F, 0xD3, 0xB4, 0x1A, 0xED, 0x47, 0x35, 0x5A, 0x4A, 0xED, 0x9C, 
//         0x73, 0x04, 0x21, 0xD0, 0xAA, 0xBD, 0x0C, 0x13, 0xB5, 0x00, 0xCA, 0x26, 0x6C, 0xC4, 0x6B, 0x0C, 
//         0x94, 0x5A, 0x95, 0x94, 0xDA, 0x50, 0x9A, 0xF1, 0xFF, 0xA5, 0x2B, 0x66, 0x31, 0xA4, 0xC9, 0x38, 
//         0xA0, 0xDF, 0x1D, 0x1F, 0xB8, 0x09, 0x2E, 0xF3, 0xA7, 0xE8, 0x67, 0x52, 0xAB, 0x95, 0x1F, 0xE0, 
//         0x46, 0x3E, 0xD8, 0xA4, 0xC3, 0xCA, 0x5A, 0xC5, 0x31, 0x80, 0xE8, 0x48, 0x9A, 0x9F, 0x94, 0x69, 
//         0xFE, 0x19, 0xDD, 0xD8, 0x73, 0x7C, 0x81, 0xCA, 0x96, 0xDE, 0x8E, 0xED, 0xB3, 0x32, 0x05, 0x65, 
//         0x84, 0x34, 0xE6, 0xE6, 0xFD, 0x57, 0x10, 0xB5, 0x5F, 0x76, 0xBF, 0x2F, 0xB0, 0x10, 0x0D, 0xC5, 
//         0x02, 0x03, 0x01, 0x00, 0x01, 0xA3, 0x82, 0x01, 0x1A, 0x30, 0x82, 0x01, 0x16, 0x30, 0x0F, 0x06, 
//         0x03, 0x55, 0x1D, 0x13, 0x01, 0x01, 0xFF, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x1D, 
//         0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0xFD, 0xDA, 0x14, 0xC4, 0x9F, 0x30, 0xDE, 
//         0x21, 0xBD, 0x1E, 0x42, 0x39, 0xFC, 0xAB, 0x63, 0x23, 0x49, 0xE0, 0xF1, 0x84, 0x30, 0x0E, 0x06, 
//         0x03, 0x55, 0x1D, 0x0F, 0x01, 0x01, 0xFF, 0x04, 0x04, 0x03, 0x02, 0x01, 0x06, 0x30, 0x81, 0xD3, 
//         0x06, 0x03, 0x55, 0x1D, 0x1F, 0x04, 0x81, 0xCB, 0x30, 0x81, 0xC8, 0x30, 0x81, 0x80, 0xA0, 0x7E, 
//         0xA0, 0x7C, 0x86, 0x7A, 0x6C, 0x64, 0x61, 0x70, 0x3A, 0x2F, 0x2F, 0x64, 0x69, 0x72, 0x65, 0x63, 
//         0x74, 0x6F, 0x72, 0x79, 0x2E, 0x64, 0x2D, 0x74, 0x72, 0x75, 0x73, 0x74, 0x2E, 0x6E, 0x65, 0x74, 
//         0x2F, 0x43, 0x4E, 0x3D, 0x44, 0x2D, 0x54, 0x52, 0x55, 0x53, 0x54, 0x25, 0x32, 0x30, 0x52, 0x6F, 
//         0x6F, 0x74, 0x25, 0x32, 0x30, 0x43, 0x6C, 0x61, 0x73, 0x73, 0x25, 0x32, 0x30, 0x33, 0x25, 0x32, 
//         0x30, 0x43, 0x41, 0x25, 0x32, 0x30, 0x32, 0x25, 0x32, 0x30, 0x32, 0x30, 0x30, 0x39, 0x2C, 0x4F, 
//         0x3D, 0x44, 0x2D, 0x54, 0x72, 0x75, 0x73, 0x74, 0x25, 0x32, 0x30, 0x47, 0x6D, 0x62, 0x48, 0x2C, 
//         0x43, 0x3D, 0x44, 0x45, 0x3F, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 
//         0x72, 0x65, 0x76, 0x6F, 0x63, 0x61, 0x74, 0x69, 0x6F, 0x6E, 0x6C, 0x69, 0x73, 0x74, 0x30, 0x43, 
//         0xA0, 0x41, 0xA0, 0x3F, 0x86, 0x3D, 0x68, 0x74, 0x74, 0x70, 0x3A, 0x2F, 0x2F, 0x77, 0x77, 0x77, 
//         0x2E, 0x64, 0x2D, 0x74, 0x72, 0x75, 0x73, 0x74, 0x2E, 0x6E, 0x65, 0x74, 0x2F, 0x63, 0x72, 0x6C, 
//         0x2F, 0x64, 0x2D, 0x74, 0x72, 0x75, 0x73, 0x74, 0x5F, 0x72, 0x6F, 0x6F, 0x74, 0x5F, 0x63, 0x6C, 
//         0x61, 0x73, 0x73, 0x5F, 0x33, 0x5F, 0x63, 0x61, 0x5F, 0x32, 0x5F, 0x32, 0x30, 0x30, 0x39, 0x2E, 
//         0x63, 0x72, 0x6C, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B, 
//         0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x7F, 0x97, 0xDB, 0x30, 0xC8, 0xDF, 0xA4, 0x9C, 0x7D, 
//         0x21, 0x7A, 0x80, 0x70, 0xCE, 0x14, 0x12, 0x69, 0x88, 0x14, 0x95, 0x60, 0x44, 0x01, 0xAC, 0xB2, 
//         0xE9, 0x30, 0x4F, 0x9B, 0x50, 0xC2, 0x66, 0xD8, 0x7E, 0x8D, 0x30, 0xB5, 0x70, 0x31, 0xE9, 0xE2, 
//         0x69, 0xC7, 0xF3, 0x70, 0xDB, 0x20, 0x15, 0x86, 0xD0, 0x0D, 0xF0, 0xBE, 0xAC, 0x01, 0x75, 0x84, 
//         0xCE, 0x7E, 0x9F, 0x4D, 0xBF, 0xB7, 0x60, 0x3B, 0x9C, 0xF3, 0xCA, 0x1D, 0xE2, 0x5E, 0x68, 0xD8, 
//         0xA3, 0x9D, 0x97, 0xE5, 0x40, 0x60, 0xD2, 0x36, 0x21, 0xFE, 0xD0, 0xB4, 0xB8, 0x17, 0xDA, 0x74, 
//         0xA3, 0x7F, 0xD4, 0xDF, 0xB0, 0x98, 0x02, 0xAC, 0x6F, 0x6B, 0x6B, 0x2C, 0x25, 0x24, 0x72, 0xA1, 
//         0x65, 0xEE, 0x25, 0x5A, 0xE5, 0xE6, 0x32, 0xE7, 0xF2, 0xDF, 0xAB, 0x49, 0xFA, 0xF3, 0x90, 0x69, 
//         0x23, 0xDB, 0x04, 0xD9, 0xE7, 0x5C, 0x58, 0xFC, 0x65, 0xD4, 0x97, 0xBE, 0xCC, 0xFC, 0x2E, 0x0A, 
//         0xCC, 0x25, 0x2A, 0x35, 0x04, 0xF8, 0x60, 0x91, 0x15, 0x75, 0x3D, 0x41, 0xFF, 0x23, 0x1F, 0x19, 
//         0xC8, 0x6C, 0xEB, 0x82, 0x53, 0x04, 0xA6, 0xE4, 0x4C, 0x22, 0x4D, 0x8D, 0x8C, 0xBA, 0xCE, 0x5B, 
//         0x73, 0xEC, 0x64, 0x54, 0x50, 0x6D, 0xD1, 0x9C, 0x55, 0xFB, 0x69, 0xC3, 0x36, 0xC3, 0x8C, 0xBC, 
//         0x3C, 0x85, 0xA6, 0x6B, 0x0A, 0x26, 0x0D, 0xE0, 0x93, 0x98, 0x60, 0xAE, 0x7E, 0xC6, 0x24, 0x97, 
//         0x8A, 0x61, 0x5F, 0x91, 0x8E, 0x66, 0x92, 0x09, 0x87, 0x36, 0xCD, 0x8B, 0x9B, 0x2D, 0x3E, 0xF6, 
//         0x51, 0xD4, 0x50, 0xD4, 0x59, 0x28, 0xBD, 0x83, 0xF2, 0xCC, 0x28, 0x7B, 0x53, 0x86, 0x6D, 0xD8, 
//         0x26, 0x88, 0x70, 0xD7, 0xEA, 0x91, 0xCD, 0x3E, 0xB9, 0xCA, 0xC0, 0x90, 0x6E, 0x5A, 0xC6, 0x5E, 
//         0x74, 0x65, 0xD7, 0x5C, 0xFE, 0xA3, 0xE2,         
//     };
//     write_data_object(CONFIG_OPTIGA_TRUST_M_TRUSTANCHOR_SLOT, trust_anchor, sizeof(trust_anchor));
// #endif //D-TRUST Root Class 3 CA 2 2009
// #if 0

//     /* User can provide a specific server certificate here and can load in any data object of optiga */
//     const uint8_t trust_anchor[] = {
        
//                        //provide region specific server root CA certificate  
//     };
//      write_data_object(CONFIG_OPTIGA_TRUST_M_TRUSTANCHOR_SLOT, trust_anchor, sizeof(trust_anchor));
// #endif //Region Specific Certificate 
// }

// static void write_platform_binding_secret (void)
// {
//     //Platform binding shared secret for example purpose
//     const uint8_t platform_binding_shared_secret [] = {
//         0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
//         0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
//         0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 
//         0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
//         0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 
//         0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
//         0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
//         0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40
//     };
    
//     write_data_object (0xE140, platform_binding_shared_secret, sizeof(platform_binding_shared_secret));
// }

static void write_set_high_performance (void)
{
    const uint8_t current_limit [] = {
    0x0F,
    };
    
    write_data_object (0xE0C4, current_limit, sizeof(current_limit));
}

void optiga_trust_init(void)
{
    optiga_lib_status_t return_status;
    optiga_util_t * me_util = NULL;

    optiga_lib_print_message("OPTIGA Trust initialization",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
    
    do
    {
        //Create an instance of optiga_util to open the application on OPTIGA.
        me_util = optiga_util_create(0, optiga_util_callback, NULL);
        if(!me_util)
        {
            optiga_lib_print_message("optiga_util_create failed !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
            break;
        }

        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_util_open_application(me_util, 0);
        {
            if (OPTIGA_LIB_SUCCESS != return_status)
            {
                //optiga_util_open_application api returns error !!!
                optiga_lib_print_message("optiga_util_open_application api returns error !!!",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
                break;
            }
            
            while (optiga_lib_status == OPTIGA_LIB_BUSY);
            if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
            {
                //optiga_util_open_application failed
                optiga_lib_print_message("optiga_util_open_application failed",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
                break;
            }
        }

        //The below specified functions can be used to personalize OPTIGA w.r.t
        //certificates, Trust Anchors, etc.
        
        //write_device_certificate ();
        write_set_high_performance();  //setting current limitation to 15mA
        //write_platform_binding_secret ();  
        //write_optiga_trust_anchor();  //can be used to write server root certificate to optiga data object

        optiga_lib_print_message("OPTIGA Trust initialization is successful",OPTIGA_UTIL_SERVICE,OPTIGA_UTIL_SERVICE_COLOR);
    }while(0);

    //me_util instance can be destroyed 
    //if no close_application w.r.t hibernate is required to be performed
    if (me_util)
    {
        optiga_util_destroy(me_util);
    }
}

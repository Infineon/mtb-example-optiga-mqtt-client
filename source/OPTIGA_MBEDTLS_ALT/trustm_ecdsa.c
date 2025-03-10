/******************************************************************************
* File Name:   trustm_ecdsa.c
*
* Description: This file contains alternative function implementation for 
*              mbedtls. In order to use them define in your project the
*              Following macros: 
*              MBEDTLS_ECDSA_SIGN_ALT
*              MBEDTLS_ECDSA_VERIFY_ALT
*              MBEDTLS_ECDSA_GENKEY_ALT
*              Define in your project CONFIG_OPTIGA_TRUST_M_PRIVKEY_SLOT
*              to enable the usage other than 0xe0f0 Key Object ID
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
#include "mbedtls/config.h"

#if defined(MBEDTLS_ECDSA_C)

#include "mbedtls/ecdsa.h"
#include "mbedtls/asn1write.h"

#include <string.h>
#include "include/optiga_crypt.h"
#include "include/optiga_util.h"
#include "include/pal/pal_os_timer.h"
#include "include/common/optiga_lib_common.h"

#define PRINT_SIGNATURE   0
#define PRINT_HASH        0
#define PRINT_PUBLICKEY   0

#ifndef CONFIG_OPTIGA_TRUST_M_PRIVKEY_SLOT
#define CONFIG_OPTIGA_TRUST_M_PRIVKEY_SLOT OPTIGA_KEY_ID_E0F0
#endif

optiga_lib_status_t ecdsa_completed_status;

//lint --e{818} suppress "argument "context" is not used in the sample provided"
static void optiga_crypt_event_completed(void * context, optiga_lib_status_t return_status)
{
    ecdsa_completed_status = return_status;
    if (NULL != context)
    {
        // callback to upper layer here
    }
}

#if defined(MBEDTLS_ECDSA_SIGN_ALT)
int mbedtls_ecdsa_sign( mbedtls_ecp_group *grp, mbedtls_mpi *r, mbedtls_mpi *s,
                const mbedtls_mpi *d, const unsigned char *buf, size_t blen,
                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{

    int return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    uint8_t der_signature[150];
    uint16_t dslen = sizeof(der_signature);
    uint8_t *p = der_signature;
    const uint8_t * end = NULL;
    optiga_crypt_t * me = NULL;
    optiga_lib_status_t crypt_sync_status = OPTIGA_CRYPT_ERROR;

    end = (der_signature + dslen);
    memset(der_signature, 0x00, sizeof(der_signature));

    me = optiga_crypt_create(0, optiga_crypt_event_completed, NULL);
    if (NULL == me)
    {
        return_status = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

#if (PRINT_HASH==1)
    for(int x=0; x<blen;)
    {
        printf(("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\r\n",
                buf[x],buf[x+1],buf[x+2],buf[x+3],
                buf[x+4],buf[x+5],buf[x+6],buf[x+7]));
        x+=8;
    }
#endif
    // Reset the status variable (updated via callback, when requested operation is finiesh, or timeout)
    ecdsa_completed_status = OPTIGA_LIB_BUSY;

    // Signing data with the Secure Element
    crypt_sync_status = optiga_crypt_ecdsa_sign(me, (unsigned char *)buf, blen, (optiga_key_id_t)CONFIG_OPTIGA_TRUST_M_PRIVKEY_SLOT, der_signature, &dslen);
    if(OPTIGA_LIB_SUCCESS != crypt_sync_status)
    {
        return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    //Wait until the optiga_crypt_ecdsa_verify is completed
    while (OPTIGA_LIB_BUSY == ecdsa_completed_status)
    {
        pal_os_timer_delay_in_milliseconds(10);
    }

    if(ecdsa_completed_status!= OPTIGA_LIB_SUCCESS)
    {
        return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

#if (PRINT_SIGNATURE==1)
    for(int x=0; x<sizeof(der_signature);)
    {
        printf(("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \r\n",
                der_signature[x],der_signature[x+1],
                der_signature[x+2],der_signature[x+3],
                der_signature[x+4],der_signature[x+5],
                der_signature[x+6],der_signature[x+7]));
        x+=8;
    }
#endif

    return_status = mbedtls_asn1_get_mpi(&p, end, r);
    if (0 != return_status)
    {
        goto cleanup;
    }

    return_status = mbedtls_asn1_get_mpi(&p, end, s);
    if (0 != return_status)
    {
        goto cleanup;
    }

    return_status = 0;
cleanup:
    // destroy crypt instances
    if (me != NULL)
    {
        (void)optiga_crypt_destroy(me);
    }

    return return_status;

}
#endif

#if defined(MBEDTLS_ECDSA_VERIFY_ALT)
int mbedtls_ecdsa_verify( mbedtls_ecp_group *grp,
                  const unsigned char *buf, size_t blen,
                  const mbedtls_ecp_point *Q, const mbedtls_mpi *r, const mbedtls_mpi *s)
{

    int return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    optiga_lib_status_t crypt_sync_status = OPTIGA_CRYPT_ERROR;
    public_key_from_host_t public_key;
    uint8_t public_key_out [150];
    uint8_t publickey_offset = 3;
    uint8_t signature [150];
    uint8_t * p = NULL;
    size_t  signature_len = 0;
    size_t public_key_len = 0;
    uint8_t truncated_hash_length;
    
    optiga_crypt_t * me = NULL;

    p = signature + sizeof(signature);
    memset(signature, 0x00, sizeof(signature));

    me = optiga_crypt_create(0, optiga_crypt_event_completed, NULL);
    if (NULL == me)
    {
        return_status = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

    if ((grp->id <  MBEDTLS_ECP_DP_SECP256R1) ||
         (grp->id > MBEDTLS_ECP_DP_BP512R1))
    {
        return_status = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

    public_key.key_type = OPTIGA_ECC_CURVE_NIST_P_256;
    truncated_hash_length = 32; //maximum bytes of hash length for the curve
    if(grp->id == MBEDTLS_ECP_DP_SECP384R1)
    {
        public_key.key_type = OPTIGA_ECC_CURVE_NIST_P_384;
        truncated_hash_length = 48;
    }
    else if(grp->id == MBEDTLS_ECP_DP_SECP521R1)
    {
        public_key.key_type = OPTIGA_ECC_CURVE_NIST_P_521;
        truncated_hash_length = 64;
        publickey_offset = 4;
    }
    else if(grp->id == MBEDTLS_ECP_DP_BP256R1)
    {
        public_key.key_type = OPTIGA_ECC_CURVE_BRAIN_POOL_P_256R1;
        truncated_hash_length = 32;
    }
    else if(grp->id == MBEDTLS_ECP_DP_BP384R1)
    {
        public_key.key_type = OPTIGA_ECC_CURVE_BRAIN_POOL_P_384R1;
        truncated_hash_length = 48;
    }
    else if(grp->id == MBEDTLS_ECP_DP_BP512R1)
    {
        public_key.key_type = OPTIGA_ECC_CURVE_BRAIN_POOL_P_512R1;
        truncated_hash_length = 64;
        publickey_offset = 4;
    }
    
    signature_len = mbedtls_asn1_write_mpi( &p, signature, s );
    signature_len += mbedtls_asn1_write_mpi( &p, signature, r );

#if (PRINT_SIGNATURE==1)
    for(int x=0; x<sizeof(signature);)
    {
        printf(("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \n\r",
                signature[x],signature[x+1],signature[x+2],signature[x+3],
                signature[x+4],signature[x+5],signature[x+6],signature[x+7]));
        x+=8;
    }
#endif

    public_key.public_key = public_key_out;

    if (mbedtls_ecp_point_write_binary( grp, Q,
                                        MBEDTLS_ECP_PF_UNCOMPRESSED, &public_key_len,
                                        &public_key_out[publickey_offset], sizeof( public_key_out ) ) != 0 )
    {
        return_status = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

    public_key_out [0] = 0x03;
    public_key_out [publickey_offset - 2] = public_key_len + 1;
    public_key_out [publickey_offset - 1] = 0x00;
    
    if(4 == publickey_offset)
    {
        public_key_out [1] = 0x81;
    }
    public_key.length = public_key_len + publickey_offset;//including 3 bytes overhead

    // If the length of the digest is larger than
    // key length of the group order, then truncate the digest to key length.
    if (blen > truncated_hash_length)
    {
        blen = truncated_hash_length;
    }

#if (PRINT_PUBLICKEY==1)
#define pubk(a) public_key.public_key[a]
    for(int i=0; i<public_key.length;)
    {
        printf(("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X \r\n",
                pubk(i), pubk(i+1), pubk(i+2), pubk(i+3),
                pubk(i+4), pubk(i+5), pubk(i+6), pubk(i+7) ));
        i+=8;
        if(public_key.length-i<8)
        {
            int x=public_key.length-i;
            while(x)
            {
                printf(("%.2X ", pubk(i)));
                i++;
                x--;
            }
            printf(("\r\n"));
        }
    }
#undef pubk(a)
#endif

    ecdsa_completed_status = OPTIGA_LIB_BUSY;
    crypt_sync_status = optiga_crypt_ecdsa_verify ( me, (uint8_t *) buf, blen,
                                                     (uint8_t *) p, signature_len,
                                                      OPTIGA_CRYPT_HOST_DATA, (void *)&public_key );

    if (OPTIGA_LIB_SUCCESS != crypt_sync_status)
    {
        return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    //Wait until the optiga_crypt_ecdsa_verify is completed
    while (OPTIGA_LIB_BUSY == ecdsa_completed_status)
    {
        pal_os_timer_delay_in_milliseconds(10);
    }

    if ( ecdsa_completed_status != OPTIGA_LIB_SUCCESS )
    {
        return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }
    return_status = 0;
cleanup:
    // destroy crypt instances
    if (me != NULL)
    {
        (void)optiga_crypt_destroy(me);
    }
    return return_status;

}
#endif

#if defined(MBEDTLS_ECDSA_GENKEY_ALT)

int mbedtls_ecdsa_genkey( mbedtls_ecdsa_context *ctx, mbedtls_ecp_group_id gid,
                  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
    optiga_lib_status_t crypt_sync_status = OPTIGA_CRYPT_ERROR;
    uint8_t public_key [150];
    size_t public_key_len = sizeof( public_key );
    optiga_ecc_curve_t curve_id;
    mbedtls_ecp_group *grp = &ctx->grp;
    uint16_t privkey_oid = OPTIGA_KEY_ID_E0F0;
    optiga_crypt_t * me = NULL;

    me = optiga_crypt_create(0, optiga_crypt_event_completed, NULL);
    if (NULL == me)
    {
        return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }
 
    mbedtls_ecp_group_load( &ctx->grp, gid );
 
    //checking group against the supported curves of OPTIGA Trust M
    if ((grp->id <  MBEDTLS_ECP_DP_SECP256R1) ||
         (grp->id > MBEDTLS_ECP_DP_BP512R1))
    {
        return_status = MBEDTLS_ERR_ECP_FEATURE_UNAVAILABLE;
        goto cleanup;
    }

    curve_id = OPTIGA_ECC_CURVE_NIST_P_256;
    if(grp->id == MBEDTLS_ECP_DP_SECP384R1)
    {
        curve_id = OPTIGA_ECC_CURVE_NIST_P_384;
    }
    else if(grp->id == MBEDTLS_ECP_DP_SECP521R1)
    {
        curve_id = OPTIGA_ECC_CURVE_NIST_P_521;
    }
    else if(grp->id == MBEDTLS_ECP_DP_BP256R1)
    {
        curve_id = OPTIGA_ECC_CURVE_BRAIN_POOL_P_256R1;
    }
    else if(grp->id == MBEDTLS_ECP_DP_BP384R1)
    {
        curve_id = OPTIGA_ECC_CURVE_BRAIN_POOL_P_384R1;
    }
    else if(grp->id == MBEDTLS_ECP_DP_BP512R1)
    {
        curve_id = OPTIGA_ECC_CURVE_BRAIN_POOL_P_512R1;
    }
    //invoke optiga command to generate a key pair.
    ecdsa_completed_status = OPTIGA_LIB_BUSY;
    crypt_sync_status = optiga_crypt_ecc_generate_keypair( me, curve_id,
                                                (optiga_key_usage_t)( OPTIGA_KEY_USAGE_KEY_AGREEMENT | OPTIGA_KEY_USAGE_AUTHENTICATION ),
                                                FALSE,
                                                &privkey_oid,
                                                public_key,
                                                (uint16_t *)&public_key_len ) ;
    if (OPTIGA_LIB_SUCCESS != crypt_sync_status)
    {
        return_status = MBEDTLS_ERR_ECP_BAD_INPUT_DATA;
        goto cleanup;
    }

    while (OPTIGA_LIB_BUSY == ecdsa_completed_status)
    {
        pal_os_timer_delay_in_milliseconds(5);
    }

    //store public key generated from optiga into mbedtls structure .
    if (mbedtls_ecp_point_read_binary( grp, &ctx->Q,(unsigned char *)&public_key[3],(size_t )public_key_len-3 ) != 0)
    {
        return_status = OPTIGA_CRYPT_ERROR;
        goto cleanup;
    }
    return_status = 0;
cleanup:
    // destroy crypt instances
    if (me != NULL)
    {
        (void)optiga_crypt_destroy(me);
    }

    return return_status;
}                      
#endif

#endif


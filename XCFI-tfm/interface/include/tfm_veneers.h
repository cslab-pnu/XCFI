/*
 * Copyright (c) 2017-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_VENEERS_H__
#define __TFM_VENEERS_H__

#include <stdint.h>
#include "psa/client.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************* Secure function declarations ***************************/

/**
 * \brief Retrieve the version of the PSA Framework API that is implemented.
 *
 * \return The version of the PSA Framework.
 */
uint32_t tfm_psa_framework_version_veneer(void);

/**
 * \brief Return version of secure function provided by secure binary.
 *
 * \param[in] sid               ID of secure service.
 *
 * \return Version number of secure function.
 */
uint32_t tfm_psa_version_veneer(uint32_t sid);

/**
 * \brief Connect to secure function.
 *
 * \param[in] sid               ID of secure service.
 * \param[in] version           Version of SF requested by client.
 *
 * \return Returns handle to connection.
 */
psa_handle_t tfm_psa_connect_veneer(uint32_t sid, uint32_t version);

/**
 * \brief Call a secure function referenced by a connection handle.
 *
 * \param[in] handle            Handle to connection.
 * \param[in] ctrl_param        Parameters combined in uint32_t,
 *                              includes request type, in_num and out_num.
 * \param[in] in_vec            Array of input \ref psa_invec structures.
 * \param[in,out] out_vec       Array of output \ref psa_outvec structures.
 *
 * \return Returns \ref psa_status_t status code.
 */
psa_status_t tfm_psa_call_veneer(psa_handle_t handle,
                                 uint32_t ctrl_param,
                                 const psa_invec *in_vec,
                                 psa_outvec *out_vec);

/**
 * \brief Close connection to secure function referenced by a connection handle.
 *
 * \param[in] handle            Handle to connection
 */
void tfm_psa_close_veneer(psa_handle_t handle);

/**************** ret2ns evaluation secure function declarations **************/

/**
 * \brief Non-secure callback prototype used by the ret2ns BLXNS test.
 */
typedef int ret2ns_ns_callback_t(void);

/**
 * \brief Vulnerable direct NSC functions used by the ret2ns evaluation sample.
 */
int32_t ret2ns_register_callback_nsc(ret2ns_ns_callback_t *callback);
int32_t print_LCD_nsc(char *msg);
int32_t blxns_nsc(char *msg);

/************ End ret2ns evaluation secure function declarations **************/

/***************** End Secure function declarations ***************************/

#ifdef __cplusplus
}
#endif

#endif /* __TFM_VENEERS_H__ */

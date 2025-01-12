/* Copyright (c) 2010 - 2017, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef simple_message_CLIENT_H__
#define simple_message_CLIENT_H__

#include <stdint.h>
#include "access.h"
#include "simple_message_common.h"

/**
 * @defgroup simple_message_CLIENT Simple Message Client
 * @ingroup simple_message_MODEL
 * This module implements a simple proprietary Simple Message Client.
 *
 * @{
 */


/** Simple Message Client model ID. */
#define simple_message_CLIENT_MODEL_ID (0x01)

/** Simple Message status codes. */
typedef enum
{
    /** Received status ON from the server. */
    simple_message_STATUS_ON,
    /** Received status OFF from the server. */
    simple_message_STATUS_OFF,
    /** The server did not reply to a Simple Message Set/Get. */
    simple_message_STATUS_ERROR_NO_REPLY
} simple_message_status_t;

/** Forward declaration. */
typedef struct __simple_message_client simple_message_client_t;

/**
 * Simple Message status callback type.
 *
 * @param[in] p_self Pointer to the Simple Message client structure that received the status.
 * @param[in] status The received status of the remote server.
 * @param[in] src    Element address of the remote server.
 */
typedef void (*simple_message_status_cb_t)(const simple_message_client_t * p_self, 
                                            simple_message_status_t status, 
                                            uint16_t src);

typedef bool (*simple_message_set_cb_t)(const simple_message_client_t * p_self,
                                              nrf_mesh_address_t src,
                                              nrf_mesh_address_t dst,  
                                              uint8_t *data, 
                                              uint8_t length);

typedef struct
{
    /** Client model response message callback. */
    simple_message_status_cb_t simple_message_status_cb;

} simple_message_client_callbacks_t;


/** Simple Message Client state structure. */
struct __simple_message_client
{
    /** Model handle assigned to the client. */
    access_model_handle_t model_handle;
    /** Status callback called after status received from server. */
    simple_message_status_cb_t status_cb;
    simple_message_set_cb_t set_cb;
    /** Internal client state. */
    struct
    {
        bool reliable_transfer_active; /**< Variable used to determine if a transfer is currently active. */
        simple_message_msg data;  /**< Variable reflecting the data stored in the server. */
    } state;
};

/**
 * Initializes the Simple Message client.
 *
 * @note This function should only be called _once_.
 * @note The client handles the model allocation and adding.
 *
 * @param[in] p_client      Simple Message Client structure pointer.
 * @param[in] element_index Element index to add the server model.
 *
 * @retval NRF_SUCCESS         Successfully added client.
 * @retval NRF_ERROR_NULL      NULL pointer supplied to function.
 * @retval NRF_ERROR_NO_MEM    No more memory available to allocate model.
 * @retval NRF_ERROR_FORBIDDEN Multiple model instances per element is not allowed.
 * @retval NRF_ERROR_NOT_FOUND Invalid element index.
 */
uint32_t simple_message_client_init(simple_message_client_t * p_client, uint16_t element_index);

/**
 * Sets the state of the Simple Message server.
 *
 * @param[in] p_client Simple Message Client structure pointer.
 * @param[in] on_off   Value to set the Simple Message Server state to.
 *
 * @retval NRF_SUCCESS              Successfully sent message.
 * @retval NRF_ERROR_NULL           NULL pointer in function arguments
 * @retval NRF_ERROR_NO_MEM         Not enough memory available for message.
 * @retval NRF_ERROR_NOT_FOUND      Invalid model handle or model not bound to element.
 * @retval NRF_ERROR_INVALID_ADDR   The element index is greater than the number of local unicast
 *                                  addresses stored by the @ref DEVICE_STATE_MANAGER.
 * @retval NRF_ERROR_INVALID_STATE  Message already scheduled for a reliable transfer.
 * @retval NRF_ERROR_INVALID_PARAM  Model not bound to appkey, publish address not set or wrong
 *                                  opcode format.
 */
uint32_t simple_message_client_set(simple_message_client_t * p_client, bool on_off);

/**
 * Sets the state of the Simple Message Server unreliably (without acknowledgment).
 *
 * @param[in] p_client Simple Message Client structure pointer.
 * @param[in] on_off   Value to set the Simple Message Server state to.
 * @param[in] repeats  Number of messages to send in a single burst. Increasing the number may
 *                     increase probability of successful delivery.
 *
 * @retval NRF_SUCCESS              Successfully sent message.
 * @retval NRF_ERROR_NULL           NULL pointer in function arguments
 * @retval NRF_ERROR_NO_MEM         Not enough memory available for message.
 * @retval NRF_ERROR_NOT_FOUND      Invalid model handle or model not bound to element.
 * @retval NRF_ERROR_INVALID_ADDR   The element index is greater than the number of local unicast
 *                                  addresses stored by the @ref DEVICE_STATE_MANAGER.
 * @retval NRF_ERROR_INVALID_PARAM  Model not bound to appkey, publish address not set or wrong
 *                                  opcode format.
 */
uint32_t simple_message_client_set_unreliable(simple_message_client_t * p_client,  uint8_t *data, uint8_t length);

/**
 * Gets the state of the Simple Message server.
 *
 * @note The state of the server will be given in the @ref simple_message_status_cb_t callback.
 *
 * @param[in] p_client Simple Message Client structure pointer.
 *
 * @retval NRF_SUCCESS              Successfully sent message.
 * @retval NRF_ERROR_NULL           NULL pointer in function arguments
 * @retval NRF_ERROR_NO_MEM         Not enough memory available for message.
 * @retval NRF_ERROR_NOT_FOUND      Invalid model handle or model not bound to element.
 * @retval NRF_ERROR_INVALID_ADDR   The element index is greater than the number of local unicast
 *                                  addresses stored by the @ref DEVICE_STATE_MANAGER.
 * @retval NRF_ERROR_INVALID_STATE  Message already scheduled for a reliable transfer.
 * @retval NRF_ERROR_INVALID_PARAM  Model not bound to appkey, publish address not set or wrong
 *                                  opcode format.
 */
uint32_t simple_message_client_get(simple_message_client_t * p_client);



/** @} end of simple_message_CLIENT */

#endif /* simple_message_CLIENT_H__ */

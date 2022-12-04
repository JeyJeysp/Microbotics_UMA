/*
 * tivarpc.h
 *
 *  Fichero de cabecera. Implementa la funcionalidad RPC entre la TIVA y el interfaz de usuario
 */

#ifndef TIVARPC_H__
#define TIVARPC_H__


#include <remotelink_messages.h>
#include<stdbool.h>
#include<stdint.h>

#include "serialprotocol.h"

BaseType_t remotelink_init(int32_t remotelink_stack, int32_t remotelink_priority, int32_t (* message_receive_callback)(uint8_t message_type, void *parameters, int32_t parameterSize));
int32_t remotelink_sendMessage(uint8_t comando,void *parameter,int32_t paramsize);

#endif /*TIVARPC_H__ */

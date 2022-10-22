/*
 * remotelink_messages.h
 *
 *  Created on: March. 2019
 *
 */


#ifndef RL_MESSAGES_H
#define RL_MESSAGES_H
//Codigos de los mensajes y definicion de parametros para el protocolo RPC

// El estudiante debe agregar aqui cada nuevo mensaje que implemente. IMPORTANTE el orden de los mensajes
// debe SER EL MISMO aqui, y en el codigo equivalente en la parte del microcontrolador (Code Composer)

typedef enum {
    MESSAGE_REJECTED,
    MESSAGE_PING,
    MESSAGE_MOTOR
    //etc, etc...
} messageTypes;

//Estructuras relacionadas con los parametros de los mensahes. El estuadiante debera crear las
// estructuras adecuadas a los comandos usados, y asegurarse de su compatibilidad en ambos extremos

#pragma pack(1) //Cambia el alineamiento de datos en memoria a 1 byte.

typedef struct {
    uint8_t message;
} MESSAGE_REJECTED_PARAMETER;

#define MOTORES 0
#define MOTOR_1 1
#define MOTOR_2 2
typedef struct {
    uint8_t id_motor;
    int8_t porcentaje_vel;
}PARAM_MESSAGE_MOTOR;


#pragma pack()  //...Pero solo para los comandos que voy a intercambiar, no para el resto.

#endif // RPCCOMMANDS_H



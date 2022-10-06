#ifndef RL_MESSAGES_H
#define RL_MESSAGES_H

typedef enum {
    MESSAGE_REJECTED,
    MESSAGE_PING,
    MESSAGE_MOTOR,
} messageTypes;

#pragma pack(1) //Cambia el alineamiento de datos en memoria a 1 byte.

typedef struct {
    uint8_t command;
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


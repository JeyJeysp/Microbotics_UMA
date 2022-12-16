#ifndef COLAEVENTOS_H_
#define COLAEVENTOS_H_


#include "event_groups.h"

// Manejador para el flag de eventos
extern EventGroupHandle_t FlagsEventos;

// Etiquetas de eventos
#define MOV 0x0002
#define PASOS_RUEDA_IZQ 0x0004
#define PASOS_RUEDA_DER 0x0008

// Flags de eventos para los whiksers
#define VAL_WHISK_FRONT 0x0010
#define VAL_WHISK_BACK 0x0020

// Flags de eventos para los sensores de linea
#define TATAMI_DD 0x0040
#define TATAMI_DI 0x0080
#define TATAMI_TD 0x0100
#define TATAMI_TI 0x0200

// Flag de evento para la inicializacion del modo busqueda del robot
#define FLAG_BOTON_DERECHO 0x0400


//Cola para los movimientos
extern QueueHandle_t colaMovimiento;

// Estructura para mandar movimiento: t = tipo (cm o grados. Aceptara 'c' o 'g') / v = valor.
struct pareja
{
    char t;
    int32_t v;
};


#endif

#ifndef COLAEVENTOS_H_
#define COLAEVENTOS_H_


#include "event_groups.h"

// Manejador para el flag de eventos
extern EventGroupHandle_t FlagsEventos;
extern QueueHandle_t colaCM, colaGrados;

// Etiquetas de eventos
#define RECTO 0x0001
#define GIRAR 0x0002
#define PASOS_RUEDA_IZQ 0x0004
#define PASOS_RUEDA_DER 0x0020

// Flag de evento para la inicializacion del modo busqueda del robot
#define FLAG_BOTON_DERECHO 0x0008

// Flags de eventos para los whiksers
#define VAL_WHISK_FRONT 0x0010
#define VAL_WHISK_BACK 0x0400

// Flags de eventos para los sensores de linea
#define TATAMI_DD 0x0040
#define TATAMI_DI 0x0080
#define TATAMI_TD 0x0100
#define TATAMI_TI 0x0200

#endif

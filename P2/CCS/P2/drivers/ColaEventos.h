#ifndef COLAEVENTOS_H_
#define COLAEVENTOS_H_


#include "event_groups.h"

// Manejador para el flag de eventos
extern EventGroupHandle_t FlagsEventos;
extern QueueHandle_t colaCM, colaGrados;

// Etiquetas de eventos
#define BOTONES 0x0001
#define RECTO 0x0002
#define GIRAR 0x0004
#define PARAR 0x0008
#define PASOS_RUEDA 0X0010

#endif

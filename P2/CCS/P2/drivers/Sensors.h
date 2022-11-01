/*
    Libreria para la implementacion de la funcionalidad de los Sensores
    Daniel Bazo Correa & Juan Jose Navarrete Galvez
*/
#include <stdbool.h>
#include <stdint.h>

// Pines Whiskers
#define PIN_WHISK_FRONT GPIO_PIN_5
#define PIN_WHISK_BACK GPIO_PIN_6
#define VAL_WHISK_FRONT 1
#define VAL_WHISK_BACK 2

// Pin sensor Sharp delantero de media distancia
#define PIN_SHARP_FRONT GPIO_PIN_3

// Pines CNY
#define PIN_CNY_IZQ GPIO_PIN_2
#define PIN_CNY_DER GPIO_PIN_3


void config_sensors(void);
unsigned short binary_lookup(unsigned short *A, unsigned short key, unsigned short imin, unsigned short imax);

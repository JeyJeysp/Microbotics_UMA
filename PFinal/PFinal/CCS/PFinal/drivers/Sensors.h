/*
    Libreria para la implementacion de la funcionalidad de los Sensores
    Daniel Bazo Correa & Juan Jose Navarrete Galvez
*/
#include <stdbool.h>
#include <stdint.h>

// Pines Whiskers
#define PIN_WHISK_FRONT GPIO_PIN_5
#define PIN_WHISK_BACK GPIO_PIN_6

// Pin sensor Sharp delantero de media distancia
#define PIN_SHARP_FRONT GPIO_PIN_3
#define VAL_SHARP_CERCANO 1
#define VAL_SHARP_MEDIO 2
#define VAL_SHARP_LEJOS 3
#define VAL_SHARP_NO_VISIBLE 4


// Pines CNY
#define PIN_CNY_IZQ GPIO_PIN_2
#define PIN_CNY_DER GPIO_PIN_3

// Pines sensor de línea
#define PIN_LIN_DD GPIO_PIN_4
#define PIN_LIN_DI GPIO_PIN_5
#define PIN_LIN_TD GPIO_PIN_6
#define PIN_LIN_TI GPIO_PIN_7

void config_sensors(void);
unsigned short binary_lookup(unsigned short *A, unsigned short key, unsigned short imin, unsigned short imax);

/*
    Libreria para la implementacion de la funcionalidad de los Sensores
    Daniel Bazo Correa & Juan Jose Navarrete Galvez
*/
#include <stdbool.h>
#include <stdint.h>

// Whisker delantero
#define PIN_WHISK_FRONT GPIO_PIN_5
#define PIN_WHISK_BACK GPIO_PIN_3

#define PIN_LED_SHARP_1 GPIO_PIN_6
#define PIN_LED_SHARP_2 GPIO_PIN_7

// Sensor Sharp delantero de media distancia
#define PIN_SHARP_FRONT GPIO_PIN_3

void config_sensors(void);

/*
    Libreria para la implementacion de la funcionalidad de los Sensores
    Daniel Bazo Correa & Juan Jose Navarrete Galvez
*/
#include <stdbool.h>
#include <stdint.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"

#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "drivers/buttons.h"
#include "driverlib/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "drivers/Sensors.h"
#include "drivers/ColaEventos.h"

extern EventGroupHandle_t FlagsEventos;

extern unsigned short val_distancia[];
extern unsigned short val_voltaje_hex[];

void config_sensors(void)
{
    // Configuracion PA5 y PA6 para el whisker frontal y trasero (respectivamente)
    // Configuracion PA2 y PA3 para los CNY izquierdo y derecho (respectivamente)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, PIN_WHISK_FRONT | PIN_WHISK_BACK | PIN_CNY_IZQ | PIN_CNY_DER);

    GPIOIntTypeSet(GPIO_PORTA_BASE, PIN_WHISK_FRONT | PIN_WHISK_BACK, GPIO_FALLING_EDGE);
    GPIOIntTypeSet(GPIO_PORTA_BASE, PIN_CNY_IZQ | PIN_CNY_DER, GPIO_FALLING_EDGE);

    // Usamos la siguiente instruccion para anadir una resistencia de pull-up
    GPIOPadConfigSet(GPIO_PORTA_BASE, PIN_WHISK_FRONT | PIN_WHISK_BACK, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    IntPrioritySet(INT_GPIOA, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    GPIOIntEnable(GPIO_PORTA_BASE, PIN_WHISK_FRONT | PIN_WHISK_BACK | PIN_CNY_IZQ | PIN_CNY_DER);
    IntEnable(INT_GPIOA);


    //Configuracion PC4, PC5, PC6 y PC7 para los sensores de linea
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOC);

    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, PIN_LIN_DD | PIN_LIN_DI | PIN_LIN_TD | PIN_LIN_TI);
    GPIOIntTypeSet(GPIO_PORTC_BASE, PIN_LIN_DD | PIN_LIN_DI | PIN_LIN_TD | PIN_LIN_TI, GPIO_RISING_EDGE);
    //GPIOPadConfigSet(GPIO_PORTC_BASE, PIN_LIN_DD | PIN_LIN_DI | PIN_LIN_TD | PIN_LIN_TI, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD); // Mejor funcionamiento con bateria 2A

    IntPrioritySet(INT_GPIOC, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    GPIOIntEnable(GPIO_PORTC_BASE, PIN_LIN_DD | PIN_LIN_DI | PIN_LIN_TD | PIN_LIN_TI);
    IntEnable(INT_GPIOC);


    //Configuracion PB0, PB1 y PB2 para LEDs verde, azul y rojo respectivamente
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, PIN_LED_G | PIN_LED_B | PIN_LED_R | PIN_LED_Y);
    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_G | PIN_LED_B | PIN_LED_R | PIN_LED_Y, 0);
}

unsigned short binary_lookup(unsigned short *A, unsigned short key, unsigned short imin, unsigned short imax)
{
    unsigned int imid;

    while (imin < imax)
    {
        imid = (imin + imax) >> 1;

        if (A[imid] < key)
        {
            imin = imid + 1;
        }
        else
        {
            imax = imid;
        }
    }

    //Al final imax=imin y en dicha posicion hay un numero mayor o igual que el buscado
    return imax;
}

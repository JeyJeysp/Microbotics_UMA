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
    /*
         - Configuracion PA5 y PA6 para el whisker frontal y trasero (respectivamente)
         - Configuracion PA2 y PA3 para los CNY izquierdo y derecho (respectivamente)
    */
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

    /*
         Configuracion PA 6 & 7 para LEDs del Sharp
     */
    //GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, PIN_LED_SHARP_1 | PIN_LED_SHARP_2);
    //GPIOPinWrite(GPIO_PORTA_BASE, PIN_LED_SHARP_1 | PIN_LED_SHARP_2, 0);
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

// EJERCICIO 2: Funcion que nos dice cuando una rueda ha dado una vuelta completa.
void vueltas_CNY(uint32_t motor)
{
    // Tenemos 6 huecos blancos, que es lo que detecta el CNY
    // 2 -> derecho
    if(motor == 1)
    {
        num_pasos_izq += 1;
    }
    else if(motor == 2)
    {
        num_pasos_der += 1;
    }

    if((num_pasos_izq || num_pasos_der) == 6)
    {
        //UARTprintf("CNY: Se ha dado una vuelta completa.");
    }
}

void resetear_cuenta_CNY()
{
    num_pasos_izq = 0;
    num_pasos_der = 0;
}

#include <stdint.h>
#include <stdbool.h>

//  Librerias que se incluyen tipicamente para configuracion de perifericos y pinout
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"

#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "utils/uartstdio.h"
#include "drivers/buttons.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "utils/cpu_usage.h"


//  DBC: Globales
QueueHandle_t cola_freertos;
uint32_t g_ui32CPUUsage;
//  Prioridad para la tarea maestra
#define MASTERTASKPRIO 1
//  Tamaño de pila para la tarea
#define MASTERTASKSIZE 512

//  TODO: Ciclos de reloj para conseguir una señal periódica de 50Hz (según reloj de periférico usado)
/*
    DBC:

    tiempo transcurrido = Numero de ciclos contados * Periodo = Numero de ciclos contados * (1 / frecuencia)
    t = N * T = N * (1 / f)
    Periodo de la onda PWM que hemos definido (el valor maximo posible es 65535)
    Como el reloj del micro funcionará a 40 MHz, tenemos que (1/50) / (1/(40*10e6)) = 800000, por lo tanto hay que dividirlo (mirar el main)
    Lo dividimos entre 16 (ver main) y  lo multiplicamos por 0.02 para obtener los 50 hz
*/
#define PERIOD_PWM ((SysCtlClockGet() / 16) * 0.02)
#define COUNT_1MS ((SysCtlClockGet() / 16) * 0.001) //   TODO: Ciclos para amplitud de pulso de 1ms (max velocidad en un sentido)
#define STOPCOUNT ((SysCtlClockGet() / 16) * 0.00152) //   TODO: Ciclos para amplitud de pulso de parada (1.52ms)
#define COUNT_2MS ((SysCtlClockGet() / 16) * 0.002) //   TODO: Ciclos para amplitud de pulso de 2ms (max velocidad en el otro sentido)
#define NUM_STEPS 50 // Pasos para cambiar entre el pulso de 2ms al de 1ms
#define CYCLE_ INCREMENTS (abs(COUNT_1MS-COUNT_2MS)) / NUM_STEPS // Variacion de amplitud tras pulsacion

#ifdef DEBUG
void __error__(char *nombrefich, uint32_t linea)
{
    // Si la ejecucion esta aqui dentro, es que el RTOS o alguna de las bibliotecas de perifericos han
    // comprobado que hay un error
    // Mira el arbol de llamadas en el depurador y los valores de nombrefich y linea para encontrar posibles pistas.
    while(1)
    {
    }
}
#endif

//*****************************************************************************
//
//  Aqui incluimos los "ganchos" a los diferentes eventos del FreeRTOS
//
//*****************************************************************************

//  Esto es lo que se ejecuta cuando el sistema detecta un desbordamiento de pila
void vApplicationStackOverflowHook(TaskHandle_t pxTask,  char *pcTaskName)
{
    while(1)
    {
    }
}

//  Esto se ejecuta cada Tick del sistema. LLeva la estadistica de uso de la CPU (tiempo que la CPU ha estado funcionando)
void vApplicationTickHook( void )
{
    static uint8_t count = 0;

    if (++count == 10)
    {
        g_ui32CPUUsage = CPUUsageTick();
        count = 0;
    }
}

//  Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationIdleHook (void)
{
    SysCtlSleep();
}


//  Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationMallocFailedHook (void)
{
    while(1);
}

//  TODO: Rutinas de interrupción de pulsadores
//  TODO: Boton Izquierdo: modifica ciclo de trabajo en CYCLE_INCREMENTS para el servo conectado a PF2, hasta llegar a COUNT_1MS
//  TODO: Boton Derecho: modifica ciclo de trabajo en CYCLE_INCREMENTS para el servo conectado a PF2, hasta llegar a COUNT_2MS
static portTASK_FUNCTION(TareaCambioPWM, pvParameters)
{
    uint32_t ui32Status;

    while(1)
    {
        if (xQueueReceive(cola_freertos, &ui32Status, portMAX_DELAY) == pdTRUE)
        {
            if ((ui32Status & LEFT_BUTTON))
            {
                PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, COUNT_1MS);
                PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, COUNT_1MS);
            }
            else if ((ui32Status & RIGHT_BUTTON))
            {
                PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, COUNT_2MS);
                PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, COUNT_2MS);
            }
        }
    }
}

int main(void)
{
    uint32_t ui32Period, ui32DutyCycle;

    cola_freertos = xQueueCreate(16, sizeof(uint32_t));
    if(cola_freertos == NULL)
    {
        while(1);
    }

    //  TODO: Elegir reloj adecuado para los valores de ciclos sean de tamaño soportable
    //  Reloj del sistema a 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5|SYSCTL_USE_PLL|SYSCTL_XTAL_16MHZ|SYSCTL_OSC_MAIN);
    /*
        DBC:

        Dividir el periodo del PWM para ajustarse al máximo, tenemos que al dividirlo entre 16
        es el mínimo valor que deja un valor exacto en el periodo y que se encuentra por debajo del límite
    */

    SysCtlPWMClockSet(SYSCTL_PWMDIV_16);

    //  TODO: Configura pulsadores placa TIVA (int. por flanco de bajada)
    //  DBC: Inicializa los botones y habilita sus interrupciones
    ButtonsInit();
    GPIOIntTypeSet(GPIO_PORTF_BASE, ALL_BUTTONS, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, ALL_BUTTONS);
    IntEnable(INT_GPIOF);
    //  DBC: Cambiamos la configuración a flanco de bajada
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_FALLING_EDGE);

    /*
        TODO:

        Configuracion ondas PWM: frecuencia 50Hz, anchura inicial= valor STOPCOUNT, 1540us
        para salida por PF2, y COUNT_1MS (o COUNT_2MS ) para salida por PF3(puedes ponerlo
        inicialmente a PERIOD_PWM/10)

        DBC:

        Tenemos que PF2 emplea el módulo PWM M1, con PWM6 Y PF3 M1 con PWM7
    */
    //  DBC: Habilitamos el módulo M1 del PWM
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    //  DBC: Habilitamos el bajo consumo del periférico
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_PWM1);
    //  DBC: Configuramos los pines como salida PWM
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2|GPIO_PIN_3);
    GPIOPinConfigure(GPIO_PF2_M1PWM6);
    GPIOPinConfigure(GPIO_PF3_M1PWM7);

    //  DBC: PWM_GEN_3 Covers M1PWM6 and M1PWM7 See page 207 4/11/13 DriverLib doc
    PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

    ui32Period = PERIOD_PWM;
    ui32DutyCycle = STOPCOUNT;

    // Carga la cuenta que establece la frecuencia de la señal PWM
    PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ui32Period);
    //Habilita/pone en marcha el generador PWM
    PWMGenEnable(PWM1_BASE, PWM_GEN_3);
    // Habilita la salida de la señal
    PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT|PWM_OUT_7_BIT, true);
    // Establece el periodo (en este caso, un porcentaje del valor máximo)
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6, ui32DutyCycle); // pf2
    PWMPulseWidthSet(PWM1_BASE, PWM_OUT_7, ui32DutyCycle); //pf3

    /*
        TODO:

        Opcion 1: Usar un Timer en modo PWM (ojo! Los timers PWM solo soportan cuentas
        de 16 bits, a menos que uséis un prescaler/timer extension)
        Opcion 2: Usar un módulo PWM(no dado en Sist. Empotrados pero mas sencillo)
        Opcion 1: Usar un Wide Timer (32bits) en modo PWM (estos timers soportan
        cuentas de 32 bits, pero tendréis que sacar las señales de control pwm por
        otros pines distintos de PF2 y PF3)
        Codigo principal, (poner en bucle infinito o bajo consumo)

        Arranca el  scheduler.  Pasamos a ejecutar las tareas que se hayan activado.
        el RTOS habilita las interrupciones al entrar aqui, asi que no hace falta habilitarlas
        De la funcion vTaskStartScheduler no se sale nunca... a partir de aqui pasan a ejecutarse las tareas.
    */
    if((xTaskCreate(TareaCambioPWM, "TaskPWM", MASTERTASKSIZE, NULL, tskIDLE_PRIORITY + MASTERTASKPRIO, NULL) != pdPASS))
    {
        while(1);
    }

    vTaskStartScheduler();

    while(1);
}

//  DBC: Función para el tratamiento de los switches
void GPIOFIntHandler(void)
{
    //  DBC: Hay que inicializarlo a False!!
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    //  DBC: Lee el estado del puerto (activos a nivel bajo)
    //  DBC: pasamos el estado de los pines cuando se produjo la interrupcion
    int32_t i32PinStatus = MAP_GPIOIntStatus(GPIO_PORTF_BASE, ALL_BUTTONS);

    //  DBC: FromISR porque estoy en un rutina de tratamiento de interrupción
    //  DBC: Pasamos un valor por referencia, escribe en la cola freeRTOS
    xQueueSendFromISR(cola_freertos, &i32PinStatus, &higherPriorityTaskWoken);
    MAP_GPIOIntClear(GPIO_PORTF_BASE, ALL_BUTTONS);

    //  DBC: Ahora hay que comprobar si hay que hacer el cambio de contexto
    //  DBC: Se puede hacer con CUALQUIERA de las dos lineas siguientes (las dos hacen lo mismo)
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
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
#include "drivers/rgb.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "utils/cpu_usage.h"
#include "commands.h"
#include "timers.h"
#include "math.h"

#include "drivers/ColaEventos.h"
#include "drivers/Servos.h"
#include "drivers/Sensors.h"
#include "drivers/configADC.h"

#include <remotelink.h>
#include <serialprotocol.h>

// *** Definiciones de prioridades y tamano de pila de las tareas *** //
#define MASTERTASKPRIO 1
#define MASTERTASKSIZE 512
#define MOVTASKPRIO 2
#define MOVTASKSIZE 256
#define ADCTASKPRIO 3
#define ADCTASKSIZE 128
#define PATASKPRIO 3

// Definicion de los modos
#define BUSQUEDA 0
#define ATAQUE 1
#define HUIDA 2



// *** Declaracion de globales *** //
bool ON_INI = false;
int8_t modo = -1, en_movimiento = 0, detectar = 1, show_once[] = {0, 0, 0};
uint8_t avanzar = 0, CASIO_on = 0, pasar_SD = 1, pasar_ST = 1;
uint32_t g_ui32CPUUsage, g_ulSystemClock, contSharp = 0, umbralSharp = 500;
QueueHandle_t colaMovimiento, colaSharp;
EventGroupHandle_t FlagsEventos;
static TimerHandle_t CASIO_BUSQUEDA, CASIO_REBOTON;

int objetivo_cm = 0, objetivo_grados = 0;

#ifdef DEBUG

void __error__(char *nombrefich, uint32_t linea)
{
    while (1);
}

#endif


void vApplicationStackOverflowHook(TaskHandle_t pxTask,  char *pcTaskName)
{
    while (1);
}


//  Esta es la funcion que ejecuta cuando el RTOS se queda sin memoria dinamica
void vApplicationMallocFailedHook (void)
{
    while (1);
}


//  Esto se ejecuta cada Tick del sistema. LLeva la estadistica de uso de la CPU (tiempo que la CPU ha estado funcionando)
void vApplicationTickHook( void )
{
    static uint8_t ui8Count = 0;

    if (++ui8Count == 10)
    {
        g_ui32CPUUsage = CPUUsageTick();
        ui8Count = 0;
    }
}


//  Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationIdleHook (void )
{
    SysCtlSleep();
}


//  Esta tarea esta definida en el fichero command.c, es la que se encarga de procesar los comandos.
//  Aqui solo la declaramos para poderla crear en la funcion main.
extern void vUARTTask(void *pvParameters);



// *** Definicion de nuestras tareas *** //


//*****************************************************************************
//
//! CASIOCallback
//!
//! Esta callback se encargara del control de nuestro timer. Al saltar activara
//! una variable que hara moverse en linea recta a nuestro microbot.
//!
//! Tipo: callback
//
//*****************************************************************************
void CASIOCallback(TimerHandle_t pxTimer)
{
    CASIO_on = 0;
    avanzar = 1;
}


//*****************************************************************************
//
//! CASIOCallback_2
//!
//! Esta callback se encargara del control de nuestro timer. Al saltar activara
//! una variable que hara moverse en linea recta a nuestro microbot.
//!
//! Tipo: callback
//
//*****************************************************************************
void CASIOCallback_2(TimerHandle_t pxTimer)
{
    detectar = 1;
}

//*****************************************************************************
//
//! TareaMaestra
//!
//! Esta tarea se encargara del control principal del programa. En esencia es
//! una maquina de estados con 3 estados principales: busqueda (0), ataque (1)
//! y huida (3).
//!
//! Tipo: hebra
//
//*****************************************************************************
static portTASK_FUNCTION (TareaMaestra, pvParameters)
{
    uint32_t ui32Status;
    EventBits_t eventos;
    struct pareja mov;


    while(1)
    {
        eventos = xEventGroupWaitBits(FlagsEventos, TATAMI_DD | TATAMI_DI | TATAMI_TD | TATAMI_TI | VAL_WHISK_FRONT | VAL_WHISK_BACK, pdTRUE, pdFALSE, 0);

        switch(modo)
        {
            case BUSQUEDA:
            {
                // El robot rota de manera indefinida hasta encontrar un objeto
                // Cuando detecte la interrupcion del sharp o whisker pasar al modo ataque

                if (show_once[0] == 0)
                {
                    show_once[1] = 0;
                    show_once[2] = 0;
                    show_once[0] = 1;
                    //UARTprintf("Entro en modo Busqueda \r\n");

                    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_G | PIN_LED_B | PIN_LED_Y, 0);
                    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_G, PIN_LED_G);
                }

                if (avanzar == 1)
                {
                    mov.t = 'c';
                    mov.v = 9;

                    xQueueSend(colaMovimiento, &mov, portMAX_DELAY);
                    xEventGroupSetBits(FlagsEventos, MOV);

                    en_movimiento = 1;

                    if(xTimerStop(CASIO_BUSQUEDA, 0) != pdPASS)
                    {
                        while(1);
                    }

                    avanzar = 0;
                }
                else if (en_movimiento == 0)
                {
                    mov_rueda_universal(RUEDA_IZQUIERDA, -100);
                    mov_rueda_universal(RUEDA_DERECHA, 90);

                    if (CASIO_on == 0)
                    {
                        if(xTimerStart(CASIO_BUSQUEDA, 0) != pdPASS)
                        {
                            while(1);
                        }

                        CASIO_on = 1;
                    }
                }


                if (((eventos & TATAMI_DD)  != 0) || ((eventos & TATAMI_DI)  != 0) || ((eventos & TATAMI_TD)  != 0) || ((eventos & TATAMI_TI)  != 0))
                {
                    modo = HUIDA;
                    xQueueReset(colaMovimiento);

                    if (CASIO_on == 1)
                    {
                        if(xTimerStop(CASIO_BUSQUEDA, 0) != pdPASS)
                        {
                            while(1);
                        }

                        CASIO_on = 0;
                    }
                }
                else if ((eventos & VAL_WHISK_FRONT)  != 0)
                {
                   modo = ATAQUE;

                   if (CASIO_on == 1)
                   {
                       if(xTimerStop(CASIO_BUSQUEDA, 0) != pdPASS)
                       {
                           while(1);
                       }

                       CASIO_on = 0;
                   }
                }
                else if ((eventos & VAL_WHISK_BACK)  != 0)
                {
                    modo = HUIDA;
                    xQueueReset(colaMovimiento);

                    if (CASIO_on == 1)
                    {
                        if(xTimerStop(CASIO_BUSQUEDA, 0) != pdPASS)
                        {
                            while(1);
                        }

                        CASIO_on = 0;
                    }
                }
                else if ((xQueueReceive(colaSharp, &ui32Status, 0) == pdTRUE) && (ui32Status != VAL_SHARP_NO_VISIBLE))
                {
                    modo = ATAQUE;

                    if (CASIO_on == 1)
                    {
                        if(xTimerStop(CASIO_BUSQUEDA, 0) != pdPASS)
                        {
                            while(1);
                        }

                        CASIO_on = 0;
                    }
                }
            }

            break;


            case ATAQUE:
            {
                // En el modo de ataque:
                // debe de acelerar hacia el objetivo, si detecta pulsaciones del whisker delantero seguira
                // en el modo ataque.

                // Si durante el modo ataque:
                // los datos del sharp son de distancias altas o medias o no detecta el whisker delantero
                // volveremos al modo de busqueda.
                // si durante el ataque al enemigo, los sensores de linea detecta el limite del mapa
                // pasamos al modo de huida

                if (show_once[1] == 0)
                {
                    show_once[0] = 0;
                    show_once[2] = 0;
                    show_once[1] = 1;
                    //UARTprintf("Entro en modo Ataque \r\n");

                    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_G|PIN_LED_B|PIN_LED_Y, 0);
                    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_B, PIN_LED_B);
                }

                mov_rueda_universal(RUEDA_IZQUIERDA, 100);
                mov_rueda_universal(RUEDA_DERECHA, 90);

                if (((eventos & TATAMI_DD)  != 0) || ((eventos & TATAMI_DI)  != 0) ||
                     ((eventos & TATAMI_TD)  != 0) || ((eventos & TATAMI_TI)  != 0) || (eventos & VAL_WHISK_BACK)  != 0)
                {
                    modo = HUIDA;
                    xQueueReset(colaMovimiento);
                }
                else if ((xQueueReceive(colaSharp, &ui32Status, 0) == pdTRUE) && (ui32Status == VAL_SHARP_NO_VISIBLE))
                {
                    contSharp++;

                    if (contSharp == umbralSharp)
                    {
                        contSharp = 0;
                        modo = BUSQUEDA;
                    }
                }
            }

            break;


            case HUIDA:
            {
                // En el modo huida:
                // Cuando entremos en este modo los motores aceleran hacia atras durante 2 segundos
                // y posteriormente giran 180 grados. Si durante ese giro, encuentra a alguien, vuelve al modo ataque.
                // Si los datos que recibe del sharp son de distancia media o larga o no recibe datos del whisker delantero
                // pasara al modo de busqueda

                if (show_once[2] == 0)
                {
                    show_once[0] = 0;
                    show_once[1] = 0;
                    show_once[2] = 1;
                    //UARTprintf("Entro en modo Huida \r\n");

                    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_G | PIN_LED_B | PIN_LED_Y, 0);
                    GPIOPinWrite(GPIO_PORTB_BASE, PIN_LED_Y, PIN_LED_Y);
                }

                // xQueueReset(colaMovimiento); TODO: lugar de inicio

                if (((eventos & TATAMI_TD) != 0) || ((eventos & TATAMI_TI) != 0))
                {
                    //UARTprintf("Sensor de linea Trasero \r\n");

                    mov.t = 'c';
                    mov.v = 9;
                    xQueueSend(colaMovimiento, &mov, 0);

                    mov.t = 'g';
                    mov.v = -180;
                    xQueueSend(colaMovimiento, &mov, 0);

                    xEventGroupSetBits(FlagsEventos, MOV);

                    en_movimiento = 1;
                }
                else if (((eventos & TATAMI_DD) != 0) || ((eventos & TATAMI_DI) != 0))
                {
                    //UARTprintf("Sensor de linea Delantero\r\n");

                    if ((eventos & VAL_WHISK_BACK)  != 0)
                    {
                        mov.t = 'g';
                        mov.v = 90;
                        xQueueSend(colaMovimiento, &mov, 0);

                        mov.t = 'c';
                        mov.v = 9;
                        xQueueSend(colaMovimiento, &mov, 0);

                        mov.t = 'g';
                        mov.v = 90;
                        xQueueSend(colaMovimiento, &mov, 0);

                        mov.t = 'c';
                        mov.v = 3;
                        xQueueSend(colaMovimiento, &mov, 0);

                        xEventGroupSetBits(FlagsEventos, MOV);

                        en_movimiento = 1;
                    }
                    else
                    {
                        mov.t = 'c';
                        mov.v = -9;
                        xQueueSend(colaMovimiento, &mov, 0);

                        mov.t = 'g';
                        mov.v = 180;
                        xQueueSend(colaMovimiento, &mov, 0);

                        xEventGroupSetBits(FlagsEventos, MOV);

                        en_movimiento = 1;
                    }
                }
                else if ((eventos & VAL_WHISK_BACK)  != 0)
                {
                    //UARTprintf("Whisker trasero \r\n");

                    mov.t = 'c';
                    mov.v = 9;
                    xQueueSend(colaMovimiento, &mov, 0);

                    mov.t = 'g';
                    mov.v = 90;
                    xQueueSend(colaMovimiento, &mov, 0);

                    mov.t = 'c';
                    mov.v = 3;
                    xQueueSend(colaMovimiento, &mov, 0);

                    mov.t = 'g';
                    mov.v = 90;
                    xQueueSend(colaMovimiento, &mov, 0);

                    xEventGroupSetBits(FlagsEventos, MOV);

                    en_movimiento = 1;
                }
                else if (en_movimiento == 0)
                {
                    xQueueReset(colaMovimiento);
                    modo = BUSQUEDA;
                }
            }

            break;

            default:
            {

            }
        }
    }
}


//*****************************************************************************
//
//! TareaMovimiento
//!
//! Esta tarea se encargara del control automatico de los motores. Es decir,
//! se encarga de mover el microbot un numero de centimetros o un numero de
//! grados.
//!
//! Tipo: hebra
//
//*****************************************************************************
static portTASK_FUNCTION (TareaMovimiento, pvParameters)
{
    struct pareja mov_recibido;
    int pasos_cm[] = {0, 0};
    int pasos_grados[] = {0, 0};
    EventBits_t eventos;

    while (1)
    {
        eventos = xEventGroupWaitBits(FlagsEventos, MOV | PASOS_RUEDA_IZQ | PASOS_RUEDA_DER, pdTRUE, pdFALSE, portMAX_DELAY);

        if ((eventos & MOV) != 0)
        {
            if ((objetivo_cm == 0) && (objetivo_grados == 0))
            {
                if (xQueueReceive(colaMovimiento, &mov_recibido, 0) == pdTRUE)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);

                    if (mov_recibido.t == 'c')
                    {
                        if (mov_recibido.v >= 0)
                        {
                            mov_rueda_universal(RUEDA_IZQUIERDA, 100);
                            mov_rueda_universal(RUEDA_DERECHA, 90);
                            objetivo_cm = round(mov_recibido.v/3);
                        }
                        else if (mov_recibido.v < 0)
                        {
                            mov_rueda_universal(RUEDA_IZQUIERDA, -100);
                            mov_rueda_universal(RUEDA_DERECHA, -90);
                            objetivo_cm = round(mov_recibido.v/-3);
                        }
                    }
                    else if (mov_recibido.t == 'g')
                    {
                        if (mov_recibido.v >= 0)
                        {
                            mov_rueda_universal(RUEDA_IZQUIERDA, 100);
                            mov_rueda_universal(RUEDA_DERECHA, -90);
                            objetivo_grados = round(mov_recibido.v/30);
                        }
                        else if (mov_recibido.v < 0)
                        {
                            mov_rueda_universal(RUEDA_IZQUIERDA, -100);
                            mov_rueda_universal(RUEDA_DERECHA, 90);
                            objetivo_grados = round(mov_recibido.v/-30);
                        }
                    }
                }
            }
        }
        else if ((eventos & PASOS_RUEDA_IZQ)  != 0)
        {
            if (objetivo_cm != 0)
            {
                pasos_cm[0]++;

                if (pasos_cm[0] == objetivo_cm)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_cm = 0;
                    pasos_cm[0] = 0;
                    pasos_cm[1] = 0;

                    // Si esto nos da != 0 quiere decir que tenemos mensajes esperando.
                    if (uxQueueMessagesWaiting(colaMovimiento) != 0)
                    {
                        xEventGroupSetBits(FlagsEventos, MOV);
                    }
                    else
                    {
                        en_movimiento = 0;
                        CASIO_on = 0;
                    }
                }
            }

            if (objetivo_grados != 0)
            {
                pasos_grados[0]++;

                if (pasos_grados[0] == objetivo_grados)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_grados = 0;
                    pasos_grados[0] = 0;
                    pasos_grados[1] = 0;

                    if (uxQueueMessagesWaiting(colaMovimiento) != 0)
                    {
                        xEventGroupSetBits(FlagsEventos, MOV);
                    }
                    else
                    {
                        en_movimiento = 0;
                        CASIO_on = 0;
                    }
                }
            }
        }
        else if ((eventos & PASOS_RUEDA_DER)  != 0)
        {
            if (objetivo_cm != 0)
            {
                pasos_cm[1]++;

                if (pasos_cm[1] == objetivo_cm)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_cm = 0;
                    pasos_cm[0] = 0;
                    pasos_cm[1] = 0;

                    if (uxQueueMessagesWaiting(colaMovimiento) != 0)
                    {
                        xEventGroupSetBits(FlagsEventos, MOV);
                    }
                    else
                    {
                        en_movimiento = 0;
                        CASIO_on = 0;
                    }
                }
            }

            if (objetivo_grados != 0)
            {
                pasos_grados[1]++;

                if (pasos_grados[1] == objetivo_grados)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_grados = 0;
                    pasos_grados[0] = 0;
                    pasos_grados[1] = 0;

                    if (uxQueueMessagesWaiting(colaMovimiento) != 0)
                    {
                        xEventGroupSetBits(FlagsEventos, MOV);
                    }
                    else
                    {
                        en_movimiento = 0;
                        CASIO_on = 0;
                    }
                }
            }
        }
    }
}


//*****************************************************************************
//
//! TareaADC0Sharp
//!
//! Tarea encargada de monitorizar el sensor Sharp y avisar si ve algo.
//! Funciona usando una cola.
//!
//! Tipo: hebra
//
//*****************************************************************************
static portTASK_FUNCTION (TareaADC0Sharp, pvParameters)
{
    MuestrasADC lectura;
    uint32_t valor;

    while (1)
    {
        configADC0_LeeADC0(&lectura);

        if (lectura.chan >= 1480 && lectura.chan <= 3760)
        {
            valor = VAL_SHARP_CERCANO;
            xQueueSend(colaSharp, &valor, 0);
        }
        else if (lectura.chan >= 730 && lectura.chan < 1480)
        {
            valor = VAL_SHARP_MEDIO;
            xQueueSend(colaSharp, &valor, 0);
        }
        else if (lectura.chan >= 600 && lectura.chan < 730)
        {
            valor = VAL_SHARP_LEJOS;
            xQueueSend(colaSharp, &valor, 0);
        }
        else
        {
            valor = VAL_SHARP_NO_VISIBLE;
            xQueueSend(colaSharp, &valor, 0);
        }

        vTaskDelay(portTICK_PERIOD_MS);
    }
}



// *** Funcion main *** //
int main(void)
{
    FlagsEventos = xEventGroupCreate();
    if (FlagsEventos == NULL)
    {
        while (1);
    }

    colaSharp = xQueueCreate(1, sizeof(uint32_t));
    if (colaSharp == NULL)
    {
        while (1);
    }

    colaMovimiento = xQueueCreate(12, sizeof(struct pareja));
    if (colaMovimiento == NULL)
    {
        while (1);
    }

    // Elegir reloj adecuado para los valores de ciclos sean de tamano soportable:
    // vamos a elegir un reloj del sistema a 40 MHz
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    // Get the system clock speed.
    g_ulSystemClock = SysCtlClockGet();
    // Habilita el clock gating de los perifericos durante el bajo consumo,
    // perifericos que se desee activos en modo Sleep, deben habilitarse con SysCtlPeripheralSleepEnable
    SysCtlPeripheralClockGating(true);
    // Inicializa el subsistema de medida del uso de CPU (mide el tiempo que la CPU no esta dormida)
    // Para eso utiliza un timer, que aqui hemos puesto que sea el TIMER5 (ultimo parametro que se pasa a la funcion)
    // (y por tanto este no se deberia utilizar para otra cosa).
    CPUUsageInit(g_ulSystemClock, configTICK_RATE_HZ/10, 5);


    // Llamamos a la funcion de inicializacion de los Servos
    servos_init();
    // Llamamos a la funcion de inicializacion de los Sensores
    config_sensors();
    // Funcion para activar ADC_0
    configADC0_IniciaADC0();


    // TAREA CON LA MAQUINA DE ESTADOS
    if ((xTaskCreate(TareaMaestra, "MasterTask", MASTERTASKSIZE, NULL, tskIDLE_PRIORITY + MASTERTASKPRIO, NULL) != pdTRUE))
    {
        while (1);
    }

    // TAREA RELACIONADAS CON EL MOVIMIENTO DE LOS SERVOS
    if ((xTaskCreate(TareaMovimiento, "TaskMov", MOVTASKSIZE, NULL, tskIDLE_PRIORITY + MOVTASKPRIO, NULL) != pdTRUE))
    {
        while (1);
    }

    // TAREA DEL SHARP
    if ((xTaskCreate(TareaADC0Sharp, "TaskADC0", ADCTASKSIZE, NULL, tskIDLE_PRIORITY + PATASKPRIO, NULL) != pdTRUE))
    {
        while (1);
    }

    // TAREA DE COMANDOS
    if (initCommandLine(512, tskIDLE_PRIORITY + 1) != pdTRUE)
    {
        while (1);
    }


    //Creacion Timer CASIO
    CASIO_BUSQUEDA = xTimerCreate("TIMER_BUSQUEDA", 4000/portTICK_PERIOD_MS, pdTRUE, NULL, CASIOCallback);
    if(CASIO_BUSQUEDA == NULL)
    {
        while(1);
    }

    //Creacion Timer Whisker
    CASIO_REBOTON = xTimerCreate("TIMER_WHISKER", 200/portTICK_PERIOD_MS, pdFALSE, NULL, CASIOCallback_2);
    if(CASIO_REBOTON == NULL)
    {
        while(1);
    }

    vTaskStartScheduler();

    while (1);
}




// *** ISRs *** //

//  Interrupcion boton derecho e izquierdo para inicializar el modo de busqueda y modo reset
void GPIOFIntHandler(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    int32_t i32PinStatus = MAP_GPIOIntStatus(GPIO_PORTF_BASE, ALL_BUTTONS);

    if ((i32PinStatus & RIGHT_BUTTON))
    {
        ON_INI = true;
        en_movimiento = 0;
        modo = BUSQUEDA;
    }

    MAP_GPIOIntClear(GPIO_PORTF_BASE, ALL_BUTTONS);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}


// Manejador del pin PA para las interrupciones del whisker y Sharp
void GPIOInt_A_Handler(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    int32_t i32PinStatus = GPIOIntStatus(GPIO_PORTA_BASE, true);

    if (ON_INI)
    {
        if ((i32PinStatus & PIN_WHISK_FRONT) && (detectar == 1))
        {
            xEventGroupSetBitsFromISR(FlagsEventos, VAL_WHISK_FRONT, &higherPriorityTaskWoken);
            modo = ATAQUE;
            detectar = 0;

            if(xTimerStart(CASIO_REBOTON, 0) != pdPASS)
            {
                while(1);
            }
        }

        if ((i32PinStatus & PIN_WHISK_BACK) && (detectar == 1))
        {
            xEventGroupSetBitsFromISR(FlagsEventos, VAL_WHISK_BACK, &higherPriorityTaskWoken);
            xQueueReset(colaMovimiento);
            modo = HUIDA;
            detectar = 0;

            if(xTimerStart(CASIO_REBOTON, 0) != pdPASS)
            {
                while(1);
            }
        }

        if (i32PinStatus & PIN_CNY_IZQ)
        {
            xEventGroupSetBitsFromISR(FlagsEventos, PASOS_RUEDA_IZQ, &higherPriorityTaskWoken);
        }

        if (i32PinStatus & PIN_CNY_DER)
        {
            xEventGroupSetBitsFromISR(FlagsEventos, PASOS_RUEDA_DER, &higherPriorityTaskWoken);
        }
    }

    MAP_GPIOIntClear(GPIO_PORTA_BASE, PIN_WHISK_FRONT | PIN_WHISK_BACK | PIN_CNY_IZQ | PIN_CNY_DER);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}


void GPIOInt_C_Handler(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    int32_t i32PinStatus = GPIOIntStatus(GPIO_PORTC_BASE, true);

    if (ON_INI)
    {
        if ((i32PinStatus & PIN_LIN_DD))
        {
            xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_DD, &higherPriorityTaskWoken);
            xQueueReset(colaMovimiento);
            modo = HUIDA;
        }

        if ((i32PinStatus & PIN_LIN_DI))
        {
            xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_DI, &higherPriorityTaskWoken);
            xQueueReset(colaMovimiento);
            modo = HUIDA;
        }

        if ((i32PinStatus & PIN_LIN_TD))
        {
            xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_TD, &higherPriorityTaskWoken);
            xQueueReset(colaMovimiento);
            modo = HUIDA;
        }

        if ((i32PinStatus & PIN_LIN_TI))
        {
            xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_TI, &higherPriorityTaskWoken);
            xQueueReset(colaMovimiento);
            modo = HUIDA;
        }
    }

    MAP_GPIOIntClear(GPIO_PORTC_BASE, PIN_LIN_DD | PIN_LIN_DI | PIN_LIN_TD | PIN_LIN_TI);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

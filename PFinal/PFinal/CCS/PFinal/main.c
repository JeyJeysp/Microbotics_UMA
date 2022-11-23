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

//  Declaracion de los arrays para la tabla look-up
unsigned short val_distancia[] = {30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 3};
unsigned short val_voltaje_hex[] = {0x130, 0x152, 0x165, 0x186, 0x1A3, 0x1E2, 0x216, 0x23F,
                                    0x2B0, 0x310, 0x3BE, 0x4A4, 0x5DF, 0x828, 0x9B5};

//  Declaracion de globales
QueueHandle_t colaCM, colaGrados, colaSharp;
uint32_t g_ui32CPUUsage, g_ulSystemClock;
EventGroupHandle_t FlagsEventos;
SemaphoreHandle_t semWhisker;

//  Definicion de la prioridad para la tarea y el tamano de pila
#define MASTERTASKPRIO 1
#define MASTERTASKSIZE 256
#define MOVTASKPRIO 2
#define MOVTASKSIZE 128
#define ADCTASKPRIO 2
#define ADCTASKSIZE 128
#define PATASKPRIO 2

// Definicion de los modos
#define BUSQUEDA 0
#define ATAQUE 1
#define HUIDA 2
#define INTERRUPCION 3
int modo = -1;

// Creamos un contador que cuente la cantidad de veces que no recibe dato del sharp
uint32_t contSharp = 0;
// Ponemos un umbral de 10 datos recibidos
uint32_t umbralSharp = 10;
int show_once[] = {0, 0, 0};

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

static portTASK_FUNCTION (TareaMaestra, pvParameters)
{
    uint32_t ui32Status;
    EventBits_t eventos;
    int grados, cm;

    while(1)
    {
        switch(modo)
        {
            case BUSQUEDA:
            {
                if (show_once[0] == 0)
                {
                    show_once[1] = show_once[2] = 0;
                    show_once[0] = 1;
                    UARTprintf("Entro en modo Busqueda \r\n");
                }
                // El robot rota de manera indefinida hasta encontrar un objeto
                // Cuando detecte la interrupcion del sharp o whisker pasar� al modo de ataque
                mov_rueda_universal(RUEDA_IZQUIERDA, -100);
                mov_rueda_universal(RUEDA_DERECHA, 90);

                while(modo == BUSQUEDA)
                {
                    eventos = xEventGroupWaitBits(FlagsEventos, VAL_WHISK_FRONT | VAL_WHISK_BACK, pdFALSE, pdFALSE, 0);

                    if ((xQueueReceive(colaSharp, &ui32Status, 0) == pdTRUE))
                    {
                        if(ui32Status != VAL_SHARP_NO_VISIBLE)
                        {
                            modo = ATAQUE;
                        }
                    }

                    if ((eventos & VAL_WHISK_FRONT)  != 0)
                    {
                        modo = ATAQUE;
                    }

                    if ((eventos & VAL_WHISK_BACK)  != 0)
                    {
                        modo = HUIDA;
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
                    show_once[0] = show_once[2] = 0;
                    show_once[1] = 1;
                    UARTprintf("Entro en modo Ataque \r\n");
                }
                mov_rueda_universal(RUEDA_IZQUIERDA, 100);
                mov_rueda_universal(RUEDA_DERECHA, 90);

                while(modo == ATAQUE)
                {
                    if (contSharp == umbralSharp)
                    {
                        contSharp = 0;
                        modo = BUSQUEDA;
                    }

                    if ((xQueueReceive(colaSharp, &ui32Status, 0) == pdTRUE))
                    {
                        if(ui32Status == VAL_SHARP_NO_VISIBLE)
                        {
                            contSharp++;
                        }
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
                    show_once[0] = show_once[1] = 0;
                    show_once[2] = 1;
                    UARTprintf("Entro en modo Huida \r\n");
                }

                eventos = xEventGroupWaitBits(FlagsEventos, TATAMI_DD | TATAMI_DI | TATAMI_TD | TATAMI_TI | VAL_WHISK_FRONT | VAL_WHISK_BACK, pdTRUE, pdFALSE, 0);

                /*
                if ((eventos & TATAMI_DD)  != 0)
                {
                    UARTprintf("Sensor de linea Delantero Derecho \r\n");
                }

                if ((eventos & TATAMI_DI)  != 0)
                {
                    UARTprintf("Sensor de linea Delantero Izquierdo \r\n");
                }

                if ((eventos & TATAMI_TD)  != 0)
                {
                    UARTprintf("Sensor de linea Trasero Derecho \r\n");
                }

                if ((eventos & TATAMI_TI)  != 0)
                {
                    UARTprintf("Sensor de linea Trasero Izquierdo \r\n");
                }
                */

                if ((eventos & (TATAMI_DD || TATAMI_DI))  != 0)
                {
                    UARTprintf("Sensor de linea Delantero \r\n");

                    grados = 180;
                    xQueueSend(colaGrados, &grados, portMAX_DELAY);
                    xEventGroupSetBits(FlagsEventos, GIRAR);

                    modo = BUSQUEDA;
                }

                if ((eventos & (TATAMI_TD || TATAMI_TI))  != 0)
                {
                    UARTprintf("Sensor de linea Trasero \r\n");

                    grados = -180;
                    xQueueSend(colaGrados, &grados, portMAX_DELAY);
                    xEventGroupSetBits(FlagsEventos, GIRAR);

                    modo = BUSQUEDA;
                }

                if ((eventos & VAL_WHISK_FRONT)  != 0)
                {
                    UARTprintf("Whisker delantero \r\n");
                }

                if ((eventos & VAL_WHISK_BACK)  != 0)
                {
                    UARTprintf("Whisker trasero \r\n");
                }
            }

            break;

            case INTERRUPCION:
            {

            }

            break;

            default:
            {

            }
        }
    }
}

static portTASK_FUNCTION (TareaMovimiento, pvParameters)
{
    int pasos_cm[] = {0, 0};
    int pasos_grados[] = {0, 0};
    int objetivo_cm = 0, objetivo_grados = 0;

    int mensaje_mov = 0;

    EventBits_t eventos;

    while (1)
    {
        eventos = xEventGroupWaitBits(FlagsEventos, RECTO|GIRAR|PASOS_RUEDA_IZQ|PASOS_RUEDA_DER, pdTRUE, pdFALSE, portMAX_DELAY);


        if ((eventos & RECTO) != 0)
        {
            // Cola con la cantidad de centimetros a recorrer
            if (xQueueReceive(colaCM, &mensaje_mov, portMAX_DELAY) == pdTRUE)
            {
                /*
                    CUENTAS:

                    la distancia recorrida se rige por D = (R/2)*(angulo_L + angulo_R)
                    Suponiendo que 'angulo_R = angulo_L', D = (R/2)*(2*angulo) = R*angulo

                    Teniendo en cuenta que nuestro CNY se activa con flanco de subida, lo haría
                    al pasar de la franja blanca a, nuevamente, otra franja blanca. En ese tiempo
                    recorremos un ángulo de 60º, lo que se traduce por (con R=3) una distancia
                    de en torno a 3 cm. Es decir, una vez activemos el motor, cada vez que salte
                    una interrupcion habremos avanzado 3 cm aproximadamente.

                    Por ello, si queremos calcular cuantas interrupciones necesitamos, basta con
                    hacer el siguiente calculo: num_vueltas = c/R (siendo c la distancia que se le
                    pasa a la funcion).

                    NOTAS:

                    1. Los angulos deben ser tratados en radianes (angulo*pi/180)
                    2. La rueda mide 18.85 cm (segun 'l = 2*pi*R')
                    3. La distancia que podemos recorrer debe ser multiplo de 3.
                 */

                if (mensaje_mov >= 0)
                {
                    mov_rueda_universal(RUEDA_IZQUIERDA, 100);
                    mov_rueda_universal(RUEDA_DERECHA, 90);
                    UARTprintf("CM %d\r\n", mensaje_mov);
                    objetivo_cm = round(mensaje_mov/3);
                }
                else
                {
                    mov_rueda_universal(RUEDA_IZQUIERDA, -100);
                    mov_rueda_universal(RUEDA_DERECHA, -90);
                    UARTprintf("CM %d\r\n", mensaje_mov);
                    objetivo_cm = round(mensaje_mov/-3);
                }
            }
        }

        if ((eventos & GIRAR) != 0)
        {
            // Cola con la cantidad de grados para girar
            if (xQueueReceive(colaGrados, &mensaje_mov, portMAX_DELAY) == pdTRUE)
            {
                if (mensaje_mov >= 0)
                {
                    mov_rueda_universal(RUEDA_IZQUIERDA, 100);
                    mov_rueda_universal(RUEDA_DERECHA, -90);
                    UARTprintf("GRADOS %d\r\n", mensaje_mov);
                    objetivo_grados = round(mensaje_mov/30);
                }
                else
                {
                    mov_rueda_universal(RUEDA_IZQUIERDA, -100);
                    mov_rueda_universal(RUEDA_DERECHA, 90);
                    UARTprintf("GRADOS %d\r\n", mensaje_mov);
                    objetivo_grados = round(mensaje_mov/-30);
                }
            }
        }

        if ((eventos & PASOS_RUEDA_IZQ)  != 0)
        {
            if (objetivo_cm != 0)
            {
                pasos_cm[0]++;
                UARTprintf("PASOS CM 0 %d\r\n", pasos_cm[0]);
                if (pasos_cm[0] == objetivo_cm)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_cm = 0;
                    pasos_cm[1] = 0;
                    pasos_cm[0] = 0;;
                    xSemaphoreGive(semWhisker);
                }
            }

            if (objetivo_grados != 0)
            {
                pasos_grados[0]++;
                UARTprintf("PASOS GRADOS 0 %d\r\n", pasos_grados[0]);
                if (pasos_grados[0] == objetivo_grados)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_grados = 0;
                    pasos_grados[0] = 0;
                    pasos_grados[1] = 0;
                    xSemaphoreGive(semWhisker);
                }
            }
        }

        if ((eventos & PASOS_RUEDA_DER)  != 0)
        {
            if (objetivo_cm != 0)
            {
                pasos_cm[1]++;
                UARTprintf("PASOS CM 1 %d\r\n", pasos_cm[1]);
                if (pasos_cm[1] == objetivo_cm)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_cm = 0;
                    pasos_cm[1] = 0;
                    pasos_cm[0] = 0;
                    xSemaphoreGive(semWhisker);
                }
            }

            if (objetivo_grados != 0)
            {
                pasos_grados[1]++;
                UARTprintf("PASOS GRADOS 1 %d\r\n", pasos_grados[1]);
                if (pasos_grados[1] == objetivo_grados)
                {
                    mov_rueda_universal(AMBAS_RUEDAS, 0);
                    objetivo_grados = 0;
                    pasos_grados[0] = 0;
                    pasos_grados[1] = 0;
                    xSemaphoreGive(semWhisker);
                }
            }
        }
    }
}

static portTASK_FUNCTION (TareaADC0Sharp, pvParameters)
{
    MuestrasADC lectura;
    uint32_t valor;

    while (1)
    {
        configADC0_LeeADC0(&lectura);
        //UARTprintf("Lectura %d\r\n", lectura.chan);

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

int main(void)
{
    colaSharp = xQueueCreate(1, sizeof(uint32_t));
    if (colaSharp == NULL)
    {
        while (1);
    }

    semWhisker = xSemaphoreCreateBinary();
    if (semWhisker == NULL)
    {
        while(1);
    }

    //Crea el grupo de eventos
    FlagsEventos = xEventGroupCreate();
    if (FlagsEventos == NULL)
    {
        while (1);
    }

    colaCM = xQueueCreate(10, sizeof(int));
    if (colaCM == NULL)
    {
        while (1);
    }

    colaGrados = xQueueCreate(10, sizeof(int));
    if (colaGrados == NULL)
    {
        while (1);
    }


    // Elegir reloj adecuado para los valores de ciclos sean de tamano soportable
    // Vamos a elegir un reloj del sistema a 40 MHz
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

    if (initCommandLine(512, tskIDLE_PRIORITY + 1) != pdTRUE)
    {
        while (1);
    }

    vTaskStartScheduler();

    while (1);
}


//  Interrupcion boton derecho para inicializar el modo de busqueda
void GPIOFIntHandler(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    int32_t i32PinStatus = MAP_GPIOIntStatus(GPIO_PORTF_BASE, ALL_BUTTONS);

    if ((i32PinStatus & RIGHT_BUTTON))
    {
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

    if (i32PinStatus & PIN_WHISK_FRONT)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, VAL_WHISK_FRONT, &higherPriorityTaskWoken);
    }

    if (i32PinStatus & PIN_WHISK_BACK)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, VAL_WHISK_BACK, &higherPriorityTaskWoken);
    }

    if (i32PinStatus & PIN_CNY_IZQ)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, PASOS_RUEDA_IZQ, &higherPriorityTaskWoken);
    }

    if (i32PinStatus & PIN_CNY_DER)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, PASOS_RUEDA_DER, &higherPriorityTaskWoken);
    }

    MAP_GPIOIntClear(GPIO_PORTA_BASE, PIN_WHISK_FRONT | PIN_WHISK_BACK | PIN_CNY_IZQ | PIN_CNY_DER);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

void GPIOInt_C_Handler(void)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    int32_t i32PinStatus = GPIOIntStatus(GPIO_PORTC_BASE, true);

    if (i32PinStatus & PIN_LIN_DD)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_DD, &higherPriorityTaskWoken);
        modo = HUIDA;
    }

    if (i32PinStatus & PIN_LIN_DI)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_DI, &higherPriorityTaskWoken);
        modo = HUIDA;
    }

    if (i32PinStatus & PIN_LIN_TD)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_TD, &higherPriorityTaskWoken);
        modo = HUIDA;
    }

    if (i32PinStatus & PIN_LIN_TI)
    {
        xEventGroupSetBitsFromISR(FlagsEventos, TATAMI_TI, &higherPriorityTaskWoken);
        modo = HUIDA;
    }

    MAP_GPIOIntClear(GPIO_PORTC_BASE, PIN_LIN_DD | PIN_LIN_DI | PIN_LIN_TD | PIN_LIN_TI);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

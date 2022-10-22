// En este archivo se configura el ADC al completo:
// 1.- Trigger
// 2.- Inicialización de la secuencia
// 3.- Recibir de la cola
// 4.- Interrupción del ADC

#include<stdint.h>
#include<stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_adc.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "FreeRTOS.h"
#include "configADC.h"
#include "task.h"
#include "queue.h"

#include "Sensors.h"


// Al ser una cola static, la cola solo se puede emplear en este fichero
static QueueHandle_t cola_adc;

void configADC_DisparaADC(void)
{
    ADCProcessorTrigger(ADC0_BASE, 3);
}

void configADC0_IniciaADC0(void)
{
    // ADC0 empleado para polling, para los 6 canales
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Especificamos que el periferico siga encendido en modo de bajo consumo
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);

    //HABILITAMOS EL GPIOE PARA LA LECTURA DEL SHARP DELANTERO
    // Para configurar el multiplexor de señales
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);

    // Enable pin PE3 for ADC AIN0
    GPIOPinTypeADC(GPIO_PORTE_BASE, PIN_SHARP_FRONT);

    // CONFIGURAR ADC0, Vamos a configurar el secuenciador 3 porque admite 1 paso
    // Secuenciador 0 -> admite 8, Secuenciador 1/2 -> admite 4 y Secuenciador 3 -> admite 1
    ADCSequenceDisable(ADC0_BASE, 3);

    // CONFIGURAR ADC0, Configuramos la velocidad de conversion al maximo (1MS/s)
    // 1us por cada muestra
    ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_RATE_FULL, 1);



    // Vamos a configurar el disparo por timer
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER2);

    //Configurar temporizador como periódico
    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
    //Cargar el valor que contará el temporizador
    TimerLoadSet(TIMER2_BASE, TIMER_A, (SysCtlClockGet()/2000));
    //Habilitar disparador de ADC por temporizador
    TimerControlTrigger(TIMER2_BASE, TIMER_A, 1);



    //Disparo software (processor trigger)
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 0);

    // Vamos a hacer oversample. Este recoge 4 muestras y hace la media
    ADCHardwareOversampleConfigure(ADC0_BASE, 64);

    //Solo tenemos una muestra que provoca la interrupcion
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0| ADC_CTL_IE | ADC_CTL_END);

    //ACTIVO LA SECUENCIA
    ADCSequenceEnable(ADC0_BASE, 3);

    //Habilita las interrupciones, ADC0
    ADCIntClear(ADC0_BASE, 3);
    ADCIntEnable(ADC0_BASE, 3);
    IntPrioritySet(INT_ADC0SS3, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    IntEnable(INT_ADC0SS3);

    //Creamos una cola de mensajes para la comunicacion entre la ISR y la tara que llame a configADC_LeeADC(...)
    // Se ha modificado las estructuras donde se encuentra definida "MuestraADC"
    cola_adc = xQueueCreate(1, sizeof(MuestrasADC));
    if (cola_adc == NULL)
    {
        while(1);
    }



    // Una vez configurado el ADC0, habilitamos la cuenta del Timer
    TimerEnable(TIMER2_BASE, TIMER_A);
}

void configADC0_LeeADC0(MuestrasADC *datos)
{
    xQueueReceive(cola_adc, datos, portMAX_DELAY);
}

void configADC0_ISR(void)
{
    portBASE_TYPE higherPriorityTaskWoken = pdFALSE;

    MuestrasLeida leidas;
    MuestrasADC finales;
    ADCIntClear(ADC0_BASE, 3);//LIMPIAMOS EL FLAG DE INTERRUPCIONES

    // Hay que tener cuidado con el tipo empleado para coger los datos guardados
    ADCSequenceDataGet(ADC0_BASE, 3, (uint32_t *)&leidas);//COGEMOS LOS DATOS GUARDADOS

    //Pasamos de 32 bits a 16 (el conversor es de 12 bits, así que sólo son significativos los bits del 0 al 11)
    // valores de entre 0 hasta 4096
    finales.chan = leidas.chan;

    //Guardamos en la cola
    xQueueSendFromISR(cola_adc, &finales, &higherPriorityTaskWoken);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

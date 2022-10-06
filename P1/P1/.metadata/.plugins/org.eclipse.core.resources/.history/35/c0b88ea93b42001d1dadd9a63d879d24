//*****************************************************************************
//
// commands.c - FreeRTOS porting example on CCS4
//
// Este fichero contiene errores que seran explicados en clase
//
//*****************************************************************************


#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* Standard TIVA includes */
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"

#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"

/* Other TIVA includes */
#include "utils/cpu_usage.h"
#include "utils/cmdline.h"
#include "utils/uartstdio.h"

#include "drivers/rgb.h"

// ==============================================================================
// The CPU usage in percent, in 16.16 fixed point format.
// ==============================================================================
extern uint32_t g_ui32CPUUsage;

// ==============================================================================
// Implementa el comando cpu que muestra el uso de CPU
// ==============================================================================
static int  Cmd_cpu(int argc, char *argv[])
{
    //
    // Print some header text.
    //
    UARTprintf("ARM Cortex-M4F %u MHz - ",SysCtlClockGet() / 1000000);
    UARTprintf("%2u%% de uso\r\n", (g_ui32CPUUsage+32768) >> 16);

    // Return success.
    return(0);
}

// ==============================================================================
// Implementa el comando free, que muestra cuanta memoria "heap" le queda al FreeRTOS
// ==============================================================================
static int Cmd_free(int argc, char *argv[])
{
    //
    // Print some header text.
    //
    UARTprintf("%d bytes libres\r\n", xPortGetFreeHeapSize());

    // Return success.
    return(0);
}

// ==============================================================================
// Implementa el comando task. S�lo es posible si la opci�n configUSE_TRACE_FACILITY de FreeRTOS est� habilitada
// ==============================================================================
#if ( configUSE_TRACE_FACILITY == 1 )

extern char *__stack;
static  int Cmd_tasks(int argc, char *argv[])
{
	char*	pcBuffer;
	uint8_t*	pi8Stack;
	portBASE_TYPE	x;
	
	pcBuffer = pvPortMalloc(1024);
	vTaskList(pcBuffer);
	UARTprintf("\t\t\t\tUnused\r\nTaskName\tStatus\tPri\tStack\tTask ID\r\n");
	UARTprintf("=======================================================\r\n");
	UARTprintf("%s", pcBuffer);
	
	// Calculate kernel stack usage
	x = 0;
	pi8Stack = (uint8_t *) &__stack;
	while (*pi8Stack++ == 0xA5)
	{
		x++;	//Esto s�lo funciona si hemos rellenado la pila del sistema con 0xA5 en el arranque
	}
	sprintf((char *) pcBuffer, "%%%us", configMAX_TASK_NAME_LEN);
	sprintf((char *) &pcBuffer[10], (const char *) pcBuffer, "kernel");
	UARTprintf("%s\t-\t*%u\t%u\t-\r\n", &pcBuffer[10], configKERNEL_INTERRUPT_PRIORITY, x/sizeof(portBASE_TYPE));
	vPortFree(pcBuffer);
	return 0;
}

#endif /* configUSE_TRACE_FACILITY */

#if configGENERATE_RUN_TIME_STATS
// ==============================================================================
// Implementa el comando "Stats"
// ==============================================================================
static Cmd_stats(int argc, char *argv[])
{
	char*	pBuffer;

	pBuffer = pvPortMalloc(1024);
	if (pBuffer)
	{
		vTaskGetRunTimeStats(pBuffer); //Es un poco inseguro, pero por ahora nos vale...
		UARTprintf("TaskName\tCycles\t\tPercent\r\n");
		UARTprintf("===============================================\r\r\n");
		UARTprintf("%s", pBuffer);
		vPortFree(pBuffer);
	}
	return 0;
}
#endif

// ==============================================================================
// Implementa el comando help
// ==============================================================================
static int Cmd_help(int argc, char *argv[])
{
    tCmdLineEntry *pEntry;

    //
    // Print some header text.
    //
    UARTprintf("Comandos disponibles\r\n");
    UARTprintf("------------------\r\n");

    //
    // Point at the beginning of the command table.
    //
    pEntry = &g_psCmdTable[0];

    //
    // Enter a loop to read each entry from the command table.  The end of the
    // table has been reached when the command name is NULL.
    //
    while(pEntry->pcCmd)
    {
        //
        // Print the command name and the brief description.
        //
        UARTprintf("%s%s\r\n", pEntry->pcCmd, pEntry->pcHelp);

        //
        // Advance to the next entry in the table.
        //
        pEntry++;
    }

    //
    // Return success.
    //
    return(0);
}

// ==============================================================================
// Implementa el comando "LED"
// ==============================================================================
static int Cmd_led(int argc, char *argv[])
{
    uint32_t cualLED;

	if (argc != 3)
	{
		//Si los parametros no son suficientes o son demasiados, muestro la ayuda
		UARTprintf(" led [red|green|blue] [on|off]\r\n");
	}
	else
	{
	    if (0==strncmp( argv[1], "red",3))
	    {

	        cualLED=GPIO_PIN_1;
	    }
	    else if (0==strncmp( argv[1], "green",5))
	    {
	        cualLED=GPIO_PIN_3;
	    }
	    else if (0==strncmp(argv[1], "blue",4))
	    {
	        cualLED=GPIO_PIN_2;
	    }
	    else
	    {
	        UARTprintf(" led [red|green|blue] [on|off]\r\n");
	        return 0;
	    }

		if (0==strncmp( argv[2], "on",2))
		{
			UARTprintf("Enciendo el LED\r\n");
			GPIOPinWrite(GPIO_PORTF_BASE, cualLED,cualLED);
		}
		else if (0==strncmp( argv[2], "off",3))
		{
			UARTprintf("Apago el LED\r\n");
			GPIOPinWrite(GPIO_PORTF_BASE,cualLED,0);
		}
		else
		{
			//Si el parametro no es correcto, muestro la ayuda
			UARTprintf(" led [red|green|blue] [on|off]\r\n");
		}

	}


    return 0;
}


// ==============================================================================
// Implementa el comando "MODE"
// ==============================================================================
static int Cmd_mode(int argc, char *argv[])
{
	if (argc != 2)
	{
		//Si los parametros no son suficientes o son demasiados, muestro la ayuda
		UARTprintf(" mode [gpio|pwm]\r\n");
	}
	else
	{
		if (0==strncmp( argv[1], "gpio",4))
		{
			UARTprintf("cambio a modo GPIO\r\n");
			RGBDisable();
			GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
		}
		else if (0==strncmp( argv[1], "pwm",3))
		{
			UARTprintf("Cambio a modo PWM (rgb)\r\n");
			RGBEnable();
		}
		else
		{
			//Si el parametro no es correcto, muestro la ayuda
			UARTprintf(" mode [gpio|pwm]\r\n");
		}
	}


    return 0;
}


static int Cmd_intensity(int argc, char *argv[])
{
    float intensity;

    if (argc != 2)
    {
        //Si los parametros no son suficientes o son demasiados, muestro la ayuda
        UARTprintf(" intensity [floatnum]\r\n");
    }
    else
    {
        intensity=strtof(argv[1],NULL);
        if ((intensity>=0.0)&&(intensity<=1.0))
        {
            RGBIntensitySet(intensity);

        }
        else
        {
            UARTprintf("El valor de intensidad deberia valer entre 0 y 1\r\n");
        }

    }


    return 0;
}


// ==============================================================================
// Implementa el comando "RGB"
// ==============================================================================
static int Cmd_rgb(int argc, char *argv[])
{
	uint32_t arrayRGB[3];

	if (argc != 4)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		UARTprintf(" rgb [red] [green] [blue]\r\n");
	}
	else
	{
		arrayRGB[0]=strtoul(argv[1], NULL, 10)<<8;
		arrayRGB[1]=strtoul(argv[2], NULL, 10)<<8;
		arrayRGB[2]=strtoul(argv[3], NULL, 10)<<8;

		if ((arrayRGB[0]>=65535)||(arrayRGB[1]>=65535)||(arrayRGB[2]>=65535))
		{

			UARTprintf(" \r\n");
		}
		else{
			RGBColorSet(arrayRGB);
		}

	}

    return 0;
}




// ==============================================================================
// Tabla con los comandos y su descripcion. Si quiero anadir alguno, debo hacerlo aqui
// ==============================================================================
//Este array tiene que ser global porque es utilizado por la biblioteca cmdline.c para implementar el interprete de comandos
//No es muy elegante, pero es lo que ha implementado Texas Instruments.
tCmdLineEntry g_psCmdTable[] =
{
    { "help",     Cmd_help,      "     : Lista de comandos" },
    { "?",        Cmd_help,      "        : lo mismo que help" },
    { "cpu",      Cmd_cpu,       "      : Muestra el uso de  CPU " },
    { "led",  	  Cmd_led,       "      : Apaga y enciende el led verde" },
    { "mode",  	  Cmd_mode,       "      : Cambia los pines PF1, PF2 y PF3 entre modo GPIO y modo PWM (rgb)" },
    { "rgb",  	  Cmd_rgb,       "      : Establece el color RGB" },
    { "intensity",      Cmd_intensity,       "      : Cambia el nivel de intensidad" },
    { "free",     Cmd_free,      "     : Muestra la memoria libre" },
#if ( configUSE_TRACE_FACILITY == 1 )
	{ "tasks",    Cmd_tasks,     "    : Muestra informacion de las tareas" },
#endif
#if (configGENERATE_RUN_TIME_STATS)
	{ "stats",    Cmd_stats,      "     : Muestra estadisticas de las tareas" },
#endif
    { 0, 0, 0 }
};

// ==============================================================================
// Tarea UARTTask.  Espera la llegada de comandos por el puerto serie y los ejecuta al recibirlos...
// ==============================================================================
static void vCommandTask( void *pvParameters )
{
    char    pcCmdBuf[64];
    int32_t i32Status;
	
    //
    // Mensaje de bienvenida inicial.
    //
    UARTprintf("\r\n\r\nWelcome to the TIVA FreeRTOS Demo!\r\n");
	UARTprintf("\r\n\r\n FreeRTOS %s \r\n",
		tskKERNEL_VERSION_NUMBER);
	UARTprintf("\r\n Teclee ? para ver la ayuda \r\n");
	UARTprintf("> ");    

	/* Loop forever */
	while (1)
	{

		/* Read data from the UART and process the command line */
		UARTgets(pcCmdBuf, sizeof(pcCmdBuf));
		if (strlen(pcCmdBuf) == 0)
		{
			UARTprintf("> ");
			continue;
		}

		//
		// Pass the line from the user to the command processor.  It will be
		// parsed and valid commands executed.
		//
		i32Status = CmdLineProcess(pcCmdBuf);

		//
		// Handle the case of bad command.
		//
		if(i32Status == CMDLINE_BAD_CMD)
		{
			UARTprintf("Comando erroneo!\r\n");	//No pongo acentos adrede
		}

		//
		// Handle the case of too many arguments.
		//
		else if(i32Status == CMDLINE_TOO_MANY_ARGS)
		{
			UARTprintf("El interprete de comandos no admite tantos parametros\r\n");	//El maximo, CMDLINE_MAX_ARGS, esta definido en cmdline.c
		}

		//
		// Otherwise the command was executed.  Print the error code if one was
		// returned.
		//
		else if(i32Status != 0)
		{
			UARTprintf("El comando devolvio el error %d\r\n",i32Status);
		}

		UARTprintf("> ");

	}
}




//
// Create la tarea que gestiona los comandos (definida en el fichero commands.c)
//
BaseType_t initCommandLine(uint16_t stack_size,uint8_t prioriry )
{

    // Inicializa la UARTy la configura a 115.200 bps, 8-N-1 .
    //se usa para mandar y recibir mensajes y comandos por el puerto serie
    // Mediante un programa terminal como gtkterm, putty, cutecom, etc...
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //Esta funcion habilita la interrupcion de la UART y le da la prioridad adecuada si esta activado el soporte para FreeRTOS
    UARTStdioConfig(0,115200,SysCtlClockGet());
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);   //La UART tiene que seguir funcionando aunque el micro esta dormido

    return xTaskCreate(vCommandTask, (signed portCHAR *)"UartComm", stack_size,NULL,prioriry, NULL);
}

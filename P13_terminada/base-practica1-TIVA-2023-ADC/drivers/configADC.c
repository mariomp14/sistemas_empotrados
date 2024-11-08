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
#include "configADC.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


static QueueHandle_t cola_adc;
static QueueHandle_t cola_adc1;
//*
uint32_t ui32Period;


//Provoca el disparo de una conversion (hemos configurado el ADC con "disparo software" (Processor trigger)
void configADC_DisparaADC(void)
{
	//ADCProcessorTrigger(ADC0_BASE,1);
    ADCProcessorTrigger(ADC0_BASE,0);//0 numero de secuenciador(Disparo software)
    //ADCProcessorTrigger(ADC1_BASE,1);
}


void aplicar_promedio(uint32_t ui32Factor)
{
  ADCHardwareOversampleConfigure(ADC0_BASE,ui32Factor);//sirve para configurar el factor de promediado HW
}



void configADC_IniciaADC(void)
{
			    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
			    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC0);

				//HABILITAMOS EL GPIOE
				SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
				SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
				// Enable pin PE3 for ADC AIN0|AIN1|AIN2|AIN3
				GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0);

				//HABILITAMOS EL GPIOD
				 SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
				 SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOD);
				// Enable pin PD3 for ADC AIN2|AIN3
				  GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3|GPIO_PIN_2);




				//CONFIGURAR SECUENCIADOR 0(YA QUE TIENE 8 posiciones de la fifo)8muestras
				ADCSequenceDisable(ADC0_BASE,0);

				//Configuramos la velocidad de conversion al maximo (1MS/s)
				ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_RATE_FULL, 1);//OJO:EL reloj de conversión e el mismo para los 2 ADC (ADC0 y ADC1).
				                                                     //Esta función no la puedo llamar con el argumento ADC1 siempre con el ADC0.


                //ojo tengo que configurar con la secuencia 0
				ADCSequenceConfigure(ADC0_BASE,0,ADC_TRIGGER_PROCESSOR,0);	//Disparo software (processor trigger)
				ADCSequenceStepConfigure(ADC0_BASE,0,0,ADC_CTL_CH0);
				ADCSequenceStepConfigure(ADC0_BASE,0,1,ADC_CTL_CH1);
				ADCSequenceStepConfigure(ADC0_BASE,0,2,ADC_CTL_CH2);
				ADCSequenceStepConfigure(ADC0_BASE,0,3,ADC_CTL_CH3);
				ADCSequenceStepConfigure(ADC0_BASE,0,4,ADC_CTL_CH4);
				ADCSequenceStepConfigure(ADC0_BASE,0,5,ADC_CTL_CH5);
				ADCSequenceStepConfigure(ADC0_BASE,0,6,ADC_CTL_TS);//termometro
				ADCSequenceStepConfigure(ADC0_BASE,0,7,ADC_CTL_CH6|ADC_CTL_IE |ADC_CTL_END );    //La ultima muestra provoca la interrupcion, ojo este canal no lo utilizamos pero tenemos que ponerlo para rellenar las 8 posiciones
				ADCSequenceEnable(ADC0_BASE,0); //ACTIVO LA SECUENCIA

				//Habilita las interrupciones
				ADCIntClear(ADC0_BASE,0);//el 0 es el secuenciador
				ADCIntEnable(ADC0_BASE,0);
				//IntPrioritySet(INT_ADC0SS1,configMAX_SYSCALL_INTERRUPT_PRIORITY);
				IntPrioritySet(INT_ADC0SS0,configMAX_SYSCALL_INTERRUPT_PRIORITY);
				//IntEnable(INT_ADC0SS1);
				IntEnable(INT_ADC0SS0);//interrupcion de secuenciador 0

				//Creamos una cola de mensajes para la comunicacion entre la ISR y la tara que llame a configADC_LeeADC(...)
				cola_adc=xQueueCreate(8,sizeof(MuestrasADC));
				if (cola_adc==NULL)
				{
					while(1);
				}
}

void configADC_IniciaADC1(void)//para el apartado 3 de modo diferencial.Utilizamos el ADC1
{
                SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
                SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_ADC1);

                //HABILITAMOS EL GPIOE
                SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
                SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOE);
                // Enable pin PE3 for ADC AIN0|AIN1|AIN2|AIN3
                GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0);

                //HABILITAMOS EL GPIOD
                 SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
                 SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_GPIOD);
                // Enable pin PD3 for ADC AIN2|AIN3
                  GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_3|GPIO_PIN_2);



                  // 4 secuenciadores. 0->8 muestras; 1,2->4 muestras; 3-> 1 muestra
                //CONFIGURAR SECUENCIADOR 1(YA QUE TIENE 4 posiciones de la fifo)4muestras
                  //En el modo diferencial solo necesitamos 3
                ADCSequenceDisable(ADC1_BASE,1);

                //Configuramos la velocidad de conversion al maximo (1MS/s)
                //ADCClockConfigSet(ADC1_BASE, ADC_CLOCK_RATE_FULL, 1);//OJO, como solo hay un reloj para todos los ADCs y ya lo hemos establecido en el ADC0 no se vuelve a poner

                //ojo tengo que configurar con la secuencia 1
                ADCSequenceConfigure(ADC1_BASE,0,ADC_TRIGGER_PROCESSOR,0);  //Disparo software (processor trigger)
                ADCSequenceStepConfigure(ADC1_BASE,1,0,ADC_CTL_D|ADC_CTL_CH0);//ADC_CTL_D:diferential select. la info viene en el datasheet, en el apartadp de Differential Sampling
                ADCSequenceStepConfigure(ADC1_BASE,1,1,ADC_CTL_D|ADC_CTL_CH1);
                ADCSequenceStepConfigure(ADC1_BASE,1,2,ADC_CTL_D|ADC_CTL_CH2);
                ADCSequenceStepConfigure(ADC1_BASE,1,3,ADC_CTL_IE |ADC_CTL_END );    //La ultima muestra provoca la interrupcion, ojo este canal no lo utilizamos pero tenemos que ponerlo para rellenar las 8 posiciones
                ADCSequenceEnable(ADC1_BASE,1); //ACTIVO LA SECUENCIA

                //Habilita las interrupciones
                ADCIntClear(ADC1_BASE,1);//el 1 es el secuenciador
                ADCIntEnable(ADC1_BASE,1);
                //IntPrioritySet(INT_ADC0SS1,configMAX_SYSCALL_INTERRUPT_PRIORITY);
                IntPrioritySet(INT_ADC1SS1,configMAX_SYSCALL_INTERRUPT_PRIORITY);//INT_ADC1SS1, ADC1 secuenciador1
                //IntEnable(INT_ADC0SS1);
                IntEnable(INT_ADC1SS1);//interrupcion de secuenciador 1

                //Creamos una cola de mensajes para la comunicacion entre la ISR y la tara que llame a configADC_LeeADC(...)
                cola_adc1=xQueueCreate(8,sizeof(MuestrasADC1));
                if (cola_adc1==NULL)
                {
                    while(1);
                }
}
void configADC_LeeADC(MuestrasADC *datos)
{
	xQueueReceive(cola_adc,datos,portMAX_DELAY);
}

void configADC_LeeADC1(MuestrasADC1 *datos)
{
    xQueueReceive(cola_adc1,datos,portMAX_DELAY);
}

void configADC_ISR(void)//OJO:Hay que cambiar en la tabla de vectores a la posicion en la que este el secuenciador que estamos utilizando
{
	portBASE_TYPE higherPriorityTaskWoken=pdFALSE;

	MuestrasLeidasADC leidas;
	MuestrasADC finales;
	ADCIntClear(ADC0_BASE,0);//LIMPIAMOS EL FLAG DE INTERRUPCIONES(adc0, numero de secuenciador)
	ADCSequenceDataGet(ADC0_BASE,0,(uint32_t *)&leidas);//COGEMOS LOS DATOS GUARDADOS

	//if

	//Pasamos de 32 bits a 16 (el conversor es de 12 bits, así que sólo son significativos los bits del 0 al 11)
	uint8_t i;
	for(i=0;i<7;i++)
	{
	    finales.chan[i]=leidas.chan[i];
	}
	//finales.chan1=leidas.chan1;
	//finales.chan2=leidas.chan2;
	//finales.chan3=leidas.chan3;
	//finales.chan4=leidas.chan4;
	//finales.chan5=leidas.chan5;
	//finales.chan6=leidas.chan6;
    //finales.term=leidas.term;
    //finales.chan7=leidas.chan7;
	//Guardamos en la cola
	xQueueSendFromISR(cola_adc,&finales,&higherPriorityTaskWoken);
	portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

//para el apartado 3, interrupcion del ADC1
void configADC1_ISR(void)//OJO:Hay que cambiar en la tabla de vectores a la posicion en la que este el secuenciador que estamos utilizando
{
    portBASE_TYPE higherPriorityTaskWoken=pdFALSE;

    MuestrasLeidasADC1 leidas;
    MuestrasADC1 finales;
    ADCIntClear(ADC1_BASE,1);//LIMPIAMOS EL FLAG DE INTERRUPCIONES(adc1, numero de secuenciador)
    ADCSequenceDataGet(ADC1_BASE,1,(uint32_t *)&leidas);//COGEMOS LOS DATOS GUARDADOS

    //if

    //Pasamos de 32 bits a 16 (el conversor es de 12 bits, así que sólo son significativos los bits del 0 al 11)
    uint8_t i;
    for(i=0;i<3;i++)
    {
        finales.chanDiff[i]=leidas.chanDiff[i];
    }
    //Guardamos en la cola
    xQueueSendFromISR(cola_adc1,&finales,&higherPriorityTaskWoken);
    portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}

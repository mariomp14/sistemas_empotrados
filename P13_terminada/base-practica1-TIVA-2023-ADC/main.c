//*****************************************************************************
//
// Codigo de partida Practica 1.
// Autores: Eva Gonzalez, Ignacio Herrero, Jose Manuel Cano
//
//*****************************************************************************

#include<stdbool.h>
#include<stdint.h>
//
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
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
#include "semphr.h"
#include "utils/cpu_usage.h"

#include "drivers/rgb.h"
#include "drivers/configADC.h"
#include "commands.h"

#include <remotelink.h>
#include <serialprotocol.h>

#include"drivers/ACME_3416.h"//Ojo: tenemos que añadir la libreria del acme
#include "drivers/BMI160.h"//Hay que añadir libreria del BMI
#include "drivers/ACME_3416.h"//J
#include "emul/ACMESim_i2c_slave.h"

#include<timers.h> //o #include "timers.h"??
#include "event_groups.h"//necesario para utilizar flags de eventos

//parametros de funcionamiento de la tareas(en las tareas numero más alto =prioridad más baja??)
//mientras que en las interrupciones es al revés el mas bajo es el de más prioridad
#define REMOTELINK_TASK_STACK (512)
#define REMOTELINK_TASK_PRIORITY (tskIDLE_PRIORITY+2)
#define COMMAND_TASK_STACK (512)
#define COMMAND_TASK_PRIORITY (tskIDLE_PRIORITY+1)
#define ADC_TASK_STACK (256)
#define ADC_TASK_PRIORITY (tskIDLE_PRIORITY+1)

#define GPIO0a3_TASK_STACK (512)//*es correcto este tamaño de stack??
#define GPIO0a3_TASK_PRIORITY (tskIDLE_PRIORITY+1)//* esta prioridad es correcta?->Las prioridades de las tareas dan un poco igual
//*
#define LED2TASKSTACKSIZE 128

/*#define mascara_bits_gpio0 0x01
#define mascara_bits_gpio1 0x02
#define mascara_bits_gpio2 0x04
#define mascara_bits_gpio3 0x08*/

//*variables Globales para controlar el estado en el que estoy.Intentar luego hacer lo mismo con un semáforo??
//int ADC_MANUAL=0;
//int ADC_AUTO= 0;
static EventGroupHandle_t FlagsEventos;//flag de eventos

#define ADC_MANUAL 0x01  //(1<<0) // 0x0001
#define ADC_AUTO  0x02  //(1<<1)// 0x0002

//* manejadores de las tareas y colas
static TaskHandle_t manejador_tareaADC; // manejador de la tarea DEL ADC
static TaskHandle_t manejador_tareaGPIO0a3;
//xQueueHandle colaISR;//manejador de cola
static SemaphoreHandle_t misemaforo;//declaracion de Semaforo(manejador)

//Globales
uint32_t g_ui32CPUUsage;
uint32_t g_ulSystemClock;

static TimerHandle_t xTimerBMI;//*identificador del temporizador software

// Definición de la enumeración y variables globales. La uso dentro de la ISR
/*typedef enum
{
    BOTON_SUELTO,
    BOTON_PRESIONADO
} ButtonState;

ButtonState boton_PF4 = BOTON_SUELTO;
ButtonState boton_PF0 = BOTON_SUELTO;
*/
//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
// Esta funcion se llama si la biblioteca driverlib o FreeRTOS comprueban la existencia de un error (mediante
// las macros ASSERT(...) y configASSERT(...)
// Los parametros nombrefich y linea contienen informacion de en que punto se encuentra el error...
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *nombrefich, uint32_t linea)
{
    while(1) //Si la ejecucion esta aqui dentro, es que el RTOS o alguna de las bibliotecas de perifericos han comprobado que hay un error
    { //Mira el arbol de llamadas en el depurador y los valores de nombrefich y linea para encontrar posibles pistas.
    }
}
#endif

//*****************************************************************************
//
// Aqui incluimos los "ganchos" a los diferentes eventos del FreeRTOS
//
//*****************************************************************************

//Esto es lo que se ejecuta cuando el sistema detecta un desbordamiento de pila
//
void vApplicationStackOverflowHook(TaskHandle_t pxTask,  char *pcTaskName)
{
	//
	// This function can not return, so loop forever.  Interrupts are disabled
	// on entry to this function, so no processor interrupts will interrupt
	// this loop.
	//
	while(1)
	{
	}
}

//Esto se ejecuta cada Tick del sistema. LLeva la estadistica de uso de la CPU (tiempo que la CPU ha estado funcionando)
void vApplicationTickHook( void )
{
	static uint8_t count = 0;

	if (++count == 10)
	{
		g_ui32CPUUsage = CPUUsageTick();
		count = 0;
	}
	//return;
}

//Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationIdleHook (void)
{
	SysCtlSleep();
}


//Esto se ejecuta cada vez que entra a funcionar la tarea Idle
void vApplicationMallocFailedHook (void)
{
	while(1);
}



//*****************************************************************************
//
// A continuacion van las tareas...
//
//*****************************************************************************


//Para especificacion 2. Esta tarea no tendria por que ir en main.c
static portTASK_FUNCTION(ADCTask,pvParameters)
{

    MuestrasADC muestras;
    MuestrasADC1 muestras1;
    MESSAGE_ADC_SAMPLE_PARAMETER parameter;
    MESSAGE_ADC_SAMPLE_AUTO_PARAMETER parameter_auto;
    MESSAGE_ADC_SAMPLE_DIFF_PARAMETER parameter_diff;

    uint32_t cont_AUTO=0;//contador que cuenta las veces que se han tomado muestras en modo automatico. De 0 a 9, para mandar paquetes de 10 en 10
    uint32_t cont_AUTO_diff=0;

    EventBits_t eventosADC;//F
    //
    // Bucle infinito, las tareas en FreeRTOS no pueden "acabar", deben "matarse" con la funcion xTaskDelete().
    //
    while(1)
    {
       eventosADC = xEventGroupWaitBits(FlagsEventos,ADC_MANUAL|ADC_AUTO,pdFALSE,pdFALSE,portMAX_DELAY);
       //argumentos de la función:(manejador, máscara de bits,indicar si queremos borrar o no los  flags,bool,que se active cualquiera de los bits de la máscara o con ambos,espera_indefinida)

        if(eventosADC & ADC_MANUAL)// verifica si el bit ADC_MANUAL está establecido en el valor de retorno de xEventGroupWaitBits (and lógica)
        {
        configADC_LeeADC(&muestras);    //Espera y lee muestras del ADC (BLOQUEANTE)

        uint8_t j;
        //Copia los datos en el parametro (es un poco redundante)
        for(j=0;j<=6;j++)
          {
              parameter.channel[j]=muestras.chan[j];
          }


        //Encia el mensaje hacia QT
        remotelink_sendMessage(MESSAGE_ADC_SAMPLE,(void *)&parameter,sizeof(parameter));

       // ADC_MANUAL=0;//*restablezco a 0 la variable global
        }
        else if( eventosADC & ADC_AUTO )//
        {
            configADC_LeeADC(&muestras);    //Espera y lee muestras del ADC (BLOQUEANTE)
            configADC_LeeADC1(&muestras1); //cuando se activa el modo automatico, tambienn activamos el modo diferencial

         //Copia los datos en el parametro (es un poco redundante)
            uint8_t i;
         for(i=0;i<=6;i++)
         {
             parameter_auto.channel[i][cont_AUTO]=muestras.chan[i];
         }

          if(cont_AUTO==9)//quiere decir que ya ha llegado a 10 muestras
          {
              remotelink_sendMessage(MESSAGE_ADC_SAMPLE_AUTO,(void *)&parameter_auto,sizeof(parameter_auto));
              cont_AUTO=0;//reseteo la variable contadora a 0 para mandar paquetes de 10 en 10 muestras
          }

          else
          {
              cont_AUTO++;
          }

          //parte del modo diferencial
          for(i=0;i<3;i++)
          {
             parameter_diff.channeldiff[i][cont_AUTO_diff]=muestras1.chanDiff[i];
          }

          if(cont_AUTO_diff==19)//quiere decir que ya tenemos las 20 muestras
          {
              remotelink_sendMessage(MESSAGE_ADC_SAMPLE_DIFF,(void *)&parameter_diff,sizeof(parameter_diff));
              cont_AUTO_diff=0;//reseteo la variable contadora a 0 para mandar paquetes de 20 en 20 muestras
          }

          else
          {
              cont_AUTO_diff++;
          }
        }

        //en otro caso, al iniciar el programa ambas variables globales valdrán 0 y no se ejcutará ninguna de las condiciones
    }
}

static portTASK_FUNCTION(GPIO0a3Task,pvParameters)
   {
    vTaskDelay(600); //OJO:!!ya que al llevar las funciones del acme tiene que retrasarse un poco respecto a las
                      //a la tarea que incializa el ACME (si no no me entra en la interrupcion).
                       //Primero se tiene que ejecutar la tarea vCommandTask que se encuentra en commands.c y tiene un vTaskDelay(500);
    //habria otra forma más elegante de hacerlo que sería:Activar la interrupcion en la tarea que inicializa al acme, ya que la idea es que por le pin pa5
                                                     //salga la interrupcion y entre por el pin pa3, por lo que antes de estar inicializada la interrupcion
                                                     //debe estar inizializado el acme??

    MESSAGE_GPIO0_3_PARAMETER parametro;
   // Los pines GPIO0 a GPIO3 del ACME3416 emulado se configurarán como entradas
      //crear mensaje para gpio0,1,2 y3
      ACME_setPinDir(0xf0);//1111 0000, gpio4,5,6,7 puestos a1 es decir como salida. Los demás como entrada(0,1,2,3), gpio emulados de puertoB!
   // se activarán sus interrupciones
      ACME_setIntType(0xff,0x00);//activamos las interrupciones con ambos flancos de los gpioB0,1,2,3
//flanco de bajada (0x55)??

      //monitorizar_estado_leds en QT
      //PA3: pin para y atender las interrupciones generadas por el dispositivo emulado
      //PA5:señal de salida /INT del ACME es implementada por el pin PA5 del microcontrolador
      uint8_t bits;//*

     while(1)
      {
         if(xSemaphoreTake(misemaforo,portMAX_DELAY)==pdTRUE)//tomo el semaforo. Creo que al tener tiempo de espera infinito no haría
         {
         //al llegar aqui ya se que ha llegado alguna interrupcion del acme que sale por el pA5 y entra mediante pA3(conectar con cable)


             //cuando las entradas están sin conectar detecta un 0x0f 0000 1111 (ya que están al aire), es lo que se almacena en bits
             //También detecta 1 cuando están conectadas a 3.3V
             //cuando se conecta alguna a tierra detecta 0 en vez de 1

            if(ACME_readPin(&bits)!=0)
            {
                UARTprintf("Error en ACME_readPin");//ya que ACME_readPin devuelve 0 si está todo correcto
            }
          /*  parametro.gpio0=0;
            parametro.gpio1=0;
            parametro.gpio2=0;
            parametro.gpio3=0;

             if((mascara_bits_gpio0)&(bits))//si esto es true (algun bit vale 1), entra en el if(operacion lógica AND)
                                             //->Hago una and de lo que me devuelve la lectura, que se almacena en el argumento bits,
                                          //con la máscara de bits que quiero comprobar. & es el operador AND bit a bit, no confundir con && que es para que ambas
               {                                          //condiciones se cumplan
                parametro.gpio0=1;
               }
             if((mascara_bits_gpio1)&(bits))//si esto es true (vale 1), entra en el if(operacion lógica AND)
               {

                 parametro.gpio1=1;
               }

             if((mascara_bits_gpio2)&(bits))
               {
                 parametro.gpio2=1;
               }

             if((mascara_bits_gpio3)&(bits))
               {
                 parametro.gpio3=1;
               }
           */

            //Otra forma más eficiente sería dado que en la variable bits tenemos una máscara de bits(de cada uno de los gpio emulados)
            //, mandar como mensaje un único byte
            parametro.gpio0a3=bits;//en recepcion sacamos cada bit de la máscara con desplazamientos


        //mandamos el mensaje a a la interfaz de usuario
         remotelink_sendMessage(MESSAGE_GPIO0_3,&parametro,sizeof(parametro));

         //Borramos los flags de interrupcion del dispositivo acme emulado
         ACME_clearInt(0xff);//borro todos los flags de interrupcion de todos los gpio del emulado


         }
      }
   }

/*  CALLBACK del timer software para el BMI */////////OJO:
void vTimerCallback(TimerHandle_t pxTimer)
{
   MESSAGE_ACELERACION_y_GIRO_PARAMETER parametro;
    BMI160_getAcceleration(&parametro.aceleracion[0], &parametro.aceleracion[1], &parametro.aceleracion[2]);
    BMI160_getRotation(&parametro.giro[0], &parametro.giro[1], &parametro.giro[2]);

   // remotelink_sendMessage(MESSAGE_BMI_SAMPLE, (void *)&medidas_BMI, sizeof(medidas_BMI));
    remotelink_sendMessage(MESSAGE_ACELERACION_y_GIRO,&parametro,sizeof(parametro));
}

//Funcion callback que procesa los mensajes recibidos desde el PC (ejecuta las acciones correspondientes a las ordenes recibidas)
static int32_t messageReceived(uint8_t message_type, void *parameters, int32_t parameterSize)
{
    int32_t status=0;   //Estado de la ejecucion (positivo, sin errores, negativo si error)

    //Comprueba el tipo de mensaje
    switch (message_type)//
    {
        case MESSAGE_PING:
        {
            status=remotelink_sendMessage(MESSAGE_PING,NULL,0);
        }
        break;
        case MESSAGE_LED_GPIO:
        {
                MESSAGE_LED_GPIO_PARAMETER parametro;

                if (check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0)//extrae el parametro de la trama y comprueba que el tamaño sea correcto
                {
                    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3,parametro.value);
                }
                else
                {
                    status=PROT_ERROR_INCORRECT_PARAM_SIZE; //Devuelve un error
                }
        }
        break;
        case MESSAGE_LED_PWM_BRIGHTNESS:
        {
            MESSAGE_LED_PWM_BRIGHTNESS_PARAMETER parametro;

            if (check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0)
            {
                RGBIntensitySet(parametro.rIntensity);
           }
           else
            {
               status=PROT_ERROR_INCORRECT_PARAM_SIZE; //Devuelve un error
            }
        }
        break;
        case MESSAGE_ADC_SAMPLE:
        {
            //activar ADC MANUAL Y desactivar automatico
            //config_ADC_seleccionarmodo(ADC_MANUAL,0.0);//le paso 0.0 ya que para este modo no necesitamos ninguna frecuencia y ese dato no se utiliza
           // ADC_MANUAL=1;
           // ADC_AUTO=0;
            xEventGroupSetBits(FlagsEventos, ADC_MANUAL);//F
            configADC_DisparaADC(); //Dispara la conversion (por software)


        }
        break;

        case MESSAGE_MODE_GPIO_PWM:
        {
            MESSAGE_MODE_GPIO_PWM_PARAMETER parametro;

            if (check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0)
            {
                if(parametro.pwm==true)
                {
                //pongo modo pwm
                    RGBEnable();//
                    UARTprintf("cambio a modo PWM\r\n");
                }
                else if(parametro.gpio==true)
                {
                    RGBDisable();
                    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);//establecemos como gpio los puertos correspondientes a los leds
                    UARTprintf("cambio a modo GPIO\r\n");
                }
            }
            else
             {
                 status=PROT_ERROR_INCORRECT_PARAM_SIZE; //Devuelve un error
              }
        }
        break;
        case MESSAGE_COLOR_PWM:
        {
            uint32_t array_multiplicado[3];
            MESSAGE_COLOR_PWM_PARAMETER parametro;
            if (check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0)
            {
                 array_multiplicado[0]=parametro.arrayRGB[0]*256;//para que vaya hasta el máximo  65536 2 elevado a 16
                 array_multiplicado[1]=parametro.arrayRGB[1]*256;
                 array_multiplicado[2]=parametro.arrayRGB[2]*256;

                 RGBColorSet(array_multiplicado);
            }
            else
            {
                status=PROT_ERROR_INCORRECT_PARAM_SIZE; //Devuelve un error
            }
        }

        break;

        case MESSAGE_CONSULTAR_PINES:
        {
            MESSAGE_CONSULTAR_PINES_PARAMETER parametro;
            uint8_t ui8Buttons, ui8Changed;
            ButtonsInit();//Inicializo los botones

            if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
            {
            //Leemos por sondeo
                ButtonsPoll(&ui8Changed,&ui8Buttons);
                if(RIGHT_BUTTON & ui8Buttons)
                {

                    parametro.PF0=1;

                }
                else
                {
                    parametro.PF0=0;
                }
                if(LEFT_BUTTON & ui8Buttons)
                 {
                    parametro.PF4=1;
                 }
                else
                {
                    parametro.PF4=0;
                }
                //mandamos el mensaje de respuesta a la interfaz de usuario
                status=remotelink_sendMessage(MESSAGE_CONSULTAR_PINES,&parametro,sizeof(parametro));
            }

            else
            {
                //mensaje desconocido/no implementado
                          status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
            }
        }
        break;

        case MESSAGE_CONTROL_ASINCRONO:

        {
            MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro;
            if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
             {
                if(parametro.automatico==true)
                    {
                    //libero el mutex para que cuando haya una interrupcion y se ejecute la funcion que
                     //devuelve el estado de los botones sin solicitarlo previamente por el interfaz de usuario pueda hacerlo
                    //xSemaphoreGive(misemaforo);

                    GPIOIntEnable(GPIO_PORTF_BASE,ALL_BUTTONS);//habilitamos las interrupciones
                    }
                else
                    {
                    //xSemaphoreTake(misemaforo,0);//tomo el semaforo con un tiempo de espera de 0 ticks(me llevo la llave)
                    GPIOIntDisable(GPIO_PORTF_BASE,ALL_BUTTONS);
                    }
             }
            else
             {
                    //mensaje desconocido/no implementado
                    status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
             }
        }
        break;

       default:
           //mensaje desconocido/no implementado
         status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.


    case MESSAGE_CONFIGURAR_SOBREMUESTREO:
    {

        MESSAGE_CONFIGURAR_SOBREMUESTREO_PARAMETER parametro;
        if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
          {
            aplicar_promedio(parametro.longitud);
          }

        else
          {
            //mensaje desconocido/no implementado
             status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
          }

    }break;

    case MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO://configuro muestreo automatico y frecuencia de muestreo
    {//si cliko habilito el muestro automatico, si desclicko lo deshabilito
        MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO_PARAMETER parametro;
        double Period;
         if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
            {
             if(parametro.automatico==true)//en mi interfaz gráfica no hay posibilidad de pulsar el modo manual si está activado el automático
             {
              // Periodo de cuenta de 0.05s. SysCtlClockGet() te proporciona la frecuencia del reloj del sistema, por lo que una cuenta
                          // del Timer a SysCtlClockGet() tardara 1 segundo, a 0.5*SysCtlClockGet(), 0.5seg, etc...
               Period = SysCtlClockGet() / (parametro.frecuencia);//calcula el periodo necesario para la frequencia que queremos,(respecto a la frecuencia general de nuestro sistema)
               TimerLoadSet(TIMER2_BASE, TIMER_A, Period - 1);
               TimerEnable(TIMER2_BASE, TIMER_A);// Carga la cuenta en el Timer2A
               ADCSequenceConfigure(ADC0_BASE,0,ADC_TRIGGER_TIMER,0);//configura el ADC0 con disparo mediante trigger timer.En este caso por TIMER HW
               TimerControlTrigger(TIMER2_BASE, TIMER_A, true);//controla si se habilita o no la generación de un disparador (trigger) en el temporizador TIMER2(selecciona el timer
                                                                                            //con el que se va a ir disparando el trigger)
                                                                                          //lo necesitamos ya que queremos realizarlo por HW y no por interrupciones software
               //ADC_AUTO=1;
               xEventGroupClearBits(FlagsEventos,ADC_MANUAL);//F.
               xEventGroupSetBits(FlagsEventos, ADC_AUTO);//F
               ADCSequenceConfigure(ADC1_BASE,1,ADC_TRIGGER_TIMER,0);//*apartado4
             }
             else
             {
                 //para desechar muestras cuando cambio de modo autoamtico a manual. también habrá que vaciar la cola??creo que no
                 vTaskDelete(manejador_tareaADC);
                 //vuelvo a crear la tarea para poder seguir utilizandola posteriormente
                 if((xTaskCreate(ADCTask, (portCHAR *)"ADCTask", ADC_TASK_STACK,NULL,ADC_TASK_PRIORITY, &manejador_tareaADC) != pdTRUE))
                    {
                        while(1);
                    }

                  /////
               TimerControlTrigger(TIMER2_BASE, TIMER_A, false);//deshabilita la generacion del disparador(trigger)
               TimerDisable(TIMER2_BASE, TIMER_A);//Deshabilitamos el timer
               ADCSequenceConfigure(ADC0_BASE,0,ADC_TRIGGER_PROCESSOR,0);//configura los criterios de iniciación para una secuencia de muestras.En este caso disparo software
               //ADC_AUTO=0;
               xEventGroupClearBits(FlagsEventos,ADC_AUTO);//F
               ADCSequenceConfigure(ADC1_BASE,1,ADC_TRIGGER_PROCESSOR,0);//*apartado4

               //si se deshabilita hay que vaciar la cola (desechar los mensajes pendientes antes del momento del envío):

             }
            }
         else
         {
           //mensaje desconocido/no implementado
            status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
         }
    }
    break;
    case MESSAGE_GPIO4_7://ACME
     {
         MESSAGE_GPIO4_7_PARAMETER parametro;
         if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
         {
             //OJO, para que esto funcione dichos pines deben estar previamente configurados como salida
             ACME_setPinDir(0xf0);//1111 0000, gpio4,5,6,7 puestos a1 es decir como salida. Los demás como entrada

             /* Lo que se le pasa a la función no es el número del pin, sino una máscara de bits, si pones 3, =0000 0011
            estarías poniendo el GPIO0 y GPIO1 y el resto a 0. */
            ACME_writePin(parametro.value);//
             //

         }
     }
        break;

    case MESSAGE_MUESTREO_y_FRECUENCIA://BMI, hago otro mensaje solo para la frecuencia ??, creo que no hace falta
    {
        MESSAGE_MUESTREO_y_FRECUENCIA_PARAMETER parametro;
        if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
           {
            if(parametro.activar_muestreo==pdTRUE)
            {
                //activo TIMER SOFTWARE!! y frecuencia de muestreo del acelerometro
                //xTimerStart( xTimer, 0 )  //no hace falta ya que al cambiar el periodo ya se pone el timer en marcha
               xTimerChangePeriod(xTimerBMI,configTICK_RATE_HZ/parametro.frecuencia,0);//(numero de tics/segundo/frecuencia que le pasamos=periodo)

            }

            else
            {
              //desactivo muestreo del acelerometro, desactivo el timer correspondiente
                xTimerStop(xTimerBMI,0);
          }

           }

        else
           {
              //mensaje desconocido/no implementado
              status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
           }

    }break;
    case MESSAGE_ACC_CAMBIAR_RANGO:
    {
        MESSAGE_ACC_CAMBIAR_RANGO_PARAMETER parametro;
        if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
             {
                if(parametro.rango_acc==2)
                {
                    BMI160_setFullScaleAccelRange(BMI160_ACCEL_RANGE_2G);
                }
                else if(parametro.rango_acc==4)
                {
                    BMI160_setFullScaleAccelRange(BMI160_ACCEL_RANGE_4G);
                }
                else if(parametro.rango_acc==8)
                {
                    BMI160_setFullScaleAccelRange(BMI160_ACCEL_RANGE_8G);
                }
                else if(parametro.rango_acc==16)
                {
                    BMI160_setFullScaleAccelRange(BMI160_ACCEL_RANGE_16G);
                }
                else
                {
                    //mensaje desconocido/no implementado
                     status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
                }
             }
        else
            {
            //mensaje desconocido/no implementado
                          status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
            }

    }break;
    case MESSAGE_GIRO_CAMBIAR_RANGO:
    {
        MESSAGE_GIRO_CAMBIAR_RANGO_PARAMETER parametro;

        if ((check_and_extract_command_param(parameters, parameterSize, &parametro, sizeof(parametro))>0))
        {
           if(parametro.rango_giro==125)
           {
               BMI160_setFullScaleGyroRange(BMI160_GYRO_RANGE_125);
           }
           else if(parametro.rango_giro==250)
           {
               BMI160_setFullScaleGyroRange(BMI160_GYRO_RANGE_250);
           }
           else if(parametro.rango_giro==500)
           {
               BMI160_setFullScaleGyroRange(BMI160_GYRO_RANGE_500);
           }
           else if(parametro.rango_giro==1000)
           {
               BMI160_setFullScaleGyroRange(BMI160_GYRO_RANGE_1000);
           }
           else if(parametro.rango_giro==2000)
           {
               BMI160_setFullScaleGyroRange(BMI160_GYRO_RANGE_2000);
           }
           else
           {
               //mensaje desconocido/no implementado
               status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
           }
        }

        else
             {
               //mensaje desconocido/no implementado
               status=PROT_ERROR_UNIMPLEMENTED_COMMAND; //Devuelve error.
             }
    }break;
    return status;   //Devuelve status
  }
}


//*****************************************************************************
//
// Funcion main(), Inicializa los perifericos, crea las tareas, etc... y arranca el bucle del sistema
//
//*****************************************************************************

/*portTASK_FUNCTION(BOTONTask,pvParameters)
{
    MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro;

   while(1)
   {

           //if((xQueueReceive(colaISR,&parametro,portMAX_DELAY)==pdTRUE)&&(xSemaphoreTake(misemaforo,0)==pdTRUE))
       if(xQueueReceive(colaISR,&parametro,portMAX_DELAY)==pdTRUE)
           {
               remotelink_sendMessage(MESSAGE_CONTROL_ASINCRONO,&parametro,sizeof(parametro));
           }
           //xSemaphoreGive(misemaforo);//libero el semáforo, para que si suelto el botón de la placa pueda volver a entrar en la condición
                                          //sin la necesidad de volver a clickar y desclickar el boton del interfaz de usuario en QT
   }
}*/


int main(void)
{

	//
	// Set the clocking to run at 40 MHz from the PLL.
	//
	MAP_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
			SYSCTL_OSC_MAIN);	//Ponermos el reloj principal a 40 MHz (200 Mhz del Pll dividido por 5)


	// Get the system clock speed.
	g_ulSystemClock = SysCtlClockGet();


	//Habilita el clock gating de los perifericos durante el bajo consumo --> perifericos que se desee activos en modo Sleep
	//                                                                        deben habilitarse con SysCtlPeripheralSleepEnable
	MAP_SysCtlPeripheralClockGating(true);

	// Inicializa el subsistema de medida del uso de CPU (mide el tiempo que la CPU no esta dormida)
	// Para eso utiliza un timer, que aqui hemos puesto que sea el TIMER3 (ultimo parametro que se pasa a la funcion)
	// (y por tanto este no se deberia utilizar para otra cosa).
	CPUUsageInit(g_ulSystemClock, configTICK_RATE_HZ/10, 3);

	//Inicializa los LEDs usando libreria RGB --> usa Timers 0 y 1
	RGBInit(1);
	MAP_SysCtlPeripheralSleepEnable(GREEN_TIMER_PERIPH);
	MAP_SysCtlPeripheralSleepEnable(BLUE_TIMER_PERIPH);
	MAP_SysCtlPeripheralSleepEnable(RED_TIMER_PERIPH);	//Redundante porque BLUE_TIMER_PERIPH y GREEN_TIMER_PERIPH son el mismo

	//Volvemos a configurar los LEDs en modo GPIO POR Defecto
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);


	//Inicializa el puerto F (LEDs) como GPIO
	 SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //Inicializa los botones (tambien en el puerto F) y habilita sus interrupciones
	ButtonsInit();//EGP
	GPIOIntTypeSet(GPIO_PORTF_BASE, ALL_BUTTONS,GPIO_FALLING_EDGE);//EGP
	IntPrioritySet(INT_GPIOF,configMAX_SYSCALL_INTERRUPT_PRIORITY); //EGP
	//GPIOIntEnable(GPIO_PORTF_BASE,ALL_BUTTONS);//Esto solo lo habilito cuando pongo el modo asincrono con interrupciones.
	                                             //Ya que si lo habilito desde el principio el sondeo puede que no funcione
	                                             //ya que las interrupciones tienen prioridad
	// IntRegister(INT_GPIOF,GPIOFIntHandler);//Esto lo hago de forma manual, añadiendolo en la tabla de vectores
	IntEnable(INT_GPIOF);//EGP

	//Habilitamos puertoA, pin3 e interrupcion para GPIOIntHandler_PA3
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
        GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_3);
        GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_3,GPIO_FALLING_EDGE);//EGP, OJO: configuramos la interrupcion con flanco de bajada
                                                                      //ya que cuando se active alguna de las interrupciones de los gpio emulados
                                                                      //se activará la señal INT que es activa a nivel bajo(saliendo por PA5 y llegando
                                                                      //a PA3, conectandola manualmente con un cable)
        IntPrioritySet(INT_GPIOA,configMAX_SYSCALL_INTERRUPT_PRIORITY); //EGP->Cambiamos prioridad a la interrupcion ya que utiliza mecanismos de freeRTO
        GPIOIntClear(GPIO_PORTA_BASE,GPIO_PIN_3);
        GPIOIntEnable(GPIO_PORTA_BASE,GPIO_PIN_3);
        IntEnable(INT_GPIOA);//EGP

    // IntMasterEnable();//*Esto ya lo hace el freeRTO al arrancar el scheduller


	// timer que necesito para muestrear el ADC automaticamente
	// Configuracion TIMER2
	// Habilita periferico Timer2
	   SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);// TIMER1,0 -> RGB, CPUUsage ->??3
	 // Configura el Timer2 para cuenta periodica de 32 bits (no lo separa en TIMER0A y TIMER0B)
	    TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
	    //falta el sleepenable
	    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_TIMER2);//importante para que pueda ejecutarse aún cuando el periferico está en bajo consumo

	/********************************      Creacion de tareas *********************/

	//Tarea del interprete de comandos (commands.c)
    if (initCommandLine(COMMAND_TASK_STACK,COMMAND_TASK_PRIORITY) != pdTRUE)
    {
        while(1);
    }
//NUEVO RESPECTO OTROS PROYECTOS
	//Esta funcion crea internamente una tarea para las comunicaciones USB.
	//Ademas, inicializa el USB y configura el perfil USB-CDC
	if (remotelink_init(REMOTELINK_TASK_STACK,REMOTELINK_TASK_PRIORITY,messageReceived)!=pdTRUE)
	{
	    while(1); //Inicializo la aplicacion de comunicacion con el PC (Remote). Ver fichero remotelink.c
	}


	//Para especificacion 2: Inicializa el ADC y crea una tarea...
	configADC_IniciaADC();
	configADC_IniciaADC1();//iniciamos el ADC1, para el apartado4(modo diferencial)


    if((xTaskCreate(ADCTask, (portCHAR *)"ADC", ADC_TASK_STACK,NULL,ADC_TASK_PRIORITY, &manejador_tareaADC) != pdTRUE))
    {
        while(1);
    }

    //creo una tarea para el apartado 4.4 de la práctoca 1.3
    if((xTaskCreate(GPIO0a3Task, (portCHAR *)"GPIO0a3", GPIO0a3_TASK_STACK,NULL,GPIO0a3_TASK_PRIORITY, &manejador_tareaGPIO0a3) != pdTRUE))
      {
          while(1);
      }

   //creo el timer software para muestrear el acelerometro
    if( (xTimerBMI = xTimerCreate("xTimerBM",configTICK_RATE_HZ, pdTRUE,NULL,vTimerCallback))==NULL )//vTimerCallback es la funcion que se llamará cuando expire el timer
      {
               while(1);
      }

    FlagsEventos = xEventGroupCreate();
     if( FlagsEventos == NULL )
     {
     while(1);
     }

	//TAREA DEL CONTROL ASINCRONO al pulsar LOS BOTONES

  /*  if((xTaskCreate(BOTONTask, (signed  portCHAR *)"boton", LED2TASKSTACKSIZE,NULL,tskIDLE_PRIORITY + 2, &Handle_boton) != pdTRUE))
     //          nombre_tarea,puntero a la tarea,tamaño de pila d la tarea,puntero que se le pasa como argumento a la tarea en este caso(NULL),prioridad de la tarea,
       //           puntero que se utiliza para almacenar el identificador de la tarea creada
      {
            while(1)//si hya algún error al crease la tarea se queda en bucle infinito
                         {
                         }
      }*/

    //SEMAFORO QUE USO PARA ACTIVAR O DESACTIVAR la tarea GPIO0a3Task
        misemaforo=xSemaphoreCreateBinary();//creo semaforo binario, para esta funcionalidad creo que no me hace falta la herencia de prioridad
                                            //en este caso puede ser binario o contador, mutex no ya que eso es para exclusión mutua entre tareas->REPASAR!!!
       // xSemaphoreTake(misemaforo,portMAX_DELAY);//tomo el semaforo con un tiempo de espera de INFINITOS ticks(me llevo la llave)
        xSemaphoreTake(misemaforo,0);

	// Arranca el  scheduler.  Pasamos a ejecutar las tareas que se hayan activado.
	//
   /*  MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro;
     colaISR=xQueueCreate(10,sizeof(parametro));
         if(NULL==colaISR)
             while(1);//Si hay problemas al crear la cola se queda aquí*/

	vTaskStartScheduler();	//el RTOS habilita las interrupciones al entrar aqui, asi que no hace falta habilitarlas
	//De la funcion vTaskStartScheduler no se sale nunca... a partir de aqui pasan a ejecutarse las tareas.

	while(1)
	{
		//Si llego aqui es que algo raro ha pasado
	}
}

void funcion_botones_asincronos(MESSAGE_CONTROL_ASINCRONO_PARAMETER parameter, uint32_t ulParameter2 )//le llega ui32PinStatus por el segundo parametro
{
   // MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro = parameter;
    //MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro=parameter;
     parameter.PF4=!(ulParameter2 & LEFT_BUTTON); //lo negamos ya que es activo a nivel bajo
     parameter.PF0=!(ulParameter2 & RIGHT_BUTTON);
    remotelink_sendMessage(MESSAGE_CONTROL_ASINCRONO,/*(void*)*/&parameter,sizeof(parameter));
}


void GPIOFIntHandler(void)   //*en este caso en la posicion del puerto F que es que estoy usando
{

   MESSAGE_CONTROL_ASINCRONO_PARAMETER parametro;
   BaseType_t higherPriorityTaskWoken=pdFALSE;
   int32_t i32PinStatus=GPIOPinRead(GPIO_PORTF_BASE,ALL_BUTTONS);//leemos el estado de los botones

        //  parametro.PF4=!(i32PinStatus & LEFT_BUTTON); //lo negamos ya que es activo a nivel bajo
        // parametro.PF0=!(i32PinStatus & RIGHT_BUTTON);

          //xQueueSendFromISR(colaISR, &parametro, &higherPriorityTaskWoken);//enviamos por la cola para que se ejecute la tarea correspondiente

        // xTimerPendFunctionCallFromISR(funcion_botones_asincronos,parametro,0,&higherPriorityTaskWoken);//es una función diferida ojo!
//ME HA SALIDO ERROR AL AÑADIR TIMERS.H??
   xTimerPendFunctionCallFromISR(funcion_botones_asincronos,0,i32PinStatus,&higherPriorityTaskWoken);

    GPIOIntClear(GPIO_PORTF_BASE,ALL_BUTTONS);//Limpia las interrupciones de los botones

    //Comprobamos si durante la interrupcion hay que hacer un cambio de contexto y por tanto
    //una cesión de la CPU si se ha despertado una tarea de mayor prioridad

     portEND_SWITCHING_ISR(higherPriorityTaskWoken);
}


//LAS RUTINAS de isr tienen por defecto todasd prioridad 0, cuando las rutinas
//tienen mecanismos de freeRTO hay que cambiarle la prioridad(viene explicado en la guia)->CAMBIADO EN EL MAIN
void GPIOIntHandler_PA3(void)//OJO, las funciones del acme no se pueden usar dentro de una ISR
{

    BaseType_t higherPriorityTaskWoken=pdFALSE;

    //Limpio la interrupción
            GPIOIntClear(GPIO_PORTA_BASE,GPIO_PIN_3);
  //Activo la tarea GPIO0a3Task(mediante un semáforo)
    xSemaphoreGiveFromISR(misemaforo,&higherPriorityTaskWoken);

        //Comprobamos si durante la interrupcion hay que hacer un cambio de contexto y por tanto
            //una cesión de la CPU si se ha despertado una tarea de mayor prioridad

        portEND_SWITCHING_ISR(higherPriorityTaskWoken);

}


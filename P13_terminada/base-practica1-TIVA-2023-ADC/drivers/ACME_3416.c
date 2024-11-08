/*
 * ACME_3416.c
 *
 *  Created on: 12 jun. 2019
 *      Author: Embedded Systems UMA
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "drivers/i2cm_drv.h"

#include "FreeRTOS.h"
#include "task.h"

#define ACME_ID_REGISTER 0
#define ACME_DIR_REGISTER 1
#define ACME_OUTPUT_REGISTER 2
#define ACME_INT_TYPE_REGISTER_A 3
#define ACME_INT_TYPE_REGISTER_B 4
#define ACME_INT_CLEAR_REGISTER 5
#define ACME_INPUT_REGISTER 6
#define ACME_INT_STATUS 7//*alñadido por mi

#define ACME_ID_REGISTER_VALUE 0xB6//??que es?




typedef struct
{
    //
    // The pointer to the I2C master interface instance used to communicate
    // with the SHT21.
    //
    tI2CMInstance *psI2CInst;

    //
    // the I2C address of the SHT21.
    //
    uint8_t ui8Addr;
} ACME_drv;

static ACME_drv ACME_data;

//static volatile int semaforo_malo=0;

//Función callback que se ejecuta desde la rutina de interrupcion I2C cuando una transacción finaliza
void ACMEAppCallback(void *pvCallbackData, uint_fast8_t ui8Status)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if(ui8Status != I2CM_STATUS_SUCCESS ) //*añado control de errores???
    {

    }

    //semaforo_malo=1;//Utilizando notificaciones a tareas no tenemos que estar sondeando constantemente una variable global
    /* Notifica a la tarea que la transmisión se completó configurando ui8Status
      en el valor de notificación de la tarea. */
    xTaskNotifyFromISR(pvCallbackData,ui8Status,eSetValueWithoutOverwrite,&xHigherPriorityTaskWoken);//*ya que se ejecuta desde la ISR
    //Parámetros:(puntero que se le pasa a la tarea notificada,campo de datos(que recibe si la operacion ha sido exitosa),
                 // (parámetro que indica que no se sobreescrira una notificacion pendiente)
}

int32_t ACME_WriteAndRead(uint8_t *ptrtotxdata,uint8_t txcount, uint8_t *ptrtorxdata,uint8_t rxcount)
{
   // semaforo_malo=0;
    uint32_t ulNotifiedValue;//*

    I2CMCommand(ACME_data.psI2CInst,ACME_data.ui8Addr, ptrtotxdata , txcount, txcount, ptrtorxdata,rxcount ,
                rxcount, ACMEAppCallback, xTaskGetCurrentTaskHandle());
    //como último  parámetro de esta hay que pasarle el manejador de la tarea en la que se ha
    //llamado a la función  a la ACME_WriteAnd Read() con una función xtaskGetCurrent TAskHandle

    //while(!semaforo_malo) {vTaskDelay(10);} //XXX CAMBIAR EP33!!!!

    /* Esperar a ser notificado de una interrupción de que la transacción se ha completado. */
    xTaskNotifyWait( pdFALSE,     /* No borra los bits al entrar. */
                               pdFALSE,         /* NO BORRA todos los bits al salir. */
                               &ulNotifiedValue, /* Almacena el valor notificado. */
                               portMAX_DELAY);//*Tiempo de espera infintito)

   // return 0; //XXX CAMBIAR EP33!!!!
    return ulNotifiedValue;
}

int32_t ACME_Write(uint8_t *ptrtotxdata,uint8_t txcount)
{
    //semaforo_malo=0;
    uint32_t ulNotifiedValue;//*

    I2CMCommand(ACME_data.psI2CInst,ACME_data.ui8Addr, ptrtotxdata , txcount, txcount, NULL,0 ,
                    0, ACMEAppCallback, xTaskGetCurrentTaskHandle());
    /* Esperar a ser notificado de una interrupción de que la transacción se ha completado. */
     xTaskNotifyWait( pdFALSE,     /* No borra los bits al entrar. */
                                pdFALSE,         /* NO BORRA todos los bits al salir. */
                                &ulNotifiedValue, /* Almacena el valor notificado. */
                                portMAX_DELAY);
    //while(!semaforo_malo) {vTaskDelay(10);} //XXX CAMBIAR EP33!!!!

     //   return 0; //XXX CAMBIAR EP33!!!!
     return ulNotifiedValue;
}

int32_t ACME_Read(uint8_t *ptrtorxdata,uint8_t rxcount)
{
    //semaforo_malo=0;
    uint32_t ulNotifiedValue;//*

    I2CMCommand(ACME_data.psI2CInst,ACME_data.ui8Addr, NULL , 0, 0, ptrtorxdata,rxcount ,
                    rxcount, ACMEAppCallback, xTaskGetCurrentTaskHandle());
                    //como último  parámetro de esta hay que pasarle el manejador de la tarea en la que se ha llamado a
                    //la función  a la ACME_WriteAnd Read() con una función xtaskGetCurrent TAskHandle()

    /* Esperar a ser notificado de una interrupción de que la transacción se ha completado. */
       xTaskNotifyWait( pdFALSE,     /* No borra los bits al entrar. */
                                  pdFALSE,         /* NO BORRA todos los bits al salir. */
                                  &ulNotifiedValue, /* Almacena el valor notificado. */
                                  portMAX_DELAY);
      //while(!semaforo_malo) {vTaskDelay(10);} //XXX CAMBIAR EP33!!!!

       //   return 0; //XXX CAMBIAR EP33!!!!
       return ulNotifiedValue;
}


//INICIALIZA EL DISPOSITIVO
//Esta funcion debe comprobar si el sensor esta o no disponible y habilitar las salidas que indiquemos
int32_t ACME_initDevice(tI2CMInstance *instance,uint8_t address)
{

    ACME_data.psI2CInst=instance;
    ACME_data.ui8Addr=address;

    uint8_t array_envio[2];
    uint8_t rxvalue;
    int32_t status;

    array_envio[0]=ACME_ID_REGISTER;
    status=ACME_WriteAndRead(array_envio,1,&rxvalue,1);
    if ((status!=0)||(rxvalue!=ACME_ID_REGISTER_VALUE))
            return -1;  // XXX Esta operación va a fallar hasta que se arregle la biblioteca i2c_driver.c

    array_envio[0]=ACME_DIR_REGISTER;
    array_envio[1]=0;
    status=ACME_Write(array_envio,sizeof(array_envio));
    if (status!=0)
       return -1;

    array_envio[0]=ACME_INT_TYPE_REGISTER_A;
    array_envio[1]=0;
    status=ACME_Write(array_envio,sizeof(array_envio));
    if (status!=0)
            return -1;

    array_envio[0]=ACME_INT_TYPE_REGISTER_B;
    array_envio[1]=0;
    status=ACME_Write(array_envio,sizeof(array_envio));
    if (status!=0)
            return -1;

    array_envio[0]=ACME_INT_CLEAR_REGISTER;
    array_envio[1]=0x3F;
    status=ACME_Write(array_envio,sizeof(array_envio));
    if (status!=0)
            return -1;


    return status;
}

//Establece la direccion de los pines
int32_t ACME_setPinDir (uint8_t dir)
{
    uint8_t array_envio[2];
    int32_t status;

    array_envio[0]=ACME_DIR_REGISTER;
    array_envio[1]=dir;
    status=ACME_Write(array_envio,sizeof(array_envio));
    return status;
}

//Escribe en los pines de salida
int32_t ACME_writePin (uint8_t pin)
{
    uint8_t array_envio[2];
    int32_t status;//

    array_envio[0]=ACME_OUTPUT_REGISTER;
    array_envio[1]=pin;
    status=ACME_Write(array_envio,sizeof(array_envio));
    return status;
}

//Lee los pines de entrada
int32_t ACME_readPin (uint8_t *pin)
{
    uint8_t dir_register=ACME_INPUT_REGISTER;
    int32_t status;

    status=ACME_WriteAndRead(&dir_register,sizeof(dir_register),pin,sizeof(uint8_t));
    return status;
}

//Borra las interrupciones
int32_t ACME_clearInt (uint8_t pin)
{
    uint8_t array_envio[2];
    int32_t status;

    array_envio[0]=ACME_INT_CLEAR_REGISTER;
    array_envio[1]=pin;
    status=ACME_Write(array_envio,sizeof(array_envio));
    return status;
}
/*Implementa la función ACME_setIntType del driver del ACME 3416, de forma que permita la escritura de
los registros INT_TYPE_A e INT_TYPE_B del dispositivo ACME en una única transacción I2C de escritura.*/
//habilita/deshabilita las intertupciones y el tipo
int32_t ACME_setIntType (uint8_t regA,uint8_t regB)
{
    //XXX PENDIENTE DE IMPLEMENTAR
    uint8_t buffer[3];//*
    //int32_t status=-1;//Cambiar
    int32_t status;//*

  /*OJO:  i2c le tienes que pasas la direccion donde escribes los datos, luego el dato que escribes y
    luego si escribes algun dato consecutivo se escribe en el registro consecutivo*/
    buffer[0] = ACME_INT_TYPE_REGISTER_A; // Dirección del registro INT_TYPE_A//*
    buffer[1] = regA; // Valor para el registro INT_TYPE_A//*¿Que valores hay que escirbirle??
    //buffer[2] = ACME_INT_TYPE_REGISTER_B; // Dirección del registro INT_TYPE_B//*
    buffer[2] = regB; // Valor para el registro INT_TYPE_B//*

    status=ACME_Write(buffer,sizeof(buffer));//*


    return status;
}
/*Implementa también la función ACME_readInt, que permita leer el valor del registro de interrupciones del
dispositivo ACME 3416.*/
//Lee el estado de interrupción
int32_t ACME_readInt (uint8_t *pin)
{
    //int32_t status=-1;//Cambiar
    uint8_t dir_register=ACME_INT_STATUS;//*
    int32_t status;//*

    status=ACME_WriteAndRead(&dir_register,sizeof(dir_register),pin,sizeof(uint8_t));//*
    return status;
}

//Funcion para probar la lectura...
int32_t ACME_read (uint8_t *bytes,uint8_t size)
{
    int32_t status;
    status=ACME_Read(bytes,size);
    return status;
}

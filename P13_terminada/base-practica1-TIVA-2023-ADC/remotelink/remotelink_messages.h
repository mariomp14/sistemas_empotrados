/*
 * remotelink_messages.h
 *
 *  Created on: March. 2019
 *
 */

#ifndef RL_MESSAGES_H
#define RL_MESSAGES_H
//Codigos de los mensajes y definicion de parametros para el protocolo RPC

// El estudiante debe agregar aqui cada nuevo mensaje que implemente. IMPORTANTE el orden de los comandos
// debe SER EL MISMO aqui, y en el codigo equivalente en la parte del microcontrolador (Code Composer)

typedef enum {//REALmente son valores num�ricos
    MESSAGE_REJECTED,
    MESSAGE_PING,
    MESSAGE_LED_GPIO,
    MESSAGE_LED_PWM_BRIGHTNESS,
    MESSAGE_ADC_SAMPLE,
    MESSAGE_MODE_GPIO_PWM,
    MESSAGE_COLOR_PWM,
    MESSAGE_CONSULTAR_PINES,
    MESSAGE_CONTROL_ASINCRONO,
    MESSAGE_CONFIGURAR_SOBREMUESTREO,
    MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO,
    MESSAGE_ADC_SAMPLE_AUTO,
    MESSAGE_ADC_SAMPLE_DIFF,
    MESSAGE_GPIO4_7,
   MESSAGE_GPIO0_3,
   MESSAGE_ACELERACION_y_GIRO,
   MESSAGE_MUESTREO_y_FRECUENCIA,
   MESSAGE_ACC_CAMBIAR_RANGO,
   MESSAGE_GIRO_CAMBIAR_RANGO,
    //etc, etc...
} messageTypes;

//Estructuras relacionadas con los parametros de los mensajes. El estuadiante debera crear las
// estructuras adecuadas a los mensajes usados, y asegurarse de su compatibilidad en ambos extremos

#pragma pack(1) //Cambia el alineamiento de datos en memoria a 1 byte.//NIVEL de empaquetamiento de la estructura
                                                                     //hace que no haya relleno entre los diferentes elementos de la estructura, independientemente del so

typedef struct {
    uint8_t command;
} MESSAGE_REJECTED_PARAMETER;

typedef union{//la diferencia de poner struct a union es que con union todos los campos se guardan en la misma posici�n de memoria
    struct {       //sirve para unir la estructura leds con value en una misma posicion de memoria. Si modifico leds se modificara value y viceversa
         uint8_t padding:1;//padding, red, blue y green son los 4 primero bits de value
         uint8_t red:1;
         uint8_t blue:1;
         uint8_t green:1;
    } leds;
    uint8_t value;
} MESSAGE_LED_GPIO_PARAMETER;

typedef struct {
    float rIntensity;
} MESSAGE_LED_PWM_BRIGHTNESS_PARAMETER;

typedef struct
{
    uint16_t channel[7];
   // uint16_t chan1;
   // uint16_t chan2;
   // uint16_t chan3;
   // uint16_t chan4;
   // uint16_t chan5;//a�adido por mi
   // uint16_t chan6;//a�adido por mi
   // uint16_t term;
} MESSAGE_ADC_SAMPLE_PARAMETER;

typedef struct
{
    bool pwm;
    bool gpio;

}MESSAGE_MODE_GPIO_PWM_PARAMETER;

typedef struct //*
{
    uint32_t arrayRGB[3];

}MESSAGE_COLOR_PWM_PARAMETER;

typedef struct//*
{
    uint8_t PF4;
     uint8_t PF0;

}MESSAGE_CONSULTAR_PINES_PARAMETER;

typedef struct
{
    uint8_t PF4;
    uint8_t PF0;
    bool automatico;
}MESSAGE_CONTROL_ASINCRONO_PARAMETER;

typedef struct
{
    uint8_t longitud;

}MESSAGE_CONFIGURAR_SOBREMUESTREO_PARAMETER;

typedef struct
{
    bool automatico;
    double frecuencia;

}MESSAGE_CONFIGURAR_FRECUENCIA_MUESTREO_PARAMETER;

typedef struct
{
    uint16_t channel[6][10];//aqui solo necesitamos 10 muestras por canal antes de realizar el env�o de informaci�n

}MESSAGE_ADC_SAMPLE_AUTO_PARAMETER;

typedef struct
{
    uint16_t channeldiff[3][20];//aqui solo necesitamos 10 muestras por canal antes de realizar el env�o de informaci�n

}MESSAGE_ADC_SAMPLE_DIFF_PARAMETER;

/*typedef struct
{
    uint8_t gpio4;
    uint8_t gpio5;
    uint8_t gpio6;
    uint8_t gpio7;

}MESSAGE_GPIO4_7_PARAMETER;
*/
typedef union{

    struct {
        uint8_t value0a3:4;
        uint8_t gpio4:1;//gpio4, gpio5, gpio6 y gpio7 son los 4 �timos bits de value
        uint8_t gpio5:1;
        uint8_t gpio6:1;
        uint8_t gpio7:1;
    } gpio4a7;

   uint8_t value;
} MESSAGE_GPIO4_7_PARAMETER;


typedef struct
{
    /*uint8_t gpio0;
    uint8_t gpio1;
    uint8_t gpio2;
    uint8_t gpio3;*/
    uint8_t gpio0a3;


}MESSAGE_GPIO0_3_PARAMETER;//leds qt//El error que me salia era por no poner aqui ;

typedef struct
{
  /*uint16_t giro_X;
  uint16_t giro_Y;
  uint16_t giro_Z;
  uint16_t aceleracion_X;
  uint16_t aceleracion_Y;
  uint16_t aceleracion_Z;*/
    uint16_t giro[3];
    uint16_t aceleracion[3];

}MESSAGE_ACELERACION_y_GIRO_PARAMETER;

typedef struct
{
    bool activar_muestreo;
    double frecuencia;
}MESSAGE_MUESTREO_y_FRECUENCIA_PARAMETER;

typedef struct
{
    uint8_t rango_acc;

}MESSAGE_ACC_CAMBIAR_RANGO_PARAMETER;

typedef struct
{
    uint8_t rango_giro;

}MESSAGE_GIRO_CAMBIAR_RANGO_PARAMETER;
#pragma pack()  //...Pero solo para los comandos que voy a intercambiar, no para el resto.

#endif // RPCCOMMANDS_H


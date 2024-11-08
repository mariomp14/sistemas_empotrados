/*
 * configADC.h
 *
 *  Created on: 22/4/2016
 *      Author: jcgar
 */

#ifndef CONFIGADC_H_
#define CONFIGADC_H_

#include<stdint.h>

typedef struct
{
    uint16_t chan[7];
	//uint16_t chan1;
	//uint16_t chan2;
	//uint16_t chan3;
	//uint16_t chan4;
	//uint16_t chan5;//añadido por mi
	//uint16_t chan6;//añadido por mi
	//uint16_t term;

} MuestrasADC;

typedef struct
{
    uint32_t chan[7];
	//uint32_t chan1;
	//uint32_t chan2;
	//uint32_t chan3;
	//uint32_t chan4;
	//uint32_t chan5;//añadido por mi
	//uint32_t chan6;//añadido por mi
	//uint32_t term;//

} MuestrasLeidasADC;

typedef struct
{
    uint16_t chanDiff[3];
}MuestrasADC1;

typedef struct
{
    uint32_t chanDiff[3];
}MuestrasLeidasADC1;

void configADC_ISR(void);
void configADC_DisparaADC(void);
void configADC_LeeADC(MuestrasADC *datos);
void configADC_IniciaADC(void);


#endif /* CONFIGADC_H_ */

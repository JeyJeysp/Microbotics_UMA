#ifndef CONFIGADC_H_
#define CONFIGADC_H_

#include <stdint.h>

typedef struct
{
    uint16_t chan;
} MuestrasADC;

typedef struct
{
    uint32_t chan;
} MuestrasLeida;


void configADC0_ISR(void);
void configADC0_LeeADC0(MuestrasADC *datos);
void configADC0_IniciaADC0(void);
void configADC_DisparaADC(void);

#endif /* CONFIGADC_H_ */

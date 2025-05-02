#include <stdint.h>
#include <stm32l432xx.h>

void delay(volatile uint32_t dly);
void initADC();
int readADC();
void initDAC();
void writeDAC(int value);
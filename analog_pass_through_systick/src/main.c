#include <eeng1030_lib.h>
void setup(void);
void delay(volatile uint32_t dly);
void initADC();
int readADC();
void initDAC();
void writeDAC(int value);

int main()
{
    setup();
    SysTick->LOAD = 1814 - 1; // Corrected: 80MHz / 44100 = 1814
    SysTick->CTRL = 7; // Enable SysTick and its interrupt
    SysTick->VAL = 10;
    __asm(" cpsie i ");
    while(1) { }
}

void delay(volatile uint32_t dly)
{
    while(dly--);
}

void setup()
{
    initClocks();
    RCC->AHB2ENR |= (1 << 0) + (1 << 1); // Enable GPIOA and GPIOB
    pinMode(GPIOB,3,1); // PB3 output (for timing debug)
    pinMode(GPIOA,0,3); // PA0 = analog mode (ADC in)
    pinMode(GPIOA,5,3); // PA5 = analog mode (DAC out)
    initADC();
    initDAC();
}

void SysTick_Handler(void)
{
    int vin;
    GPIOB->ODR |= (1 << 3);
    vin = readADC();  
    writeDAC(vin);
    GPIOB->ODR &= ~(1 << 3);
}

void initADC()
{
    RCC->AHB2ENR |= (1 << 13); // Enable ADC
    RCC->CCIPR |= (1 << 29) | (1 << 28); // Select system clock for ADC
    ADC1_COMMON->CCR = ((0b0000) << 18) + (1 << 22); // HCLK + VREFEN

    ADC1->CR = (1 << 28); // Enable ADC voltage regulator
    delay(100);
    ADC1->CR |= (1 << 31); // Start calibration
    while (ADC1->CR & (1 << 31)); // Wait for calibration

    ADC1->CFGR = (1 << 31); // Disable injected conversions
    ADC1_COMMON->CCR |= (0x00 << 18);

    ADC1->SQR1 &= ~(0x1F << 6);      // Clear previous channel
    ADC1->SQR1 |= (5 << 6);          // Channel 5 = PA0

    ADC1->CR |= (1 << 0); // Enable ADC
    while ((ADC1->ISR & (1 << 0)) == 0); // Wait for ready
}

int readADC()
{
    int rvalue = ADC1->DR;
    ADC1->ISR = (1 << 3); // Clear EOC
    ADC1->CR |= (1 << 2); // Start next conversion
    return rvalue;
}

void initDAC()
{
    RCC->APB1ENR1 |= (1 << 29);   // Enable DAC
    RCC->APB1RSTR1 &= ~(1 << 29); // Clear DAC reset
    DAC->CR &= ~(1 << 0);         // Disable DAC (reset)
    DAC->CR |= (1 << 0);          // Enable DAC channel 1 (PA5)
}

void writeDAC(int value)
{
    DAC->DHR12R1 = value; // Write to DAC channel 1
}

// Connect a POT betweek 0 and 3.3V with the wiper connected to PA0
// NOTE!!: PA0 actually connects to IN5 of ADC1.
// The POT voltage controls the duty cycle of a PWM output on PA3
// Timer 2 Channel 4     is used to generate the PWM output.
#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void initADC(void);
int readADC(int chan);
void initTimer2(void);
void setTimer2Duty(int duty);
int vin;
int main()
{
    setup();
    while(1)
    {        
        vin = readADC(5);  
        setTimer2Duty(vin);
        if (vin > 2047)
        {
            GPIOB->ODR |= (1 << 3);
        }
        else
        {
            GPIOB->ODR &= ~(1 << 3);
        }
        delay(100); // need to delay for a bit to allow the 
                    // PWM output to change to the desired duty                    
    }
}
void setup(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1); // digital output
    pinMode(GPIOB,4,0); // digital input
    enablePullUp(GPIOB,4); // pull-up for button
    pinMode(GPIOA,0,3);  // analog input
    pinMode(GPIOA,3,2);  // alternative function mode
    GPIOA->AFR[0] &= ~(3 << (2*3));    // Clear out old alternative function  bits
    GPIOA->AFR[0] |= 1 << (4*3);    // Select alternative function 1
    initADC();
    initTimer2();
}
void initADC()
{
    // initialize the ADC
    RCC->AHB2ENR |= (1 << 13); // enable the ADC
    RCC->CCIPR |= (1 << 29) | (1 << 28); // select system clock for ADC
    ADC1_COMMON->CCR = ((0b01) << 16) + (1 << 22) ; // set ADC clock = HCLK and turn on the voltage reference
    // start ADC calibration    
    ADC1->CR=(1 << 28); // turn on the ADC voltage regulator and disable the ADC
    delay(100); // wait for voltage regulator to stabilize (20 microseconds according to the datasheet).  This gives about 180microseconds
    ADC1->CR |= (1<< 31);
    while(ADC1->CR & (1 << 31)); // wait for calibration to finish.
    ADC1->CFGR = (1 << 31); // disable injection
    ADC1_COMMON->CCR |= (0x0f << 18);
}

int readADC(int chan)
{

    ADC1->SQR1 |= (chan << 6);
    ADC1->ISR = (1 << 3); // clear EOS flag
    ADC1->CR |= (1 << 0); // enable the ADC
    while ( (ADC1->ISR & (1 <<0))==0); // wait for ADC to be ready
    ADC1->CR |= (1 << 2); // start conversion
    while ( (ADC1->ISR & (1 <<3))==0); // wait for conversion to finish
    return ADC1->DR; // return the result
    ADC1->CR = 0;
}

void delay(volatile uint32_t dly)
{
    while(dly--);
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void initTimer2(void)
{
    RCC->APB1ENR1 |= (1 << 0); // enable Timer 2
    TIM2->CR1 = 0;
    TIM2->CCMR2 = (0b110 << 12) + (1 << 11)+(1 << 10);
    TIM2->CCER |= (1 << 12);
    TIM2->ARR = 1000-1;
    TIM2->CCR4 = 500;
    TIM2->EGR |= (1 << 0);
    TIM2->CR1 = (1 << 7);
    TIM2->CR1 |= (1 << 0);  
}
void setTimer2Duty(int duty)
{
    int arrvalue=(duty*TIM2->ARR)/4095;
    TIM2->CCR4=arrvalue;
}

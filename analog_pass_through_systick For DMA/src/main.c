// This example uses the SysTick timer to control the sample rate for an analog-passthrough system.
// The sample rate is set to 44100 Hz.  At this speed it is not possible to keep re-enabling the ADC and
// setting the channel number in the SysTick interrupt handler.  These actions take longer than the 
// sampling period.  The alternative approach is to configure and enable the ADC in the initADC function.
// The interrupt handler operates by starting a conversion at one interrupt event and then reading the
// result at the next event.  Enough time will have passed between interrupts (about 22us) for the conversion 
// to finish (about 5 microseconds) so it will be safe to read the ADC result

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
    SysTick->LOAD = 200-1; // Systick clock = 80MHz. 80000000/44100 = 1814
	SysTick->CTRL = 7; // enable systick counter and its interrupts
	SysTick->VAL = 10; // start from a low number so we don't wait for ages for first interrupt
	__asm(" cpsie i "); // enable interrupts globally
    while(1)
    {
        //TIM7->CNT; // read the timer to keep it running
    }
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void setup()
{
    initClocks();
    RCC->AHB2ENR |= (1 << 0) + (1 << 1); // enable GPIOA and GPIOB
    pinMode(GPIOB,3,1); // make PB3 an output.
    pinMode(GPIOA,0,1);
    pinMode(GPIOA,5,3);  // PA5 = analog mode (ADC in)
    pinMode(GPIOA,4,3);  // PA4 = analog mode (DAC out)
    initADC();
    initDAC();
}
void SysTick_Handler(void)
{
    int vin;
    GPIOB->ODR |= (1 << 3);
    vin = readADC(10);  
    writeDAC(vin);
    GPIOB->ODR &= ~(1 << 3); // toggle PB3 for timing measurement
}
void initADC()
{
    // initialize the ADC
    RCC->AHB2ENR |= (1 << 13); // enable the ADC
    RCC->CCIPR |= (1 << 29) | (1 << 28); // select system clock for ADC
    ADC1_COMMON->CCR = ((0b0000) << 18) + (1 << 22) ; // set ADC clock = HCLK and turn on the voltage reference
    // start ADC calibration    
    ADC1->CR=(1 << 28); // turn on the ADC voltage regulator and disable the ADC
    delay(100); // wait for voltage regulator to stabilize (20 microseconds according to the datasheet).  This gives about 180microseconds
    ADC1->CR |= (1<< 31);
    while(ADC1->CR & (1 << 31)); // wait for calibration to finish.
    ADC1->CFGR = (1 << 31); // disable injection
    ADC1_COMMON->CCR |= (0x00 << 18);
    ADC1->SQR1 |= (10 << 6);
     ADC1->CR |= (1 << 0); // enable the ADC
    while ( (ADC1->ISR & (1 <<0))==0); // wait for ADC to be ready
}
int readADC()
{

    int rvalue=ADC1->DR; // get the result from the previous conversion    
    ADC1->ISR = (1 << 3); // clear EOS flag
    ADC1->CR |= (1 << 2); // start next conversion    
    return rvalue; // return the result
}

void initDAC()
{

    RCC->APB1ENR1 |= (1 << 29);   // Enable the DAC
    RCC->APB1RSTR1 &= ~(1 << 29); // Take DAC out of reset
    DAC->CR &= ~(1 << 0);         // Enable = 0
    DAC->CR |= (1 << 0);          // Enable = 1
}
void writeDAC(int value)
{
    DAC->DHR12R1 = value;
}
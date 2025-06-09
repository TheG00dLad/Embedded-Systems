// This example uses Timer 15 to trigger ADC conversions on channel 5 (PA0)
// The ADC data is saved to a circular buffer in RAM using DMA
// It is then copied to the DAC also using DMA

#include <eeng1030_lib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
void initSerial(uint32_t baudrate);
void eputc(char c);
void setup(void);
void initADC(void);
int readADC(int chan);
void initTimer15(void);
int count;
uint16_t ADC_Buffer[16];
int main()
{
    setup();
    initADC();

    
    readADC(5); // not really a read, just sets up channel etc.
    initTimer15();
    initTimer7();
    initDAC();
    while(1)
    {
        printf("ADC->DR=%04d ",ADC1->DR);                    
        printf("ADC_Buffer[0]=%04d\r\n",ADC_Buffer[15]);     
    }

}

void setup()
{
    initClocks();    
    SysTick->LOAD = 80000-1; // Systick clock = 80MHz. 80000000/80000=1000
	SysTick->CTRL = 7; // enable systick counter and its interrupts
	SysTick->VAL = 10; // start from a low number so we don't wait for ages for first interrupt
	__asm(" cpsie i "); // enable interrupts globally
    RCC->AHB2ENR |= (1 << 0) + (1 << 1); // enable GPIOA and GPIOB
    initSerial(9600);

    RCC->AHB1ENR |= (1 << 0); // enable DMA1
    DMA1_Channel1->CCR = 0;
    DMA1_Channel1->CNDTR = 16;
    DMA1_Channel1->CPAR = (uint32_t)(&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)ADC_Buffer;
    DMA1_CSELR->CSELR = 0;
    DMA1_Channel1->CCR =  (1 << 7) | (1 << 5)+(1 << 10)+(1 << 8);   
    DMA1_Channel1->CCR |= (1 << 0);


    DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CNDTR = 16;
    DMA1_Channel3->CPAR = (uint32_t)(&(DAC->DHR8R1));
    DMA1_Channel3->CMAR = (uint32_t)ADC_Buffer;
    DMA1_CSELR->CSELR = (0b0110 << 8); // DMA Trigger = DAC channel 1.    
    DMA1_Channel3->CCR =  (1 << 7) | (1 << 5) | (1 << 4) | (1 << 10) | (1 << 8);   
    DMA1_Channel3->CCR |= (1 << 0);
}
void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX

    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=80000000;    
	uint32_t BaudRateDivisor;
	
	BaudRateDivisor = CLOCK_SPEED/baudrate;	
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 =  (1 << 3);  // enable the transmitter
	USART2->CR1 |= (1 << 0);
}
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    while(len--)
    {
        eputc(*data);    
        data++;
    }    
    return 0;
}
void eputc(char c)
{
    while( (USART2->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
    USART2->TDR=c;
}       

void initADC()
{
    // initialize the ADC
    RCC->AHB2ENR |= (1 << 13); // enable the ADC
    RCC->CCIPR |= (1 << 29) | (1 << 28); // select system clock for ADC
    ADC1_COMMON->CCR = ((0b01) << 18) + (1 << 22) ; // set ADC clock = HCLK and turn on the voltage reference
    // start ADC calibration    
    ADC1->CR=(1 << 28); // turn on the ADC voltage regulator and disable the ADC
    delay_ms(1); // wait for voltage regulator to stabilize (20 microseconds according to the datasheet).  This gives about 180microseconds
    ADC1->CR |= (1<< 31);
    while(ADC1->CR & (1 << 31)); // wait for calibration to finish.
    ADC1->CFGR = (1 << 31); // disable injection
    ADC1->CFGR |= (1 << 11); // enable external trigger
    ADC1->CFGR &= ~(1 << 10);
    ADC1->CFGR |= (0b1110 << 6); // select EXT14 (Timer 15) as trigger
    ADC1->CFGR |= (1 << 12); // enable Over run of ADC results
    ADC1->CFGR |= (1 << 1) + (1 << 0);
    ADC1_COMMON->CCR |= (0x0f << 18);



}
int readADC(int chan)
{

    ADC1->SQR1 |= (chan << 6);
    ADC1->ISR =  (1 << 3); // clear EOS flag
    ADC1->CR |= (1 << 0); // enable the ADC
    while ( (ADC1->ISR & (1 <<0))==0); // wait for ADC to be ready
    ADC1->CR |= (1 << 2); // start conversion
    /*while ( (ADC1->ISR & (1 <<3))==0); // wait for conversion to finish
    ADC1->CR &= ~(1 << 0); // disable the ADC
    */
    return ADC1->DR; // return the result
}
void initTimer15(void)
{
    // Timer 15 can be used to trigger the ADC    
    RCC->APB2ENR |= (1 << 16); // enable Timer 15
    TIM15->CR1 = 0;    
    TIM15->CR2 = (0b010 << 4); // update event is selected as trigger output  
    //TIM15->CR2 |= (1 << 5); // use update event as TRGO in master mode
    TIM15->PSC = 0;
    TIM15->ARR = 20000;
    TIM15->EGR = (1 << 0); // enable update event generation 
    TIM15->CR1 = (1 << 7);   
    TIM15->EGR |= 1;
 
    TIM15->CR1 |= (1 << 0);  
}

void initDAC()
{
    // Configure PA4 as a DAC output
    RCC->AHB2ENR |= (1 << 0);     // ensure GPIOA is enabled    
    GPIOA->MODER |= (1 << 8) | (1 << 9); // Set mode to analogue (DAC) 
    RCC->APB1ENR1 |= (1 << 29);   // Enable the DAC
    RCC->APB1RSTR1 &= ~(1 << 29); // Take DAC out of reset
    DAC->CR = 0;         // Enable = 0
    DAC->CR |= (1 << 12) | (0b010 << 3) | (1 << 2);  // enable DMA and TIM7 trigger
    DAC->CR |= (1 << 0);          // Enable = 1
    writeDAC(0);
}
void initTimer7(void)
{
    // Timer 2 can be used to trigger the DAC
    
    RCC->APB1ENR1 |= (1 << 5); // enable Timer 7
    TIM7->CR1 = 0;    
    TIM7->CR2 = (0b010 << 4); // update event is selected as trigger output
    TIM7->DIER = (1 << 8)+(1 << 0); // update dma request enabled - NECESSARY?
    TIM7->PSC = 0;
    TIM7->ARR = 20-1;
    TIM7->EGR = (1 << 0); // enable update event generation 
    TIM7->CR1 = (1 << 7);    
    TIM7->CR1 |= (1 << 0);  
}
void writeDAC(int value)
{
    DAC->DHR12R1 = value;
}

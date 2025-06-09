// Connect a POT betweek 0 and 3.3V with the wiper connected to PA0
// NOTE!!: PA0 actually connects to IN5 of ADC1.
//
#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void initADC(void);
int readADC(int chan);
int inpu;
void initDAC(void);
void writeDAC(int value);
void initTimer7(void);
void initClocks(void);

int main()
{
    setup();
    while(1)
    {        
        inpu = readADC(5);  
        writeDAC(inpu);
        if (inpu > 2047)
        {
            GPIOB->ODR |= (1 << 3);
        }
        else
        {
            GPIOB->ODR &= ~(1 << 3);
        }
    }
}
void setup(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1); // digital output
    pinMode(GPIOB,4,0); // digital input
    enablePullUp(GPIOB,4); // pull-up for button
    pinMode(GPIOA,0,3);  // analog input
    initADC();

    initClocks();
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    RCC->AHB1ENR |= (1 << 0); // enable DMA1
  /*  DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CNDTR = 256;
    DMA1_Channel3->CPAR = (uint32_t)(&(DAC->DHR8R1));
    DMA1_Channel3->CMAR = (uint32_t); //Need to make the ADC -> DAC
    DMA1_CSELR->CSELR = (0b0110 << 8); // DMA Trigger = DAC channel 1.    
    DMA1_Channel3->CCR =  (1 << 7) | (1 << 5) | (1 << 4);   
    DMA1_Channel3->CCR |= (1 << 0);*/
    initTimer7();
    initDAC();
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
void writeDAC(int value)
{
    DAC->DHR12R1 = value;
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

void initClocks()
{
	// Initialize the clock system to a higher speed.
	// At boot time, the clock is derived from the MSI clock 
	// which defaults to 4MHz.  Will set it to 80MHz
	// See chapter 6 of the reference manual (RM0393)
	    RCC->CR &= ~(1 << 24); // Make sure PLL is off
	
	// PLL Input clock = MSI so BIT1 = 1, BIT 0 = 0
	// PLLM = Divisor for input clock : set = 1 so BIT6,5,4 = 0
	// PLL-VCO speed = PLL_N x PLL Input clock
	// This must be < 344MHz
	// PLL Input clock = 4MHz from MSI
	// PLL_N can range from 8 to 86.  
	// Will use 80 for PLL_N as 80 * 4 = 320MHz
	// Put value 80 into bits 14:8 (being sure to clear bits as necessary)
	// PLLSAI3 : Serial audio interface : not using leave BIT16 = 0
	// PLLP : Must pick a value that divides 320MHz down to <= 80MHz
	// If BIT17 = 1 then divisor is 17; 320/17 = 18.82MHz : ok (PLLP used by SAI)
	// PLLQEN : Don't need this so set BIT20 = 0
	// PLLQ : Must divide 320 down to value <=80MHz.  
	// Set BIT22,21 to 1 to get a divisor of 8 : ok
	// PLLREN : This enables the PLLCLK output of the PLL
	// I think we need this so set to 1. BIT24 = 1 
	// PLLR : Pick a value that divides 320 down to <= 80MHz
	// Choose 4 to give an 80MHz output.  
	// BIT26 = 0; BIT25 = 1
	// All other bits reserved and zero at reset
	    RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) + (1 << 17) + (80 << 8) + (1 << 0);	
	    RCC->CR |= (1 << 24); // Turn PLL on
	    while( (RCC->CR & (1 << 25))== 0); // Wait for PLL to be ready
	// configure flash for 4 wait states (required at 80MHz)
	    FLASH->ACR &= ~((1 << 2)+ (1 << 1) + (1 << 0));
	    FLASH->ACR |= (1 << 2); 
	    RCC->CFGR |= (1 << 1)+(1 << 0); // Select PLL as system clock
}
// DMA to the DAC triggered by Timer 7 
// DAC output is on PA4
// It would appear that 16 bit (half word) transfers to the DAC are not possible 
// using DMA.  8 bits seem to work ok.  Not clear about 32 bit transfers.
// Using the DAC in 12 bit mode under software control works fine.  I suspect
// there is a problem involving the DMA on the AHB bus and the DAC on the APB bus

#include <stm32l432xx.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO

void setup(void);
void delay(volatile uint32_t dly);
void initClocks(void);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void initDAC(void);
void writeDAC(int value);
void initTimer7(void);
void initADC(void);
void eputc(char c);
/*
Octave/matlab code to generate the sine wave table:
clear
angle_step=pi/128;
angle=[0:angle_step:2*pi-angle_step];
s=sin(angle);
s=(s+1)/2;
s=s*4095;
s=floor(s);
f=fopen('sine.txt','wt');
for(i=1:length(s))
  fprintf(f,"%d,",s(i));
end
fclose(f);

*/
//volatile uint16_t adc_buffer[1]; // 12-bit samples
/*const uint8_t sine_table[]={127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173,176,179,182,184,\
                             187,190,193,195,198,200,203,205,208,210,213,215,217,219,221,224,226,228,229,231,233,\
                             235,236,238,239,241,242,244,245,246,247,248,249,250,251,251,252,253,253,254,254,254,\
                             254,254,255,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246,245,244,242,\
                             241,239,238,236,235,233,231,229,228,226,224,221,219,217,215,213,210,208,205,203,200,\
                             198,195,193,190,187,184,182,179,176,173,170,167,164,161,158,155,152,149,146,143,139,\
                             136,133,130,127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81,78,75,72,70,67,\
                             64,61,59,56,54,51,49,46,44,41,39,37,35,33,30,28,26,25,23,21,19,18,16,15,13,12,10,9,8,\
                             7,6,5,4,3,3,2,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,2,3,3,4,5,6,7,8,9,10,12,13,15,16,18,19,21,\
                             23,25,26,28,30,33,35,37,39,41,44,46,49,51,54,56,59,61,64,67,70,72,75,78,81,84,87,90,93,\
                             96,99,102,105,108,111,115,118,121,124};*/


int vin;
#define ADC_BUF_LEN 256
uint16_t adc_buffer[ADC_BUF_LEN];

int main()
{
    setup();
    printf("Setup complete\n");
    while(1)
    {
        delay(800000); // Increase delay for slower printing if needed (approx 10ms at 80MHz)
        // Read from a location likely updated by ADC DMA
        printf("ADC Value [0]: %u, ADC Value [128]: %u\r\n", adc_buffer[0], adc_buffer[128]); // Print elements
        // asm(" wfi "); // Keep commented out for initial debug
    }
}
void initClocks()
{
    printf("Initializing clocks...\n");
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

void setup(void)
{
    initClocks();
    initUSART2();
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1); // digital output
    pinMode(GPIOB,4,0); // digital input
    enablePullUp(GPIOB,4); // pull-up for button
    pinMode(GPIOA,5,3);  // analog input

    RCC->AHB1ENR |= (1 << 0); // enable DMA1
    // --- Configure DMA1 Channel 3 for Sine Table -> DAC Transfer ---
    // Disable DMA Channel 3 before configuration.
    // Writing 0 clears all control bits, including the enable bit (EN).
    DMA1_Channel3->CCR = 0;
    DMA1_Channel1->CCR = 0;
    // Set the Number of Data units To Transfer register.
    // Specifies that 256 transfers should occur before the DMA might stop
    // (or loop if circular mode is enabled). This matches the size of sine_table.
    DMA1_Channel3->CNDTR = 256;
    DMA1_Channel1->CNDTR = 256; // Number of ADC samples

    // Set the Peripheral Address Register (CPAR).
    // This is the DESTINATION address for the DMA transfer.
    // It points to the DAC Channel 1's 12-bit LEFT-aligned data holding register (DHR12L1).
    // Writing 8-bit data here puts it in the MSBs (bits 11:4) of the DAC input.
    DMA1_Channel3->CPAR = (uint32_t)(&(DAC->DHR12L1));
    DMA1_Channel1->CPAR = (uint32_t)&(ADC1->DR); // ADC data register

    // Set the Memory Address Register (CMAR).
    // This is the SOURCE address for the DMA transfer.
    // It points to the beginning of the sine_table array in memory.
    DMA1_Channel3->CMAR = (uint32_t)adc_buffer; // ADC data register
    DMA1_Channel1->CMAR = (uint32_t)adc_buffer; // Buffer to store ADC samples

    // The DMA will read from this address and write to the DAC's DHR12L1 register.
    // Configure the DMA Channel Selection Register (CSELR).
    // This maps a specific peripheral's DMA request signal to this DMA channel (Channel 3).
    // Value 0b0110 (decimal 6) corresponds to the DAC_CHANNEL1 request signal.
    // `<< 8` positions this value into the C3S (Channel 3 Select) bitfield (bits 11:8) within the CSELR register.
    DMA1_CSELR->CSELR = (0b0110 << 8); // DMA Trigger = DAC channel 1.
    // Configure the Channel Control Register (CCR) with multiple settings:
    // (1 << 7): Set MINC bit (Memory Increment Mode). The CMAR (memory address) will increment after each transfer.
    // (1 << 5): Set CIRC bit (Circular Mode). When CNDTR reaches 0, it reloads to the initial value, and CMAR resets to the start of sine_table, allowing continuous output.
    // (1 << 4): Set DIR bit (Data Transfer Direction). '1' means Read from Memory (CMAR->CPAR).
    // Note: MSIZE (Memory Size) and PSIZE (Peripheral Size) default to 00 (8-bit), matching the sine_table data type.
    DMA1_Channel3->CCR =  (1 << 7) | (1 << 5) | (1 << 4);
    // Enable DMA Channel 3 using a bitwise OR operation.
    // (1 << 0): Set the EN bit (Channel Enable).
    // This makes the channel active and ready to respond to the selected DMA request (from DAC_CHANNEL1).
    DMA1_Channel3->CCR |= (1 << 0);
    DMA1_Channel1->CCR = (1 << 7) | // MINC: Enable Memory Increment Mode
                     (1 << 5) | // CIRC: Enable Circular Mode
                     (0 << 4) | // DIR: Peripheral to Memory (0)
                     (0b01 << 10) | // PSIZE: Peripheral size = 16 bits
                     (0b01 << 8);   // MSIZE: Memory size = 16 bits

    DMA1_Channel1->CCR |= (1 << 0); // Enable DMA Channel 1
    printf("Before timer7 is on\n");
    initTimer7();
   // initTimer2();
    initDAC();
    initADC();
}
void initADC()
{
    RCC->AHB2ENR |= (1 << 0);     // ensure GPIOA is enabled
    GPIOA->MODER |= (1 << 10) | (1 << 11); // Set mode to analogue (ADC)
    RCC->AHB2ENR |= (1 << 13); // enable the ADC
    RCC->AHB2RSTR &= ~(1 << 13); // take ADC out of reset

    ADC1->CFGR &= ~ADC_CFGR_RES; // Clear RES bits to select 12-bit resolution
    ADC1->CFGR |= (0b00 << 3); // Set resolution to 12 bits
   // DONT WANT CONTINUOUS MODE FOR ADC, due to the timer.       ADC1->CFGR |= ADC_CFGR_CONT; ; // Enable single conversion mode
    ADC1->CFGR |= ADC_CFGR_EXTEN_0; // Enable external trigger on rising edge
    ADC1->CFGR |= ADC_CFGR_EXTSEL_0; // Select Timer 2 TRGO as trigger source
    ADC1->CFGR |= ADC_CFGR_DMACFG; // Enable DMA in circular mode

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
// void initADC()
// {
//     // initialize the ADC
//     RCC->AHB2ENR |= (1 << 13); // enable the ADC
//     RCC->CCIPR |= (1 << 29) | (1 << 28); // select system clock for ADC
//     ADC1_COMMON->CCR = ((0b0000) << 18) + (1 << 22) ; // set ADC clock = HCLK and turn on the voltage reference
//     // start ADC calibration    
//     ADC1->CR=(1 << 28); // turn on the ADC voltage regulator and disable the ADC
//     delay(100); // wait for voltage regulator to stabilize (20 microseconds according to the datasheet).  This gives about 180microseconds
//     ADC1->CR |= (1<< 31);
//     while(ADC1->CR & (1 << 31)); // wait for calibration to finish.
//     ADC1->CFGR = (1 << 31); // disable injection
//     ADC1_COMMON->CCR |= (0x00 << 18);
//     ADC1->SQR1 |= (10 << 6);
//     ADC1->CR |= (1 << 0); // enable the ADC
//     while ( (ADC1->ISR & (1 <<0))==0); // wait for ADC to be ready
// }

int readADC()
{
    int rvalue=ADC1->DR; // get the result from the previous conversion    
    ADC1->ISR = (1 << 3); // clear EOS flag
    ADC1->CR |= (1 << 2); // start next conversion    
    return rvalue; // return the result
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

void initTimer2(void)
{
    // Timer 2 to trigger the ADC.
    // Uses TIM2_CH2
    
    RCC->APB1ENR1 |= (1 << 0); // enable Timer 2
    TIM2->CR1 = 0;    
    TIM2->CR2 = (0b010 << 4); // update event is selected as trigger output
    TIM2->DIER = (1 << 8)+(1 << 0); // update dma request enabled - NECESSARY?
    TIM2->PSC = 0;
    TIM2->ARR = 1814-1; //44.1KHz
    TIM2->EGR = (1 << 0); // enable update event generation 
    TIM2->CR1 = (1 << 7);    
    TIM2->CR1 |= (1 << 0);  
}

// --- _write (Keep as is, but add check for STDOUT/STDERR) ---
int _write(int file, char *ptr, int len)
{
    // Optional: Check if it's stdout or stderr
    if (file != STDOUT_FILENO && file != STDERR_FILENO) {
         errno = EBADF;
         return -1;
    }

    int i;
    for (i = 0; i < len; i++) {
        // Wait until TX data register is empty
        while (!(USART2->ISR & USART_ISR_TXE));
        USART2->TDR = ptr[i];
    }
    // Optional: Wait for Transmission Complete
    // while (!(USART2->ISR & USART_ISR_TC));
    return len;
}

void eputc(char c)
{
    while( (USART2->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
    USART2->TDR=c;
}   
void initUSART2(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // PA2 (TX), PA3 (RX)
    GPIOA->MODER &= ~((3 << (2*2)) | (3 << (3*2)));
    GPIOA->MODER |=  (2 << (2*2)) | (2 << (3*2)); // AF mode
    GPIOA->AFR[0] |= (7 << (4*2)) | (7 << (4*3)); // AF7 for USART2

    USART2->BRR = 80000000 / 115200; // Assuming 80 MHz clock
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}
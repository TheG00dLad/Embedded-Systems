// Connect a POT betweek 0 and 3.3V with the wiper connected to PA0
// NOTE!!: PA0 actually connects to IN5 of ADC1.
// PA2 is used for USART 
#include <stm32l432xx.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "eeng1030_lib.h"
void setup(void);
void delay(volatile uint32_t dly);
void initTimer2(void);
void initSerial(uint32_t baudrate);
void eputc(char c);
int currentTimestamp = 0;
int oldTimestamp = 0;
int duration = 0;
int period = 0;
int main()
{
    setup();

    while(1)
    {        
        
        while(!(TIM2->SR & TIM_SR_CC2IF)) // Wait while CC1 flag is low
        {
            // Do nothing
           // printf("%ld\r\n",TIM2->CNT);
    
        }
        
        // Print the results
        currentTimestamp = TIM2->CCR2;
        duration = currentTimestamp - oldTimestamp;
        oldTimestamp = currentTimestamp;
        printf("Period (ms) %d\r\n",1000*SystemCoreClock/duration);
        
    }
}
void setup(void)
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB

    pinMode(GPIOB,3,1);                     // PB3 digital output - LED
    
    initTimer2();                           // Initialise timer 2
    // NVIC->ISER[0] |= (1 << 28);             // IRQ 28 maps to TIM2 (Alternative NVIC_EnableIRQ(TIM2_IRQn))
    // TIM2->DIER |= (1 << 1);                // Enable interrupt on CC1
    // __enable_irq();                         // __asm(" cpsie i ");

    initSerial(9600);                       // Initialise serial comms
    
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void initTimer2(void)
{

    // Configure PA1 as alternate function (AF1 for TIM2_CH2)
    GPIOA->MODER &= ~(GPIO_MODER_MODE1);        // Clear mode bits
    GPIOA->MODER |= (GPIO_MODER_MODE1_1);       // Set alternate function mode
    GPIOA->AFR[0] |= (1 << GPIO_AFRL_AFSEL1_Pos); // AF1 for TIM2
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD1);       // No pull-up/down

    // Enable TIM2 clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    // Configure TIM2 CH2 as input capture on TI2
    TIM2->CCMR1 &= ~TIM_CCMR1_CC2S;    // Clear CC2S
    TIM2->CCMR1 |= TIM_CCMR1_CC2S_0;   // CC2S = 01: IC2 mapped to TI2

    // Set input capture prescaler (optional)
    TIM2->CCMR1 &= ~TIM_CCMR1_IC2PSC;  // No prescaler, capture every event

    // Set input filter (optional)
    TIM2->CCMR1 &= ~TIM_CCMR1_IC2F;    // No filter

    // Configure active edge detection
    TIM2->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP); // Rising edge

    // Enable capture
    TIM2->CCER |= TIM_CCER_CC2E; // Enable capture on channel 2

    // Enable Timer counter
    TIM2->CR1 |= TIM_CR1_CEN;

}
void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX

    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=SystemCoreClock;    
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

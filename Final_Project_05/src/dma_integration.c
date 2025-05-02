#include <eeng1030_lib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include <string.h>      // Required for memcpy if used, but manual loop is fine too.
#include <stm32l4xx.h> // Include CMSIS device header for NVIC

#define BUFFER_SIZE 64
// Define two buffers: one for ADC input, one for DAC output
volatile uint16_t adc_buffer[BUFFER_SIZE];
volatile uint16_t dac_buffer[BUFFER_SIZE];


void initADC(void);
// int readADC(int chan); // No longer needed/used in DMA mode
void initTimer15(void);
void initTimer7(void);
void initDAC(void);
void writeDAC(int value); // Keep for initDAC initial value
void process_adc_data(uint32_t start_index, uint32_t length);
void DMA1_Channel1_IRQHandler(void); // Declare the ISR
void delay_us(uint32_t us); // Ensure delay_us is declared if used

// --- Main ---
// int main()
// {

//     initADC(); // Initializes ADC for PA5 (Channel 10) triggered by TIM15
//     // readADC(10); // REMOVED: This call is misleading/ineffective in DMA mode.
//                  // The channel is set permanently in initADC's SQR1.
//     initDAC();     // Initialize DAC before starting timers
//     initTimer15(); // Configure & Start Timer15 to trigger ADC conversions
//     initTimer7();  // Configure & Start Timer7 to trigger DAC conversions

//     // The core work is now done by DMA and the ISR.
//     // Main loop can monitor or do other tasks.
//     while (1)
//     {
//         // Display the most recent raw ADC value received (from buffer start)
//         printf("Most recent raw ADC value (Ch10): %04d | Processed DAC value: %04d\r\n",
//                adc_buffer[0], dac_buffer[0]);
//         delay_ms(200); // Slow down printf rate
//     }
// }

// --- Setup ---
// void setup()
// {
//     initClocks();
//     SysTick->LOAD = (80000000 / 1000) - 1; // Configure SysTick for 1ms delay_ms (if lib relies on it)
//     SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick, Processor clock
//     SysTick->VAL = 0;

//     // Enable global interrupts AFTER configuring SysTick if delay_ms is needed early
//     __enable_irq(); // Use CMSIS standard function
//     // __asm(" cpsie i "); // Or use inline assembly

//     RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN; // enable GPIOA and GPIOB
//     initSerial(9600); // Or higher like 115200

//     RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; // enable DMA1

//     // ADC DMA (Channel 1) - Writes to adc_buffer
//     DMA1_Channel1->CCR = 0;                 // Disable channel to configure
//     DMA1_Channel1->CNDTR = BUFFER_SIZE;     // Number of data transfers
//     DMA1_Channel1->CPAR = (uint32_t)(&(ADC1->DR)); // Peripheral address (ADC data register)
//     DMA1_Channel1->CMAR = (uint32_t)adc_buffer;   // Memory address (adc_buffer)
//     DMA1_CSELR->CSELR &= ~(DMA_CSELR_C1S);       // Select ADC1 trigger (0000) for DMA1 Channel 1
//     DMA1_Channel1->CCR = (DMA_CCR_MINC |    // Memory increment mode
//                           DMA_CCR_CIRC |    // Circular mode
//                           DMA_CCR_PSIZE_0 | // Peripheral size 16-bit
//                           DMA_CCR_MSIZE_0 | // Memory size 16-bit
//                           DMA_CCR_PL_1 |    // Priority High
//                           DMA_CCR_HTIE |    // Half Transfer interrupt enable
//                           DMA_CCR_TCIE);    // Transfer Complete interrupt enable
//                                             // DIR=0 (Peri->Mem) default
//     DMA1_Channel1->CCR |= DMA_CCR_EN;       // Enable DMA Channel 1

//     // DAC DMA (Channel 3) - Reads from dac_buffer
//     DMA1_Channel3->CCR = 0;                 // Disable channel to configure
//     DMA1_Channel3->CNDTR = BUFFER_SIZE;     // Number of data transfers
//     DMA1_Channel3->CPAR = (uint32_t)(&(DAC1->DHR12R1)); // Peripheral address (DAC data holding register) - Use DAC1
//     DMA1_Channel3->CMAR = (uint32_t)dac_buffer;   // Memory address (dac_buffer)
//     DMA1_CSELR->CSELR &= ~(DMA_CSELR_C3S);       // Clear selection for Channel 3
//     DMA1_CSELR->CSELR |= (0b0110 << DMA_CSELR_C3S_Pos); // Select DAC channel 1 trigger (request 6)
//     DMA1_Channel3->CCR = (DMA_CCR_MINC |    // Memory increment mode
//                           DMA_CCR_CIRC |    // Circular mode
//                           DMA_CCR_DIR |     // Direction: Read from memory
//                           DMA_CCR_PSIZE_0 | // Peripheral size 16-bit (for DHR12R1)
//                           DMA_CCR_MSIZE_0 | // Memory size 16-bit
//                           DMA_CCR_PL_1);    // Priority High
//     DMA1_Channel3->CCR |= DMA_CCR_EN;       // Enable DMA Channel 3

//     // Enable DMA1 Channel 1 Interrupt in NVIC
//     NVIC_SetPriority(DMA1_Channel1_IRQn, 1); // Set interrupt priority
//     NVIC_EnableIRQ(DMA1_Channel1_IRQn);      // Enable the interrupt
// }


// --- ADC Initialization (Corrected for PA5 / Channel 10) ---
void initADC()
{
    // -- Clocks and Pin Configuration --
    // GPIOA clock should be enabled in setup()
    // ADC clock enabled here
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;

    // Configure PA5 as Analog Input (ADC1_IN10)
    // Use the library function if available and reliable
    pinMode(GPIOA, 5, 3); // Mode 3 is Analog
    // Or direct register access:
    // GPIOA->MODER &= ~(GPIO_MODER_MODE5); // Clear bits first
    // GPIOA->MODER |= (GPIO_MODER_MODE5_0 | GPIO_MODER_MODE5_1); // Set bits for Analog mode

    // Select SYSCLK as ADC clock source (common setting)
    RCC->CCIPR |= (RCC_CCIPR_ADCSEL_1 | RCC_CCIPR_ADCSEL_0); // 0b11 = SYSCLK

    // -- Common ADC Settings --
    // Set CKMODE = Synchronous/HCLK / 1 (0b01)
    // Enable Vrefint channel (useful for calibration/monitoring, TSEN/VBATEN disabled)
    ADC1_COMMON->CCR = ADC_CCR_CKMODE_0 | ADC_CCR_VREFEN;

    // -- ADC Power Up and Calibration --
    ADC1->CR &= ~ADC_CR_DEEPPWD; // Exit deep power down
    ADC1->CR |= ADC_CR_ADVREGEN; // Enable ADC voltage regulator
    delay_us(20);                // Wait for regulator startup time (check datasheet, 20us is typical)

    ADC1->CR &= ~ADC_CR_ADCALDIF; // Ensure calibration is for single-ended mode
    ADC1->CR |= ADC_CR_ADCAL;     // Start ADC calibration
    while ((ADC1->CR & ADC_CR_ADCAL) != 0); // Wait for calibration to complete

    // -- ADC Configuration for Timer Triggered DMA --
    ADC1->CFGR = 0; // Reset configuration register
    // RES = 12-bit (00 - default)
    // ALIGN = Right alignment (0 - default)
    ADC1->CFGR |= (ADC_CFGR_EXTSEL_3 | ADC_CFGR_EXTSEL_2 | ADC_CFGR_EXTSEL_1); // EXTSEL = 0b1110 (TIM15_TRGO)
    ADC1->CFGR |= ADC_CFGR_EXTEN_0;  // EXTEN = 0b01 (Hardware Trigger on rising edge)
    ADC1->CFGR |= ADC_CFGR_OVRMOD;   // OVRMOD = 1 (Overrun mode enabled - DR overwritten)
    ADC1->CFGR |= ADC_CFGR_DMAEN;    // DMAEN = 1 (DMA Mode enabled)
    ADC1->CFGR |= ADC_CFGR_DMACFG;   // DMACFG = 1 (DMA Circular mode)
    // CONT = 0 (Single conversion mode - default)

    // -- Channel Specific Settings (Channel 10 / PA5) --
    // Set sampling time for channel 10. Needs SMPR2 register.
    // Example: 12.5 cycles (SMP[2:0] = 010)
    ADC1->SMPR2 &= ~ADC_SMPR2_SMP10;     // Clear previous SMP10 setting
    ADC1->SMPR2 |= ADC_SMPR2_SMP10_1;  // Set SMP10 = 010

    // Set regular sequence - 1 conversion, channel 10
    ADC1->SQR1 = 0; // Clear sequence settings (L=0 means 1 conversion)
    ADC1->SQR1 |= (10 << ADC_SQR1_SQ1_Pos); // SQ1 = channel 10

    // -- Enable ADC and Start --
    ADC1->ISR |= ADC_ISR_ADRDY; // Clear ADRDY flag by writing 1
    ADC1->CR |= ADC_CR_ADEN;    // Enable ADC
    while ((ADC1->ISR & ADC_ISR_ADRDY) == 0); // Wait for ADC Ready flag

    // Start ADC conversion. It will now wait for the TIM15 trigger.
    ADC1->CR |= ADC_CR_ADSTART;
}


// --- Other Functions (Unchanged unless noted) ---




// void initSerial(uint32_t baudrate)
// {
//     // Assumes GPIOA clock enabled
//     pinMode(GPIOA, 2, 2); // PA2 Alternate Function
//     selectAlternateFunction(GPIOA, 2, 7); // PA2 AF7 = USART2_TX

//     RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; // Enable USART2 Clock

//     const uint32_t CLOCK_SPEED = 80000000; // System clock speed
//     uint32_t BaudRateDivisor = CLOCK_SPEED / baudrate;

//     USART2->CR1 = 0; // Disable USART
//     USART2->CR2 = 0;
//     USART2->CR3 = 0; // No flow control, etc.
//     USART2->BRR = BaudRateDivisor;
//     USART2->CR1 = USART_CR1_TE | USART_CR1_UE; // Enable Transmitter and USART
// }


// readADC stub (not used for DMA)
/*
int readADC(int chan)
{
    // This function is incompatible with DMA/Timer triggered mode.
    return 0;
}
*/

void initTimer15(void) // Timer for ADC Trigger
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM15EN; // Enable TIM15 clock

    TIM15->CR1 = 0;    // Disable timer for configuration
    TIM15->CR2 = (TIM_CR2_MMS_1); // TRGO on Update Event (010)
    TIM15->PSC = 0;    // Prescaler = 0 (80 MHz timer clock)

    // Set frequency: Freq = 80MHz / (ARR+1)

    TIM15->ARR = 70; // Example: Set for ~44.1kHz

    TIM15->EGR = TIM_EGR_UG; // Generate an update event to load PSC/ARR
    TIM15->CR1 = TIM_CR1_ARPE; // Enable Auto-Reload Preload
    TIM15->CR1 |= TIM_CR1_CEN;  // Enable TIM15 counter
}

void initTimer7(void) // Timer for DAC Trigger
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN; // Enable TIM7 clock

    TIM7->CR1 = 0;    // Disable timer for configuration
    TIM7->CR2 = (TIM_CR2_MMS_1); // TRGO on Update Event (010)
    TIM7->DIER = TIM_DIER_UDE;   // Enable DMA request on Update Event
    TIM7->PSC = 0;    // Prescaler = 0 (80 MHz timer clock)

    // ARR MUST match TIM15 for synchronized ADC/DAC rates
    TIM7->ARR = TIM15->ARR; // Use the same ARR as TIM15

    TIM7->EGR = TIM_EGR_UG; // Generate an update event to load PSC/ARR
    TIM7->CR1 = TIM_CR1_ARPE;    // Enable Auto-Reload Preload
    TIM7->CR1 |= TIM_CR1_CEN;  // Enable TIM7 counter
}

void writeDAC(int value) // Only needed for initial value in initDAC
{
    // Use DAC1 since DAC peripheral is enabled
    DAC1->DHR12R1 = (uint16_t)value;
}

void initDAC()
{
    // Assumes GPIOA clock enabled
    // Configure PA4 as Analog Output
    pinMode(GPIOA, 4, 3); // Mode 3 is Analog
    // GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD4; // Ensure no pull-up/pull-down (usually default for analog)

    RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN;   // Enable DAC1 Clock

    // DAC1->CR configuration (use DAC1 registers)
    DAC1->CR = 0; // Reset DAC control register
    DAC1->CR |= (DAC_CR_TEN1 |           // Enable Trigger for Channel 1
                 DAC_CR_TSEL1_1 |        // Select TIM7_TRGO as Trigger (010)
                 DAC_CR_DMAEN1 |         // Enable DMA for Channel 1
                 DAC_CR_EN1);            // Enable DAC Channel 1
    // BOFF1=0 (Buffer enabled), WAVE1=00 (Disabled) are defaults

    // Optional: Initial DAC output value (DMA will overwrite this quickly)
    writeDAC(2048); // Set initial output to mid-range (assuming 0-4095)
}

// Simple blocking delay (ensure SysTick is running if using this)
void delay_us(uint32_t us) {
    // Crude delay - assumes 80MHz clock and minimal overhead
    volatile uint32_t count = us * 80 / 4; // Adjust divisor based on loop overhead
    while(count--);
}
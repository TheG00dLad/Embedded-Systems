// This example uses Timer 15 to trigger ADC conversions on channel 5 (PA0)
// The ADC data is saved to a circular buffer in RAM using DMA

#include <eeng1030_lib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include <stdbool.h>
#include <echo.h>
#include <display.h>
#include <string.h>
#include <dma_integration.h>

//#define SAMPLE_RATE 88200     // Sampling rate (samples per second)
#define DELAY_SECONDS 0.25    // Desired echo delay time in seconds
//#define BUFFER_SIZE (7500) // Calculate buffer size based on delay
#define ECHO_DECAY 0.6f       // Echo decay factor (0.0 to < 1.0) - Use 'f' for float
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 80
#define HEADER_HEIGHT 30
#define ENCODER_MAX 159
#define BUFFER_SIZE 4000

// Define two buffers: one for ADC input, one for DAC output
volatile uint16_t adc_buffer[BUFFER_SIZE];
volatile uint16_t dac_buffer[BUFFER_SIZE];
void initSerial(uint32_t baudrate);
void eputc(char c);
void setup(void);
void initADC(void);
int readADC(int chan);
void initTimer15(void);
void initTimer7(void );
void initDAC(void );
void delay(volatile uint32_t dly);
void initEncoder(void);
void settings(void);

uint16_t ADC_Buffer[16];
volatile bool recordingDone = false;  // Flag to indicate recording completion
volatile uint32_t bufferIndex = 0;         // Write index
volatile uint32_t playbackIndex = 0;
volatile uint32_t milliseconds;
int count;
volatile int mode = 0;                          // Current operating mode (0=Passthrough, 1=Echo, etc.)
volatile uint16_t audioBuffer[BUFFER_SIZE];     // Circular buffer for echo delay
volatile uint32_t bufferWriteIndex = 0;         // Index where new samples are written
volatile int percent_int = 0; // Percentage of the dial position;
volatile int vol_con;
uint16_t echoBuffer[BUFFER_SIZE];
uint32_t writeIndex = 0;

int main()
{
    uint16_t dial_background,rect_colour;
    uint16_t dial_x,dial_oldx;
    // BUTTON PRESS
    static uint8_t previous_button_state = 1; // Assume not pressed initially (HIGH due to pull-up)
    //static uint32_t last_debounce_time = 0;
    // const uint32_t debounce_delay = 50; // milliseconds debounce time
    //LOADING RECTANGLE
    // Define foreground color for the rectangle
    rect_colour = RGBToWord(255,128,215); // PRUPLE hehehe
    uint16_t rect_y = 0;     // Rectangle top edge
    uint16_t rect_height = 80;// Rectangle height (full screen height)
    mode = 0;

    drawRectangle(0,0,159,79,RGBToWord(255,0,0));
    dial_background = RGBToWord(255,255,255);
    fillRectangle(0,0,160,80,dial_background);    
    dial_x = dial_oldx = 0;
    //uint32_t encoderValue = TIM2->CNT;
    setup();
    //readADC(10); // not really a read, just sets up channel etc.

    // while(1)
    // {        
    // //printf("ADC->DR=%04d ",ADC1->DR);                    
    // //printf("ADC_Buffer[0]=%04d\r\n",ADC_Buffer[0]);     
    // }
    while (1)
    {
        uint16_t current_count = TIM2->CNT;
        //uint16_t percent_int = ((uint16_t)current_count*100)/159; //159 instead of 160 lets the dial reach 100% rather than 99%
        //printf("Position: %u%%\r\n", percent_int);
        // --- Update display based on the count ---
        dial_x = current_count; // Update dial_x
        // --- Update Display Only If Position Changed ---
        //BUTTON ---------------------------------------
        uint8_t current_button_reading = (GPIOB->IDR & GPIO_IDR_ID4) ? 1 : 0; // 1 if HIGH, 0 if LOW
        if (current_button_reading == 0 && previous_button_state == 1)
        {
            // === Button Pressed Event ===
            printf("Encoder Button Pressed!\r\n");

            if (mode == 0)
            {
                // Fill header background
                fillRectangle(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGBToWord(200, 0, 100));
                // Fill rest of screen
                fillRectangle(0, HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, dial_background);
                // Draw text in header
                printTextX2("Volume Control", 0, 10, (255), RGBToWord(200, 0, 100));
                // Draw bar below header
                rect_y = HEADER_HEIGHT + 10;
              //  uint16_t width_now = (dial_x + 1 > SCREEN_WIDTH) ? SCREEN_WIDTH : (dial_x + 1);
                mode = 1; // Change mode after processing
            }
            else if (mode == 1)
            {
                // Fill header background
                fillRectangle(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGBToWord(255, 230, 128));
                // Fill rest of screen
                fillRectangle(0, HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, dial_background);
                // Draw text in header
                printTextX2("Echo", 40, 10, (255), RGBToWord(255, 230, 128));
                // Draw bar below header
                rect_y = HEADER_HEIGHT + 10;
                uint16_t width_now = (dial_x + 1 > SCREEN_WIDTH) ? SCREEN_WIDTH : (dial_x + 1);
                if (width_now > 0) fillRectangle(0, rect_y, width_now, rect_height, rect_colour);
                mode = 2; // Change mode after processing
            }
            else if (mode == 2)
            {
                // Fill header background
                fillRectangle(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, RGBToWord(0, 255, 100));
                // Fill rest of screen
                fillRectangle(0, HEADER_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, dial_background);
                // Draw text in header
                printTextX2("Pass Through", 40, 10, (255), RGBToWord(200, 255, 255));
                // Draw bar below header
                rect_y = HEADER_HEIGHT + 10;
                // Need to redraw the bar too after changing background
                uint16_t width_now = (dial_x + 1 > SCREEN_WIDTH) ? SCREEN_WIDTH : (dial_x + 1);
                if (width_now > 0) fillRectangle(0, rect_y, width_now, rect_height, rect_colour);
                mode = 0; // Cycle back to mode 0 after processing
            }
            // Update state *after* processing the press
            previous_button_state = current_button_reading; // Now record it as pressed (0)
            //delay_ms(500); // Optional: Add a small delay to debounce the button
            // WHY IS IT WORSE?!
        }
        else if (current_button_reading == 1 && previous_button_state == 0)
        {
            // === Optional: Button Released Event ===
            // printf("Encoder Button Released!\r\n");
            previous_button_state = current_button_reading; // Record as released (1)
        }
        
        //-------------------------------------------------------------------------------------
        if (dial_x != dial_oldx)
        {
            // --- Incremental Update Logic ---
            if (dial_x > dial_oldx) 
            {
                // === Value INCREASED ===
                // The bar needs to grow. Fill the *newly added* segment with the foreground color.
                // Segment starts just after the old bar ended (at x = dial_oldx + 1).
                // Width of the segment is the difference: dial_x - dial_oldx.
                uint16_t start_x = dial_oldx + 1;
                uint16_t segment_width = dial_x - dial_oldx;
                // Ensure calculations don't go out of bounds (though unlikely here)
                if (start_x < SCREEN_WIDTH && segment_width > 0) 
                {
                // Make sure drawing doesn't exceed screen width
                if (start_x + segment_width > SCREEN_WIDTH) {
                    segment_width = SCREEN_WIDTH - start_x;
                }
                if (segment_width > 0) { // Check again after adjustment
                    fillRectangle(start_x, rect_y, segment_width, rect_height, rect_colour);
                }
                }
            } 
            else { // dial_x < dial_oldx
                // === Value DECREASED ===
                // The bar needs to shrink. Fill the *removed* segment with the background color.
                // Segment starts just after the new (smaller) bar ends (at x = dial_x + 1).
                // Width of the segment is the difference: dial_oldx - dial_x.
                uint16_t start_x = dial_x + 1;
                uint16_t segment_width = dial_oldx - dial_x;
                // Ensure calculations don't go out of bounds
                if (start_x < SCREEN_WIDTH && segment_width > 0) {
                // Make sure drawing doesn't exceed screen width
                if (start_x + segment_width > SCREEN_WIDTH) {
                    segment_width = SCREEN_WIDTH - start_x;
                }
                    if (segment_width > 0) { // Check again after adjustment
                    fillRectangle(start_x, rect_y, segment_width, rect_height, dial_background);
                }
                }
            }
            // --- Update old position AFTER drawing ---
            dial_oldx = dial_x;
        }
        delay_ms(1); // Small delay
    }

}

void setup()
{
    initClocks();
    SysTick->LOAD = 800 - 1; // Configure SysTick for 1ms delay_ms (if lib relies on it)
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick, Processor clock
    SysTick->VAL = 0;

    // Enable global interrupts AFTER configuring SysTick if delay_ms is needed early
    __enable_irq(); // Use CMSIS standard function
    // __asm(" cpsie i "); // Or use inline assembly

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN; // enable GPIOA and GPIOB
    initSerial(9600); // Or higher like 115200

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN; // enable DMA1

    // ADC DMA (Channel 1) - Writes to adc_buffer
    DMA1_Channel1->CCR = 0;                 // Disable channel to configure
    DMA1_Channel1->CNDTR = BUFFER_SIZE;     // Number of data transfers
    DMA1_Channel1->CPAR = (uint32_t)(&(ADC1->DR)); // Peripheral address (ADC data register)
    DMA1_Channel1->CMAR = (uint32_t)adc_buffer;   // Memory address (adc_buffer)
    DMA1_CSELR->CSELR &= ~(DMA_CSELR_C1S);       // Select ADC1 trigger (0000) for DMA1 Channel 1
    DMA1_Channel1->CCR = (DMA_CCR_MINC |    // Memory increment mode
                          DMA_CCR_CIRC |    // Circular mode
                          DMA_CCR_PSIZE_0 | // Peripheral size 16-bit
                          DMA_CCR_MSIZE_0 | // Memory size 16-bit
                          DMA_CCR_PL_1 |    // Priority High
                          DMA_CCR_HTIE |    // Half Transfer interrupt enable
                          DMA_CCR_TCIE);    // Transfer Complete interrupt enable
                                            // DIR=0 (Peri->Mem) default
    DMA1_Channel1->CCR |= DMA_CCR_EN;       // Enable DMA Channel 1

    // DAC DMA (Channel 3) - Reads from dac_buffer
    DMA1_Channel3->CCR = 0;                 // Disable channel to configure
    DMA1_Channel3->CNDTR = BUFFER_SIZE;     // Number of data transfers
    DMA1_Channel3->CPAR = (uint32_t)(&(DAC1->DHR12R1)); // Peripheral address (DAC data holding register) - Use DAC1
    DMA1_Channel3->CMAR = (uint32_t)dac_buffer;   // Memory address (dac_buffer)
    DMA1_CSELR->CSELR &= ~(DMA_CSELR_C3S);       // Clear selection for Channel 3
    DMA1_CSELR->CSELR |= (0b0110 << DMA_CSELR_C3S_Pos); // Select DAC channel 1 trigger (request 6)
    DMA1_Channel3->CCR = (DMA_CCR_MINC |    // Memory increment mode
                          DMA_CCR_CIRC |    // Circular mode
                          DMA_CCR_DIR |     // Direction: Read from memory
                          DMA_CCR_PSIZE_0 | // Peripheral size 16-bit (for DHR12R1)
                          DMA_CCR_MSIZE_0 | // Memory size 16-bit
                          DMA_CCR_PL_1);    // Priority High
    DMA1_Channel3->CCR |= DMA_CCR_EN;       // Enable DMA Channel 3

    // Enable DMA1 Channel 1 Interrupt in NVIC
    NVIC_SetPriority(DMA1_Channel1_IRQn, 1); // Set interrupt priority
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);      // Enable the interrupt

    pinMode(GPIOA,5,3);  // PA5 = analog mode (ADC in)
    pinMode(GPIOA,4,3);  // PA4 = analog mode (DAC out)
   
    //----------------------------------------------------------
    pinMode(GPIOA, 3, 1); // Configure PA3 as Output (mode 1)
    pinMode(GPIOB,4,0);
    enablePullUp(GPIOB,4);  
    
    initADC();
    initDAC();
    initTimer15();
    initTimer7();
    init_display();
    initEncoder();
    settings();
    __enable_irq();
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

// void initADC()
// {
//     // initialize the ADC
//     RCC->AHB2ENR |= (1 << 13); // enable the ADC
//     RCC->CCIPR |= (1 << 29) | (1 << 28); // select system clock for ADC
//     ADC1_COMMON->CCR = ((0b01) << 16) + (1 << 22) ; // set ADC clock = HCLK and turn on the voltage reference
//     // start ADC calibration    
//     ADC1->CR=(1 << 28); // turn on the ADC voltage regulator and disable the ADC
//     delay_ms(1); // wait for voltage regulator to stabilize (20 microseconds according to the datasheet).  This gives about 180microseconds
//     ADC1->CR |= (1<< 31);
//     while(ADC1->CR & (1 << 31)); // wait for calibration to finish.
//     ADC1->CFGR = (1 << 31); // disable injection
//     ADC1->CFGR |= (1 << 11); // enable external trigger
//     ADC1->CFGR &= ~(1 << 10);
//     ADC1->CFGR |= (0b1110 << 6); // select EXT14 (Timer 15) as trigger
//     ADC1->CFGR |= (1 << 12); // enable Over run of ADC results
//     ADC1->CFGR |= (1 << 1) + (1 << 0);
//     //ADC1_COMMON->CCR |= (0x0f << 18);



// }
// int readADC(int chan)
// {

//     ADC1->SQR1 |= (chan << 6);
//     ADC1->ISR =  (1 << 3); // clear EOS flag
//     ADC1->CR |= (1 << 0); // enable the ADC
//     while ( (ADC1->ISR & (1 <<0))==0); // wait for ADC to be ready
//     ADC1->CR |= (1 << 2); // start conversion
//     /*while ( (ADC1->ISR & (1 <<3))==0); // wait for conversion to finish
//     ADC1->CR &= ~(1 << 0); // disable the ADC
//     */
//     return ADC1->DR; // return the result
// }

void SysTick_Handler(void)
{    
    milliseconds++;
}
//    uint32_t encoderValue = TIM2->CNT;
//      if (mode == 0) {
//         // Mode 0: Passthrough
//         // uint16_t adcValue = readADC(10);
//         // writeDAC(adcValue);
//      }
//     if (mode == 1) {
//         // Mode 1: Volume Control
//         float volume = (float)encoderValue / ENCODER_MAX;
//         uint16_t adcValue = readADC(10);
//         uint16_t adjustedValue = (uint16_t)((float)adcValue*volume);
//         if (adjustedValue > 4095) adjustedValue = 4095;
//         writeDAC(adjustedValue);
//     }
//  if (mode == 2) {
//         // Mode 2: Echo Effect
//         uint32_t delay = (encoderValue * 2000) / ENCODER_MAX; // scale delay to max 2000 samples
//         uint32_t readIndex = (writeIndex + BUFFER_SIZE - delay) % BUFFER_SIZE;
//         uint16_t input = readADC(10);
//         uint16_t echo = echoBuffer[readIndex];
//         uint32_t output = (input + echo) / 2;
//         if (output > 4095) output = 4095;
//         echoBuffer[writeIndex] = input;
//         writeIndex = (writeIndex + 1) % BUFFER_SIZE;
//         writeDAC((uint16_t)output);
//     }

//#define BUFFER_SIZE 256
//uint16_t ADC_Buffer[BUFFER_SIZE];
uint16_t Adjusted_Buffer[BUFFER_SIZE];

void settings()
{
    uint32_t encoderValue = TIM2->CNT;
    if (mode==0)
    {
        //Passthrough mode
        

    }
    else if (mode == 1)
{
    float volume = (float)encoderValue / ENCODER_MAX;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        Adjusted_Buffer[i] = (uint16_t)((float)ADC_Buffer[i] * volume);
    }
}

    else if (mode==2)
    {
       // DMA1_Channel1->CMAR = (uint32_t);
    }
}
#define DELAY_SAMPLES 32
void process_adc_data(uint32_t start_index, uint32_t length)
{
    if (mode == 0)
    {
        // Passthrough mode - Directly copy ADC data to DAC buffer
        // Consider using memcpy for potential speedup if length is large
        // memcpy((void*)&dac_buffer[start_index], (void*)&adc_buffer[start_index], length * sizeof(uint16_t));
        for (uint32_t i = 0; i < length; ++i)
        {
            dac_buffer[start_index + i] = adc_buffer[start_index + i];
        }
    }
    else if (mode == 1)
    {
        // --- READ ENCODER VALUE *ONCE* ---
        uint16_t current_encoder_val = TIM2->CNT;

        // --- FIXED-POINT VOLUME ---
        // Scale encoder value to a 16-bit fractional representation (0 to 65535)
        // Use 32-bit intermediate to avoid overflow during calculation
        uint32_t volume_fixed = ((uint32_t)current_encoder_val * 65536) / (ENCODER_MAX + 1); // +1 to allow reaching full scale slightly easier
        if (volume_fixed > 65535) volume_fixed = 65535; // Clamp to max

        uint32_t temp_val; // Use 32-bit for intermediate multiplication

        for (uint32_t i = 0; i < length; ++i)
        {
            // Multiply ADC value (12-bit) by volume (16-bit fixed-point)
            temp_val = (uint32_t)adc_buffer[start_index + i] * volume_fixed;

            // Result is 28-bit (12 + 16). Shift right by 16 bits to get the scaled 12-bit result.
            dac_buffer[start_index + i] = (uint16_t)(temp_val >> 16);

            // Optional clamping (less likely needed with fixed point if scaling is right)
            if (dac_buffer[start_index + i] > 4095) {
               dac_buffer[start_index + i] = 4095;
            }
        }
    }
    else if (mode == 2) // Echo mode
    {
        // *** Your Echo implementation needs to be added here ***
        // Placeholder: Simple attenuation (as before)
         const uint32_t delay = (DELAY_SAMPLES < BUFFER_SIZE) ? DELAY_SAMPLES : BUFFER_SIZE - 1;
         if (delay == 0) {
             for (uint32_t i = 0; i < length; ++i) {
                dac_buffer[start_index + i] = adc_buffer[start_index + i] / 2; // Simple passthrough/attenuation
             }
             return;
        }

        // --- EXAMPLE ECHO LOGIC (Needs refinement based on your exact goal) ---
        // This simple echo mixes current input with a delayed sample from adc_buffer itself.
        // A separate echo buffer is usually better for decaying echo.
        for (uint32_t i = 0; i < length; ++i) {
            uint32_t current_abs_index = start_index + i;
            // Calculate the index to read the delayed sample from *within the current processing chunk*
            // This requires careful handling of buffer wrap-around if delay > length
            // A simpler approach often uses a separate, larger circular buffer for the delay line.

            // Simplified example (likely incorrect for true echo effect across buffer halves):
            // uint32_t read_abs_index = (current_abs_index + BUFFER_SIZE - delay) % BUFFER_SIZE; // WRONG FOR HALF-BUFFER PROCESSING

            // Let's stick to simple attenuation for now until echo logic is designed
             dac_buffer[start_index + i] = adc_buffer[start_index + i] / 2; // Replace with actual echo
        }
         // Proper echo needs state maintained across calls (writeIndex, potentially separate echo buffer)
         // This function processes chunks, making direct circular buffer access complex within it.
    }
}
void DMA1_Channel1_IRQHandler(void)
{
    uint32_t isr_flags = DMA1->ISR; // Read flags once for efficiency

    // Check for Half Transfer complete interrupt flag
    if ((isr_flags & DMA_ISR_HTIF1) != 0)
    {
        DMA1->IFCR = DMA_IFCR_CHTIF1; // Clear the Half Transfer flag for Channel 1
        process_adc_data(0, BUFFER_SIZE / 2); // Process the first half
    }

    // Check for Transfer Complete interrupt flag
    if ((isr_flags & DMA_ISR_TCIF1) != 0)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF1; // Clear the Transfer Complete flag for Channel 1
        process_adc_data(BUFFER_SIZE / 2, BUFFER_SIZE / 2); // Process the second half
    }

    // Optional: Check for Transfer Error flag
    if ((isr_flags & DMA_ISR_TEIF1) != 0)
    {
         DMA1->IFCR = DMA_IFCR_CTEIF1; // Clear the Transfer Error flag
         // Handle error, e.g., toggle an LED or print error
         printf("!!! ADC DMA Error (Ch1) !!!\r\n");
    }

    // Clear the Global Interrupt flag - often redundant if specific flags cleared, but safe
     DMA1->IFCR = DMA_IFCR_CGIF1;
}
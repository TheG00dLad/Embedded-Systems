/*******************************************************************************
* Pin Configuration (Based on Setup and Peripheral Initialization)
********************************************************************************
*   Microcontroller: STM32L432KC (Nucleo-L432KC board assumed)
*
*   Analog I/O :
*     PA5: ADC1_IN5        - Analog Audio Input
*     PA4: DAC1_OUT1       - Analog Audio Output
*

*
*   Rotary Encoder:
*     - TIM2 Channel Pins: Configured within initEncoder() library function.
*                          Commonly PA0/PA1 or PA15/PB3 or PA8/PA9.
*                          *** WARNING: PA0 is used by ADC1_IN5 in this code. ***
*                          Ensure encoder library or hardware uses different pins (e.g., PA8/PA9).
*     - PB4: GPIO Input     - Encoder Push Button (with internal Pull-Up enabled)
*     - PA13: SWDIO         - Serial Wire Debug IO
*     - PA14: SWCLK         - Serial Wire Debug Clock
*     - PB3: SWO           - Serial Wire Output (Optional Trace Output)
*   LCD Display (SPI):
*     - SPI Pins (SCK, MOSI): Configured within init_display() library function.
*                              Likely SPI1 (e.g., PA5/PA7 or PB3/PB5) or SPI3.
*     - Control Pins (CS, DC, RST): Configured within init_display(). Specific
*                                  GPIO pins depend on library & wiring.
*   Serial Debug (USART2):
*     PA2: USART2_TX       - Transmit Data (to PC via ST-Link VCOM)
*     PA1: USART2_RX       - (Likely unused, ST-Link VCOM RX)
*     PA7
*     PA6
*     PA8
*     PA12
*   Miscellaneous GPIO:
*     PA3: GPIO Output      - Purpose currently unspecified in main logic.
*
*   Trigger Timers:
*     - Timer 15 (TIM15): Used to trigger ADC conversions (Internal TRGO signal).
*     - Timer 7 (TIM7): Used to trigger DAC conversions (Internal TRGO signal).
*
*   DMA Requests:
*     - DMA1 Channel 1: Triggered by ADC1 conversion complete.
*     - DMA1 Channel 3: Triggered by TIM7 TRGO (routed via DAC request).
*
*   System/Debug:
*     - NRST: Reset Pin
*******************************************************************************/

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
#define DELAY_SECONDS 0.5    // Desired distortion delay time in seconds
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 80
#define HEADER_HEIGHT 30
#define ENCODER_MAX 159
#define BUFFER_SIZE 3950
#define distortion_BUFFER_SIZE BUFFER_SIZE // Use the same size for simplicity, or make it larger for longer delays
#define MIN_DELAY_SAMPLES 10         // Minimum distortion delay in samples (must be > 0)
#define MAX_DELAY_SAMPLES (distortion_BUFFER_SIZE - 1) // Max delay limited by buffer size
#define distortion_DECAY 1f       // distortion decay factor (0.0 to < 1.0) - Use 'f' for float
#define distortion_DECAY_FIXED_Q8 153 // (uint8_t)(0.6f * 256.0f)
#define distortion_DECAY_SHIFT 8      // The amount to shift back
// --- Fixed-point parameters for decay calculation ---
#define distortion_DECAY_SHIFT 8      // Q-format shifter (e.g., 8 for Q8)
// Maximum decay factor in Q8 format (e.g., 230/256 is approx 0.9, safely below 1.0)
#define MAX_DECAY_VALUE_Q8 230

// --- FIX: Set a fixed delay time for the distortion ---
#define FIXED_distortion_DELAY_SAMPLES (distortion_BUFFER_SIZE / 2) // Example: delay is 1/2 of the buffer size

// Buffer to store past samples for distortion effect
volatile uint16_t distortion_history_buffer[distortion_BUFFER_SIZE];
// Write index for the distortion history buffer
volatile uint32_t distortion_write_index = 0;
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
volatile int mode = 0;                          // Current operating mode (0=Passthrough, 1=Volume, 2=distortion)
volatile uint16_t audioBuffer[BUFFER_SIZE];     // Circular buffer for distortion delay (if needed differently than history)
volatile uint32_t bufferWriteIndex = 0;         // Index where new samples are written
volatile int percent_int = 0; // Percentage of the dial position;
volatile int vol_con;
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
    setup();

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
                printTextX2("Distortion", 40, 10, (255), RGBToWord(255, 230, 128)); // Changed text
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

    memset((void*)distortion_history_buffer, 0, sizeof(distortion_history_buffer));
    distortion_write_index = 0; // Start writing at the beginning

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
    // --- IMPORTANT: FIX DMA MUX FOR ADC1 ---
    // ADC1 is request 5 on L432KC. Setting to 0 selects MEM2MEM.
    DMA1_CSELR->CSELR &= ~(DMA_CSELR_C1S);       // Clear current selection
    DMA1_CSELR->CSELR |= (5 << DMA_CSELR_C1S_Pos); // Select ADC1 (request 5) for DMA1 Channel 1
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
    DMA1_CSELR->CSELR |= (6 << DMA_CSELR_C3S_Pos); // Select DAC channel 1 trigger (request 6) - Correct
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

    pinMode(GPIOA,0,3);  // PA0 = analog mode (ADC1_IN5)
    pinMode(GPIOA,4,3);  // PA4 = analog mode (DAC1_OUT1)

    //----------------------------------------------------------
    
    pinMode(GPIOB,4,0);   // PB4 as Input (for encoder button)
    enablePullUp(GPIOB,4); // Enable pull-up for the button

    initADC();
    initDAC();
    initTimer15();
    initTimer7();
    init_display();
    initEncoder();
    __enable_irq(); // Enable interrupts globally *AFTER* all peripheral setup
}

void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2 (USART2 TX)
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX

    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; // turn on USART2 clock

	const uint32_t CLOCK_SPEED=80000000; // Assuming HCLK = 80 MHz
	uint32_t BaudRateDivisor;

	// For oversampling by 16 (default), USARTDIV = HCLK / BaudRate
	BaudRateDivisor = CLOCK_SPEED / baudrate;

	USART2->CR1 = 0; // Disable USART first
	USART2->CR2 = 0;
	USART2->CR3 = 0; // No hardware flow control, etc.
    // USART2->CR3 |= USART_CR3_OVRDIS; // Disable overrun detection if needed

	USART2->BRR = BaudRateDivisor; // Set baud rate

	USART2->CR1 |= USART_CR1_TE;  // Enable Transmitter
	USART2->CR1 |= USART_CR1_UE;  // Enable USART
}
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    // Transmit data using polling
    for (int i = 0; i < len; i++) {
        eputc(data[i]);
    }
    return len; // Return number of chars written
}
void eputc(char c)
{
    // Wait for Transmit Data Register Empty flag
    while( !(USART2->ISR & USART_ISR_TXE) );
    USART2->TDR = c; // Send char
}

void SysTick_Handler(void)
{
    milliseconds++; // Used for delay_ms()
}

void process_adc_data(uint32_t start_index, uint32_t length)
{
    if (mode == 0) // Passthrough Mode
    {
        // Directly copy ADC data to DAC buffer
        memcpy((void*)&dac_buffer[start_index], (const void*)&adc_buffer[start_index], length * sizeof(uint16_t));
        // for (uint32_t i = 0; i < length; ++i)
        // {
        //     dac_buffer[start_index + i] = adc_buffer[start_index + i];
        // }
    }
    else if (mode == 1) // Volume Control Mode
    {
        // --- READ ENCODER VALUE *ONCE* per processing block ---
        uint16_t current_encoder_val = TIM2->CNT; // Read current encoder count

        // --- FIXED-POINT VOLUME (Q16 format: 0x0000 to 0xFFFF maps to 0.0 to ~1.0) ---
        // Scale encoder value [0, ENCODER_MAX] to [0, 65535]
        uint32_t volume_fixed = ((uint32_t)current_encoder_val * 65536) / (ENCODER_MAX + 1); // Use 32-bit intermediate
        if (volume_fixed > 65535) volume_fixed = 65535; // Clamp to max Q16 value

        uint32_t temp_val; // Use 32-bit for intermediate multiplication

        for (uint32_t i = 0; i < length; ++i)
        {
            uint16_t current_sample = adc_buffer[start_index + i]; // Get sample from ADC buffer

            // Multiply ADC value (assume 12-bit, 0-4095) by volume (16-bit fixed-point, Q16)
            temp_val = (uint32_t)current_sample * volume_fixed;

            // Result is potentially 28-bit (12 + 16). Shift right by 16 bits to scale back to 12-bit range.
            uint16_t processed_sample = (uint16_t)(temp_val >> 16);

            // Clamp to valid DAC range (0 to 4095 for 12-bit DAC)
            if (processed_sample > 4095) {
               processed_sample = 4095;
            }
            dac_buffer[start_index + i] = processed_sample; // Write processed sample to DAC buffer
        }
    }
    else if (mode == 2) // distortion Mode (Encoder controls Decay/Feedback)
    {
        // --- Read encoder and calculate DYNAMIC decay factor *ONCE* per chunk ---
        uint16_t encoder_value = TIM2->CNT;
        uint32_t dynamic_decay_fixed_q8; // Variable for the decay factor (Q8 format)

        // Map encoder value [0, ENCODER_MAX] to decay factor [0, MAX_DECAY_VALUE_Q8]
        if (ENCODER_MAX > 0) {
             // Scale encoder value to Q8 range [0, 230]
             dynamic_decay_fixed_q8 = ((uint32_t)encoder_value * MAX_DECAY_VALUE_Q8) / ENCODER_MAX;
        } else {
             dynamic_decay_fixed_q8 = 0; // Default to no feedback if ENCODER_MAX is 0
        }

        // --- Use the FIXED delay time ---
        const uint32_t delay_in_samples = FIXED_distortion_DELAY_SAMPLES;

        // --- Ensure fixed delay is valid (safety check - IMPORTANT) ---
        // Delay must be > 0 and < distortion_BUFFER_SIZE
        if (delay_in_samples >= distortion_BUFFER_SIZE || delay_in_samples == 0) {
             // Invalid delay setting! Fallback to passthrough to avoid errors.
             memcpy((void*)&dac_buffer[start_index], (const void*)&adc_buffer[start_index], length * sizeof(uint16_t));
             // Optionally print an error message here (if printf is safe within ISR context, usually not recommended)
             // printf("Warning: Invalid distortion delay!\r\n");
             return; // Skip distortion processing for this chunk
        }


        // --- Variables for the loop ---
        uint16_t current_sample;
        uint16_t delayed_sample;
        uint32_t output_sample_temp; // Use 32-bit for intermediate addition
        uint32_t read_index;
        uint32_t decayed_sample_fixed; // Use 32-bit for intermediate multiplication

        // --- Process samples in the chunk ---
        for (uint32_t i = 0; i < length; ++i)
        {
            // 1. Get current sample from ADC buffer
            current_sample = adc_buffer[start_index + i];

            // 2. Calculate read index for the delayed sample using the FIXED delay
            // Modulo arithmetic handles buffer wrap-around
            read_index = (distortion_write_index + distortion_BUFFER_SIZE - delay_in_samples) % distortion_BUFFER_SIZE;

            // 3. Get delayed sample from history buffer
            delayed_sample = distortion_history_buffer[read_index];

            // 4. Calculate the feedback term using DYNAMIC FIXED-POINT decay (Q8)
            // Multiply delayed sample (12-bit) by decay factor (Q8) -> Result is Q8
            decayed_sample_fixed = (uint32_t)delayed_sample * dynamic_decay_fixed_q8;
            // Shift right by shift amount (8) to scale back to integer range
            uint16_t decayed_sample = (uint16_t)(decayed_sample_fixed >> distortion_DECAY_SHIFT);

            // 5. Mix current sample with the decayed delayed sample
            output_sample_temp = (uint32_t)current_sample + decayed_sample;

            // 6. Clamp output to valid DAC range (0-4095)
            if (output_sample_temp > 4095) {
                output_sample_temp = 4095;
            }
            // Write final sample to the DAC output buffer
            dac_buffer[start_index + i] = (uint16_t)output_sample_temp;

            // 7. Store *current* input sample into history buffer for future echoes
            distortion_history_buffer[distortion_write_index] = current_sample;

            // 8. Advance history buffer write index (circularly)
            distortion_write_index = (distortion_write_index + 1) % distortion_BUFFER_SIZE;
        }
    }
}

void DMA1_Channel1_IRQHandler(void)
{
    uint32_t isr_flags = DMA1->ISR; // Read DMA1 interrupt status register

    // Check for Half Transfer complete interrupt flag for Channel 1
    if ((isr_flags & DMA_ISR_HTIF1) != 0)
    {
        DMA1->IFCR = DMA_IFCR_CHTIF1; // Clear the Half Transfer flag for Channel 1
        process_adc_data(0, BUFFER_SIZE / 2); // Process the first half of the buffer
    }

    // Check for Transfer Complete interrupt flag for Channel 1
    if ((isr_flags & DMA_ISR_TCIF1) != 0)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF1; // Clear the Transfer Complete flag for Channel 1
        process_adc_data(BUFFER_SIZE / 2, BUFFER_SIZE / 2); // Process the second half of the buffer
    }

    // Optional: Check for Transfer Error flag for Channel 1
    if ((isr_flags & DMA_ISR_TEIF1) != 0)
    {
         DMA1->IFCR = DMA_IFCR_CTEIF1; // Clear the Transfer Error flag
         // Handle error, e.g., toggle an LED or print error (use printf cautiously in ISR)
         // Consider setting a global error flag checked in main loop instead
         // printf("!!! ADC DMA Error (Ch1) !!!\r\n");
    }

    // Clear the Global Interrupt flag - Might be redundant if specific flags cleared, but safe practice
    // DMA1->IFCR = DMA_IFCR_CGIF1; // Already cleared by clearing specific flags HTIF1, TCIF1, TEIF1
}
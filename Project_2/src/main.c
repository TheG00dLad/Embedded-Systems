#include <stm32l432xx.h> // Include necessary for direct register access if eeng1030_lib doesn't cover everything
#include <stdint.h>
#include <eeng1030_lib.h> // Assumed to provide pinMode, selectAlternateFunction, initClocks, delay_ms etc.
#include <stdio.h>
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "display.h"    // Header for your SPI display functions

// Function Prototypes
void setup(void);
// delay_ms is likely provided by eeng1030_lib via SysTick setup
void initSerial(uint32_t baudrate);
void eputc(char c);
int _write(int file, char *data, int len); // Prototype for printf redirection
void initEncoder(void);
// initClocks is likely provided by eeng1030_lib or called within setup

// --- Configuration for Percentage Bar ---
#define BAR_X 10          // X position of the bar
#define BAR_Y 50          // Y position of the bar
#define BAR_WIDTH 100     // Total width of the bar in pixels (represents 100%)
#define BAR_HEIGHT 15     // Height of the bar in pixels
#define TEXT_X BAR_X      // X position for percentage text
#define TEXT_Y (BAR_Y - 12) // Y position for percentage text (adjust based on font)

int main()
{
    uint16_t screen_bg_color = RGBToWord(40, 40, 40); // Dark grey background
    uint16_t bar_outline_color = RGBToWord(200, 200, 200); // Light grey outline
    uint16_t bar_fill_color = RGBToWord(0, 255, 0);       // Green fill
    uint16_t bar_empty_color = screen_bg_color;         // Color for the empty part inside the outline
    uint16_t text_color = RGBToWord(255, 255, 255);     // White text

    uint16_t encoder_val;
    uint16_t percentage = 0;
    uint16_t old_percentage = 101; // Force initial draw (value > 100)
    uint16_t filled_width = 0;
    uint16_t old_filled_width = 0;
    char text_buffer[20]; // Buffer for sprintf

    // Initialize core systems first
    setup();

    // Initialize peripherals that depend on core setup
    init_display(); // Initialize the SPI display
    initEncoder();  // Initialize the Timer for encoder input

    // --- Initial Screen Setup ---
    fillRectangle(0, 0, 160, 80, screen_bg_color); // Clear screen with background color

    // Draw the outline for the percentage bar
    drawRectangle(BAR_X - 1, BAR_Y - 1, BAR_WIDTH + 2, BAR_HEIGHT + 2, bar_outline_color);
    // Initial fill of the bar area (empty)
    fillRectangle(BAR_X, BAR_Y, BAR_WIDTH, BAR_HEIGHT, bar_empty_color);

    // Initial text will be drawn in the first loop iteration because old_percentage != percentage

    while (1)
    {
        // Print current encoder count via Serial (USART2) for debugging
        // printf("count %u\r\n", (uint16_t)TIM2->CNT);

        // Read encoder value (0-159)
        encoder_val = TIM2->CNT;

        // Calculate percentage (0-100) using integer math
        // Multiply by 100 *before* dividing to maintain precision
        percentage = (encoder_val * 100) / 159;

        // Ensure percentage doesn't exceed 100 due to potential rounding/timing
        if (percentage > 100) {
            percentage = 100;
        }

        // Calculate the width of the filled part of the bar
        // Since BAR_WIDTH is 100, filled_width is simply the percentage
        filled_width = percentage;
        if (filled_width > BAR_WIDTH) { // Sanity check
            filled_width = BAR_WIDTH;
        }


        // --- Update display only if percentage changed ---
        if (percentage != old_percentage)
        {
            // 1. Update Percentage Text
            //    Erase old text by drawing a background rectangle over it
            fillRectangle(TEXT_X, TEXT_Y, 100, 10, screen_bg_color); // Adjust width/height as needed for text area
            //    Format and print new text
            sprintf(text_buffer, "%3d%%", percentage); // Right-align percentage in 3 spaces
            printText(text_buffer, TEXT_X, TEXT_Y, text_color, screen_bg_color);

            // 2. Update Percentage Bar (Optimized redraw)
            old_filled_width = (old_percentage * BAR_WIDTH) / 100; // Calculate previous width
            if (old_filled_width > BAR_WIDTH) old_filled_width = BAR_WIDTH; // Sanity check


            if (filled_width > old_filled_width) {
                // Bar grew: Fill the new part
                fillRectangle(BAR_X + old_filled_width, BAR_Y, filled_width - old_filled_width, BAR_HEIGHT, bar_fill_color);
            } else if (filled_width < old_filled_width) {
                // Bar shrank: Erase the removed part (draw background color)
                fillRectangle(BAR_X + filled_width, BAR_Y, old_filled_width - filled_width, BAR_HEIGHT, bar_empty_color);
            }
            // If filled_width == old_filled_width, do nothing to the bar graphics

            // Update old values for the next iteration
            old_percentage = percentage;
        }

        // Small delay
        delay_ms(20); // Slightly longer delay can reduce sensitivity/flicker
    }
}

void setup()
{
    // Initialize clock system (assumes eeng1030_lib provides/uses initClocks)
    initClocks(); // Make sure this sets clock to 80MHz as expected by SysTick/Serial

    // Configure SysTick for 1ms interrupts (used by delay_ms)
    SysTick->LOAD = 80000 - 1; // Systick clock = 80MHz. 80000000 / 80000 = 1000 Hz = 1ms
    SysTick->CTRL = 7;         // Enable SysTick counter, IRQ, use processor clock
    SysTick->VAL = 10;         // Start timer
    __enable_irq();            // Enable global interrupts (CMSIS function)
                               // Note: Code Block 2 used __asm(" cpsie i "); which also works

    // Enable clocks for GPIO Ports used
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN; // Use CMSIS defines for clarity

    // Initialize Serial Port (USART2 on PA2) - Optional for debugging
    // initSerial(9600);
}

// Initializes USART2 for Serial Communication (Optional - from Code Block 2)
void initSerial(uint32_t baudrate)
{
    // GPIOA clock is enabled in setup()
    // PA2 is USART2 TX
    pinMode(GPIOA, 2, 2);                // Set PA2 to Alternate function mode
    selectAlternateFunction(GPIOA, 2, 7); // Select AF7 (USART2_TX) for PA2

    // Enable USART2 Clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    // Configure USART2
    const uint32_t CLOCK_SPEED = 80000000; // Assume 80MHz system clock from initClocks()
    uint32_t BaudRateDivisor = CLOCK_SPEED / baudrate;

    USART2->CR1 = 0;                       // Disable USART first
    USART2->BRR = BaudRateDivisor;         // Set baud rate
    USART2->CR1 = USART_CR1_TE;            // Enable Transmitter
    USART2->CR1 |= USART_CR1_UE;           // Enable USART
}

// Retargets _write syscall used by printf to use USART2 (Optional - from Code Block 2)
int _write(int file, char *data, int len)
{
    // If initSerial is commented out, prevent printf from hanging
    #ifdef RCC_APB1ENR1_USART2EN // Check if USART2 clock enable definition exists (crude check if serial might be active)
    if (!(RCC->APB1ENR1 & RCC_APB1ENR1_USART2EN)) return len; // If USART clock not enabled, just return
    #endif

    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    for (int i = 0; i < len; i++)
    {
        eputc(data[i]);
    }
    return len;
}

// Sends a single character via USART2 (Optional - from Code Block 2)
void eputc(char c)
{
    // If initSerial is commented out, prevent hanging
    #ifdef RCC_APB1ENR1_USART2EN
    if (!(RCC->APB1ENR1 & RCC_APB1ENR1_USART2EN)) return;
    #endif

    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = c;
}

// Initializes Timer 2 for Encoder Input (from Code Block 2)
// PA0: TIM2_CH1 (AF1) - Encoder CLK/A
// PB3: TIM2_CH2 (AF1) - Encoder DT/B
// PB4: Optional GPIO Input - Encoder SW (Switch) - configured but not read
void initEncoder()
{
    // GPIOA and GPIOB clocks are enabled in setup()

    // Configure Encoder Pins (PA0 and PB3)
    pinMode(GPIOA, 0, 2);                // PA0 = Alternate function
    selectAlternateFunction(GPIOA, 0, 1); // PA0 = AF1 (TIM2_CH1)
    pinMode(GPIOB, 3, 2);                // PB3 = Alternate function
    selectAlternateFunction(GPIOB, 3, 1); // PB3 = AF1 (TIM2_CH2)

    // Configure Optional Encoder Switch Pin (PB4)
    pinMode(GPIOB, 4, 0); // PB4 = Input mode
    // enablePullUp(GPIOB, 4); // *** Add this line if your encoder switch needs a pull-up resistor ***
                              // Make sure enablePullUp function is available (e.g., from eeng1030_lib)

    // Enable Timer 2 Clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    // Configure Timer 2 in Encoder Mode
    TIM2->CR1 = 0;          // Disable timer while configuring
    TIM2->ARR = 159;        // Set Auto-Reload Register (counter max value)
    TIM2->CNT = 0;          // Start counter at 0
    TIM2->SMCR = TIM_SMCR_SMS_1; // Encoder mode 2: Counter counts up/down on TI2FP2 edge depending on TI1FP1 level
                                 // Use TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1 for Encoder mode 3 (counts both edges) if needed
    TIM2->CCMR1 = TIM_CCMR1_CC1S_0 | // CC1 channel is configured as input, IC1 is mapped on TI1
                  TIM_CCMR1_CC2S_0; // CC2 channel is configured as input, IC2 is mapped on TI2
    // Optional Input Filters (reduce noise sensitivity). Set filter N=15 (max)
    TIM2->CCMR1 |= TIM_CCMR1_IC1F | TIM_CCMR1_IC2F; // Set bits [7:4] and [15:12] to 1111b
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E; // Enable capture/compare channels 1 and 2
    TIM2->CR1 |= TIM_CR1_CEN; // Enable Timer 2
}


// Add SysTick_Handler if required by delay_ms implementation in eeng1030_lib
// Example:
// volatile uint32_t ticks = 0; // Needs to be global or static
// void SysTick_Handler(void) {
//     // This function name must match the vector table definition
//     ticks++;
// }
// // Make sure delay_ms uses the 'ticks' variable
// void delay_ms(volatile uint32_t dly) {
//    uint32_t start_ticks = ticks;
//    while ((ticks - start_ticks) < dly);
// }
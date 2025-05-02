#include <stdint.h>
#include <stm32l432xx.h>
#include <stdbool.h>
#include <string.h>

#define SAMPLE_RATE 22100  // 22kHz sampling rate
#define BUFFER_SIZE 22100  // 0.5-second delay buffer
#define ECHO_DECAY 0.9   // Echo decay factor

// volatile bool recordingDone = false;  // Flag to indicate recording completion
// volatile uint16_t audioBuffer[BUFFER_SIZE]; // Circular buffer for delay
// volatile uint32_t bufferIndex = 0;         // Write index
// volatile uint32_t playbackIndex = 0;
// void setup()
// {
//     initClocks();
//     RCC->AHB2ENR |= (1 << 0) + (1 << 1); // enable GPIOA and GPIOB
//     //pinMode(GPIOB,3,1); // make PB3 an output.
//     //pinMode(GPIOA,0,1);
//     pinMode(GPIOA,0,3);  // PA0 = analog mode (ADC in)
//     pinMode(GPIOA,4,3);  // PA4 = analog mode (DAC out)

//     initADC();
//     initDAC();
// }
// void SysTick_Handler(void)
// {
//     int16_t inputSample = readADC();  // Read input sample

//     if (!recordingDone) {
//         // Store initial segment
//         audioBuffer[bufferIndex] = inputSample;
//         bufferIndex++;

//         if (bufferIndex >= BUFFER_SIZE) {
//             recordingDone = true;  // Stop recording after filling buffer
//             bufferIndex = 0;       // Reset to loop playback
//             GPIOB->ODR |= (1 << 3); // Debug LED ON when loop starts
//         }
//     }

//     int16_t loopSample = 0;
//     if (recordingDone) {
//         loopSample = audioBuffer[bufferIndex];  // Play stored sample
//         bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;  // Loop playback
//     }
//     // Mix live input (noise) with looped audio
//     int16_t outputSample = (loopSample * 0.3) + (inputSample * 0.6);  // Boost loop

//     writeDAC(outputSample);  // Output processed sound
// }
void delay(volatile uint32_t dly)
{
    while(dly--);
}


// void initDAC()
// {

//     RCC->APB1ENR1 |= (1 << 29);   // Enable the DAC
//     RCC->APB1RSTR1 &= ~(1 << 29); // Take DAC out of reset
//     DAC->CR &= ~(1 << 0);         // Enable = 0
//     DAC->CR |= (1 << 0);          // Enable = 1
    
// }



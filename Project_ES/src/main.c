// This example uses the SysTick timer to control the sample rate for an analog-passthrough system.
// The sample rate is set to 44100 Hz.  At this speed it is not possible to keep re-enabling the ADC and
// setting the channel number in the SysTick interrupt handler.  These actions take longer than the 
// sampling period.  The alternative approach is to configure and enable the ADC in the initADC function.
// The interrupt handler operates by starting a conversion at one interrupt event and then reading the
// result at the next event.  Enough time will have passed between interrupts (about 22us) for the conversion 
// to finish (about 5 microseconds) so it will be safe to read the ADC result


//The First Void systick is the basic echo that follows the input.
// The Second Void Systick will create a loop of noise that repeats while allowing music to pass through.
#include <stdbool.h>
#include <eeng1030_lib.h>

void setup(void);
void delay(volatile uint32_t dly);
void initADC();
int readADC();
void initDAC();
void writeDAC(int value);

#define SAMPLE_RATE 20500  // 20.5kHz sampling rate (Half audio quality)
#define BUFFER_SIZE 20500  // 1-second delay buffer
#define ECHO_DECAY 0.5     // Echo decay factor 0-1

volatile bool recordingDone = false;  // Flag to indicate recording completion
volatile uint16_t audioBuffer[BUFFER_SIZE]; // Circular buffer for delay
volatile uint32_t bufferIndex = 0;         // Write index

int main()
{
    setup();
    SysTick->LOAD = 1814-1; // Systick clock = 80MHz. 80000000/44100 = 1814
	SysTick->CTRL = 7; // enable systick counter and its interrupts
	SysTick->VAL = 10; // start from a low number so we don't wait for ages for first interrupt
	__asm(" cpsie i "); // enable interrupts globally
    while(1)
    {
    
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
    //pinMode(GPIOB,3,1); // make PB3 an output.
    //pinMode(GPIOA,0,1);
    pinMode(GPIOA,0,3);  // PA3 = analog mode (ADC in)
    pinMode(GPIOA,4,3);  // PA4 = analog mode (DAC out)

    initADC();
    initDAC();
}
/*void SysTick_Handler(void)
{
    GPIOB->ODR ^= (1 << 3);
   DAC->DHR12R1 = ADC1->DR; // get the result from the previous conversion    
   ADC1->ISR = (1 << 3); // clear EOS flag
   ADC1->CR |= (1 << 2); // start next conversion 
   GPIOB->ODR &= ~(1 << 3); // toggle PB3 for timing measurement
//---------------------------------------------------------------------------------------
    int16_t inputSample = readADC();   // Read input
    int16_t delayedSample = audioBuffer[bufferIndex]; // Get delayed sample
    int16_t outputSample = inputSample + (delayedSample * ECHO_DECAY); // Mix

  audioBuffer[bufferIndex] = inputSample; // Store current sample
  bufferIndex = (bufferIndex + 1) % BUFFER_SIZE; // Circular buffer wrap

  writeDAC(outputSample); // Output processed signal

//-----------------------------------------------------------------------------------------
if (!recordingDone)  
    {
        // RECORD PHASE: Fill buffer until full
        audioBuffer[bufferIndex] = readADC();
        bufferIndex++;

        if (bufferIndex >= BUFFER_SIZE)
        {
            bufferIndex = 0;  // Reset index for playback
            recordingDone = true; // Stop recording
        }
    }
    else
    {
        // PLAYBACK PHASE: Output stored samples in a loop
        writeDAC(audioBuffer[bufferIndex]); // Output recorded sample
        bufferIndex = (bufferIndex + 1) % BUFFER_SIZE; // Loop playback
    }
}*/

void SysTick_Handler(void)
{
    int16_t inputSample = readADC();  // Read input sample

    if (!recordingDone) {
        // Store initial segment
        audioBuffer[bufferIndex] = inputSample;
        bufferIndex++;

        if (bufferIndex >= BUFFER_SIZE) {
            recordingDone = true;  // Stop recording after filling buffer
            bufferIndex = 0;       // Reset to loop playback
            GPIOB->ODR |= (1 << 3); // Debug LED ON when loop starts
        }
    }

    int16_t loopSample = 0;
    if (recordingDone) {
        loopSample = audioBuffer[bufferIndex];  // Play stored sample
        bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;  // Loop playback
    }
    // Mix live input (noise) with looped audio
    int16_t outputSample = (loopSample * 0.3) + (inputSample * 0.3);  // Boost loop

    writeDAC(outputSample);  // Output processed sound
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
    ADC1_COMMON->CCR |= (0x00 << 18); // Needed to change it from 0x0f -> 0x00 for the ADC to keep up
    ADC1->SQR1 |= (5 << 6);
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
    if (value < 0) value = 0;       // Clip negative values
    if (value > 4095) value = 4095; // Clip over-range values
    DAC->DHR12R1 = value;           // Output to DAC
}


## Project On Building an Echo Effect For An Audio Input

The project uses the operation of an ADC and DAC, and runs the audio through an echo effect process.
The effects either cause a 1 second echo that follows the input, or else repeats a second behind continuously.

Helper functions have been provided that handle the configuration of the pin modes (`pinMode()`), initialisation fo the ADC (`initADC()`), and reading of the ADC (`readADC()`).
# Introduction
The goal of the project is to implement a digital audio effect processor for audio effects commonly used by musicians (e.g. echo).
Echos are used commonly as a way to play back audio after a short delay, typically starting from 660ms to around 1500ms as technology developed.
"Brighton Rock" by Queen is a good example of the echo effect.
Another form of the echo effect is to loop the echo as a backing track, such as a loop pedal would do.
Both use the same buffer, and can be switched and implemented easily.



Buffer will cycle, the data similar to a cicular buffer.
![ES_Project1](https://github.com/user-attachments/assets/ee9ed631-32da-479b-9f7b-7a8448bb8877)
# Theory
- Circuit Design
  Circuit has a basic DC positive bias to account for the STM L432 only reading positive voltage.
  This is inputted into PA0.
  All signals must be >0V & <3.3V.
  The capacititor to PA4 reduces voltage flutuations.
  There is an RC filter at the end of the output.
  Audio from here will be filtered and clearer.
- Buffer
  A buffer is the temproary storage area that will store the data needed for the echo.
  It can be used in many electronics, primarily to move temporary data from one place to another. 
- ADC
  An Analog-to-Digital Convertor (ADC) is a system that converts signals from the "real" world, i.e sound, into a digital output that a computer can use.
- DAC
  A Digital-to-Analog Convertor (DAC) is a system that converts digital computer data into a analog, "real" signal such as sound.
- Interrupts
  An interrupt is a signal to the processor. It can be emitted through software, indicating that an event needs immediate attention.
  It can halt current normal tasks if a task is needing attention.
  The code uses the systick handler, which is a Interrupt Service Routine (ISR).
  The ISR is what happens when the interrupt is toggled.
  The SysTick in the code is toggled, it starts the reading of the ADC.
- SysTick
  A SysTick is a sytem timer which counts down from a set value to generate periodic interrupts at precise time intervals.
  It starts at a pre-determined value and counts down to 0.
  Then it generates the interupt.
  It reads and outputs from the ADC and DAC respectively at this frequency.
  The SysTick controls when the ADC and DAc are accessed, making it more efficient on the CPU.
- Low Pass Filter
  A low pass filter or RC filter allows for lower frequencys to pass through but high frequency noise to be filtered out.

# Debugging
Debugging was done via 2 ways. The first was creating a breakpoint in the "readADC" function.
This verified that there was a value being read consistently from the SysTick function.
The second debugging was done via hardware on an osilloscope.
A function was put in to the ADC.
The original output was should be a sine wave.
However, the ADC prescaler caused aliasing.
This left poor resolution outputting from the DAC.

Lowering the prescaler fixed this issue.
Outputted was a sine wave which had some quantisiation, but at a acceptable frequency.

Using the Echo function in the systick then created a sinewave, but playing them together meant the output was two waveforms summer together, which was visible.
Results

Conclusion
# Primary Goal
1. To create an echo to follow the music in as a digital echo, similar to old tape echos.
2. Make an echo effect sampled from the start to act as a background to the music currently played.
3. Gain better understanding of ADC and DAC in C

Approach;
1. Using a function generator, determine the resolution of the ADC to DAC.
   - Here arose an issue, that the ADC started aliasing.
   - This was due to the ADC prescaler.
   - Moving the prescaler from 0x0f to 0x00 meant that the prescaler went from the maximum possible division of the system clock to no division.
   - This meant it could read from 312kHz to 80 MHz
   - This was verified through a ollsiloscope.
2. Create a buffer to log the the audio in, and save.
   - This meant that audio can be stored in the buffer.
   - The amount of audio is the sampling frequency (fs) times samples.
   - Each sample is 2 bytes per sample.
   - For 1 second of audio, fs = buffer size.
   - Due to the chip having access to a limited amount of RAM, using a fs of 20500 Hz (half typical audio quality), times the buffer (20500).
   - The total memory used was 41,000 bytes in one moment.
3. Verified & Debugged Using Osillscope.
   - A waveform which was the sum of 2 inputs was outputted from ADC.
   - This verified the Echo effects.
   - Testing by audio was then used to confirm effects worked as intended.
4. 

# Conclusion

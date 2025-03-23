# Project On Building an Echo Effect For An Audio Input
## Table of Contents

[Theory](#Theory)

[Introduction](#Introduction)

[Debugging](#Debugging)

[Approach](#Approach)

[Conclusion](#Conclusion)

## Introduction
The goal of the project is to implement a digital audio effect processor for audio effects commonly used by musicians (e.g. echo).
The project uses the operation of an ADC and DAC, and runs the audio through an echo effect process.
The effects either cause a 1 second echo that follows the input, or else repeats a second behind continuously.
Echos are used commonly as a way to play back audio after a short delay, typically starting from 660ms to around 1500ms as technology developed.
"Brighton Rock" by Queen is a good example of the echo effect.
Another form of the echo effect is to loop the echo as a backing track, such as a loop pedal would do.
Both use the same buffer, and can be switched and implemented easily.
![AAEMS](https://github.com/user-attachments/assets/c7966a21-ff5e-45db-ad80-502a210cd912)

Pictured is an original mechanical echo, which has 2 tape heads.
One reads and one writes.
This can be reproduced with the ADC and DAC.
A small echo machine would be able to echo 66ms to 150 ms.
The STM L432 will be used to simiulate this mechanical device.


### Primary Goal
1. To create an echo to follow the music in as a digital echo, similar to old tape echos.
2. Make an echo effect sampled from the start to act as a background to the music currently played.
3. Gain better understanding of ADC and DAC in C.

![ES_Project1](https://github.com/user-attachments/assets/ee9ed631-32da-479b-9f7b-7a8448bb8877)

![425788302-99fc52a4-e186-4c1d-90fc-ae8e527c847b](https://github.com/user-attachments/assets/c72fc3ee-75f1-4be4-a186-5d4d67339cf4)

## Theory
- Circuit Design
  
    Circuit has a basic DC positive bias to account for the STM L432 only reading positive voltage.
    This is inputted into PA0.
    All signals must be >0V & <3.3V.
    The capacititor to PA4 reduces voltage flutuations.
    There is an RC filter at the end of the output.
    Audio from here will be filtered and clearer.
    PA0 & PA4 were decided due to their ADC & DAC capabilities.
  
- Buffer
  
    A buffer is the temproary storage area that will store the data needed for the echo.
    It can be used in many electronics, primarily to move temporary data from one place to another.
    The buffer will cycle, the data similar to a cicular buffer, filling and emptying the buffer with data like the old tape echo.
  
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

## Debugging & Sources of Error
Debugging was done via 2 ways. The first was creating a breakpoint in the "readADC" & "writeDAC" function.
This verified that there was a value being read and writen consistently from the SysTick function.
Therefore it can be reasoned that the system will work as intended, taking in and putting out samples.

The second debugging was done via hardware on an osilloscope.
A function was put in to the ADC.
The original output was should be a sine wave.
However, the ADC prescaler caused aliasing.
This left poor resolution outputting from the DAC.

![Aliasing](https://github.com/user-attachments/assets/fb6225cd-d480-4340-8051-4880abc12bc8)

The output frequency was close to 14kHz, which is not near the desired audio quality 44.1kHz

![ADC good res](https://github.com/user-attachments/assets/761d9934-ebab-4d6e-8e1b-d9ab3ac7db72)

Lowering the prescaler fixed this issue.
Outputted was a sine wave which had some quantisiation, but at an acceptable frequency of 44.1kHz.

Using the Echo function in the systick then created a sinewave, but playing them together meant the output was two waveforms summer together, which was visible.
The system can now be confirmed as inputing and outputting the as intended.

![Echo 2](https://github.com/user-attachments/assets/770f1356-a0af-45c2-b403-23c5151726c8)
![Echo 1](https://github.com/user-attachments/assets/b0eaf05b-08f6-4ab9-9e52-c61080af572a)

Final source of error was the uploading of the wrong enviroment to the microprocessor.
This was an oversight and is easily avoid through correct uploading protocol.

## Code Notes
Helper functions have been provided that handle the configuration of the pin modes (`pinMode()`), initialisation for the ADC (`initADC()`), and reading of the ADC (`readADC()`).
Two systick functions exist in the code, with one commented out.
Both will work but only one should be used at one time.
This first ('SysTick()') will create an echo that follows the input.
This will be the "echo pedal".
The second ('SysTick()') will create an echo that will become quieter in the background to act as the backing track of the inputs.
This will be the "loop pedal".

## Approach;
1. Using a function generator, determine the resolution of the ADC to DAC.
   - Here arose an issue, that the ADC started aliasing.
   - This was due to the ADC prescaler.
   - Moving the prescaler from 0x0f to 0x00 meant that the prescaler went from the maximum possible division of the system clock to no division.
   - This meant it could read theoretically from 312kHz to 80 MHz
   - This was verified through a ollsiloscope.
   - However, in reality as seen in the above pictures, it was reading at around 44.1kHz, which corresponds to the systick speed.
2. Create a buffer to log the the audio in, and save.
   - This meant that audio can be stored in the buffer.
   - The amount of audio is the sampling frequency (fs) times samples.
   - Each sample is 2 bytes per sample.
   - For 1 second of audio, fs = buffer size.
   - Due to the chip having access to a limited amount of RAM, using a fs of 22500 Hz (half typical audio quality), times the buffer (22500).
   - The total memory used was over 45,000 bytes in one moment.
3. Verified & Debugged Using Osillscope.
   - A waveform which was the sum of 2 inputs was outputted from ADC.
   - This verified the Echo effects.
   - Testing by audio was then used to confirm effects worked as intended.
4. Physical Testing Of The Effects
   - As the device was an echo pedal, it could be plugged into a audio source, such as a guitar amp.
   - Outputting the cable into a speaker with a 3.5mm jack input allowed for the effects to be tested for their purpose.
   - Echo delay of 1 second worked as intended.
   - Echo loop playback worked too.
   - There is feedback from poor connections, and capacitor discharge, however, they can be filtered using better filters.

## Conclusion
The project provided techniques available through use of the buffer, ADC, DAC and interrupts.
The main constraint on this project was the onboard RAM.
The limited RAM led to the audio quality outputted from the DAC to not be good, at 22.5kHz comparered to the standard 44.1kHz.
Another issues to arise was the noise interfering on the output, but a simple RC filter can avoid that.
Adding a phyiscal button or a rotary encoder would be a fun improvement and add to the ability to change variables while playing music, rather than changing code.
The project was a success with two different effects being able to be obtained using the same buffer and techniques.
There are many further improvements that could be implemented, from user friendlyness to filters implemented digitally in the microcontroller.

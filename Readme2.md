# Specialist Embedded Systems Project : Guitar Effects Pedal With Controller, DMA, Rotary Encoder and LCD
## Introduction
The goal of this project was to improve upon the shortcomings of the previous guitar pedal design. 
The main issues to arise during the previous pedal was sampling frequency, as it was too slow for any real sound quality, only producing at about 30Khz, at max speed, which was due to the systick.
The second issue to arise was the lack of functionality in the previous code, with any effects being manually coded in, and no live adjustments possible.
The goal of the project was to design a LCD that would act as the user interface, such as a volume bar, so that any settings were visible on the breadboard unit.
To edit it with live adjustments back and forward, in addition to changing modes, a rotary encoder was used.
Finally, to get better speeds, and bring up the standard of the audio, use of the DMA abilities in the  STM43 L432kc were crucial in the project. 
Due to the pure direct signalling of DMA, double buffering had to be used as a way to alter the live value.
This was not an inbuilt feature of the l432 microprocessor, but using a buffer as a way to store and edit memory with half-transfers allowed it to be possible.
The L432kc was a good choice of controller due to the high-speed processing power.
With a 12-bit ADC and DAC, and access to 2 DMA's, with 14 combined channels, it can be effective for designing some audio effects.
Adding a rotary encoder and a lcd to the project were brought about due to the previous project's lack of a UI, and therefore they improved the new systems usefulness.

## Schematic
![image](https://github.com/user-attachments/assets/ee81a6e5-8f30-4811-96ab-82b986933808)

This schematic was the basis of my design which allowed for constant referencing through the design stage.
This was pivotal to the system design.

## LCD 
A Liquid Crystal Display (LCD) is a flat-panel display.
It uses liquid crystals that modulate light to create images and is common in TV's.
The LCD uses a backlight to illuminate the crystals.
They are very thing and can be applied wildly and cheaply.

To create colours on the LCD, shapes of various rectangles were layered on top of each other.
Adding a small rectangle starting from the left and making it "grow" to the right will create a loading bar.
To add colour to the text or shape used, RGB codes of the colour were used. 
Careful consideration, as well as trial and error can be used to match colours.
Using a RGB code reference for colours can be a useful tool, and easily called with (XXX,YYY,ZZZ).

![LCD-Screen-Technology](https://github.com/user-attachments/assets/671eaa26-7662-4b65-bfd7-393951c3a662)

The current model used was the ST7735S, which has a resolution of 160x80, and a SPI interface, using SPI MOSI and SPI SCLK.
## Rotary Encoder
A rotary encoder is commonly found on car radios.
It provides a very simple and intuitive left-right directions, as well as a tertiary function on a button press.
It is an electromechanical device that rotates the rotational motion of a shaft into a clock signal.
Matching the signal against a reference clock, it will be capable of providing information of position, speed and direction.
This project will utilize the encoder's direction and position information, which can set parameters.
While speed can be derived, it was deemed unnecessary due to the lack of fine sensitivity in the settings.
The rotary encoder, as it uses TIM2, is therefore not using any CPU polling, which allows for greater efficiency.
## DMA
Direct Memory Access (DMA) is a feature in microcontrollers or computers that allows either peripherals, or memory, to transfer to another place without any CPU involvement.
On the L432, there is 4 types, Peripheral to Memory, Memory to Peripheral, Memory to Memory, and Peripheral to Peripheral.
It is based on the speed of a separate clock rather than a system like the Systick, which is dependent on the CPU.
Hence, if there is a code in the main loop, the systick can slow down.
Therefore, when adding an effect, it can actually slow down the processing, and hence outputting of the signal.
With audio, which should be at 44.1KHz, high speeds are necessary.
Testing was done with DMA first to determine if DMA was truly greater than a very fast systick.
A systick code was set up, compared to a peripheral-to-peripheral DMA setup.

![TEK00000](https://github.com/user-attachments/assets/b410cd4c-deb4-40e8-9997-0a611acadd9f)
![TEK00001](https://github.com/user-attachments/assets/16c276cc-c6c4-429a-a47f-7e4ab50002cb)
![TEK00002](https://github.com/user-attachments/assets/1ca19286-4941-4b1f-8f07-088e126f9bb8)

Image 1 was the default systick, sampling at a supposed 44.1KHz. 
The second image was when the systick passthrough was set to 400, meaning it ran at 200KHz.
The third image is the peripheral-to-peripheral DMA, which attained a greater resolution.
As visible on the oscilloscope readings, there is a far greater resolution on DMA versus systick, even when the systick was set to a value of 400.
Calling DMA is a large task.
To call it, a lot of use of the reference manual for the STM32L43XXX Arm-based 32 MCUs was needed.
Chapter 11 is dedicated to DMA.
![image](https://github.com/user-attachments/assets/92524a20-0b7f-43c3-9426-117782597d2a)


DMA works by;
1. The DMA configures at its channel level, composed of a sequence of AHB bus transfers.
2. To transfer, a peripheral sends a single DMA request to the DMA controller.
This controller serves the request.
However, there is channel priority associated with this request, hence it is served with a priority.
The priority is channel 1 to channel 7, on both DMA1 and DMA2.
3. When the DMA controller grants the peripheral, it serves an acknowledgement from the peripheral by the DMA controller.
4. The peripheral releases the request as when it gets this acknowledgment.
5. Then as the request is de-asserted by the peripheral, the DMA controller releases the acknowledgement.

A single data read can by defined as a byte, word or half-word
While there are 4 modes for DMA, which is MEM2MEM, MEM2PER, PER2MEM & PER2PER.
There is a circular mode, which is able to hand circular buffers and continuous data flows, such as sound or sinewaves.
However, the final issue is the need to access the buffer at any instance, so that the effect can be altered, such as volume.
This led to the need of double buffering.

## Double Buffering
Double buffering is commonly used in computer graphics, seen in technology such as VSync on videogames to smooth screen tearing.
On monitors it will synchronise the framerate with the displays refresh rate, which is typically 144Hz.
For two buffers, a front buffer is displayed on the screen, and the second buffer is used to render the next frame.
On the code, we use half the buffer to render the ADC in, and the other half to apply an effect to the output, which gets loaded back on to the DAC.
Expensive models of microcontrollers have double buffering enabled.

For the STM32L432kc there is no double buffering, and a faux version of the double buffering referred to as "ping-pong" buffering was used.
It uses the buffers halves enabled by DMA interrupts.
The goal is seamless audio flow.
To do this, the ADC needs to be triggered at a fixed rate, set by timer 15.
Then timer 7 is set to this same rate, and it triggers the DAC continuously.
There is also time needed to process the data, which has to be done on the CPU.
Trying to process data from the same memory buffer the ADC is writing to, and the DAC is reading lead to the DMA overwriting the data before anything could happen.
The code separates the buffer, treating it as two halves of the buffers, the adc_buffer and the dac_buffer.
The DMA and CPU can work alternately then.
The process works by the DMA filling the first half, and then triggers a Half Transfer Interrupt, which is triggered by the buffer being half filled.
![image](https://github.com/user-attachments/assets/a4cad1db-c646-4e20-9043-4d5723aacf26)
The CPU can read the stable first half, and process it into the second half of the buffer. 
The Transfer Complete Interrupt (TCIF1) fires, and fills the rest of the buffer.
At the same time the DMA is able to send out the first half through the DAC.
The ADC then wraps around the process repeats.
Hence it ping-pongs.

## USART2
The use of USART2 was crucial to the production of the project, as it allowed for debugging using printf. This allowed for the development of the decoder, as it could show if the button was pressed, or if it was counting up and down.
It also read the DAC and ADC, and could display any features yet to be implemented.
It substituted the LCD display as a way of measuring position.

## Timers
The timers on the ADC and DAC, using DMA, allowed for it to sample data far faster; by setting the timer to 800Khz, it was able to accurately visualise a sine wave at above 100KHz. 
There was an issue arising with TIM2, as the LCD and systick were both reliant on timer 2 originally, however moving the systick to be use the milliseconds changed this.
However, there was interference, but it was above the audible range, as it was at around 1 MHz.
This is quite possible just switching noise or artifacts left on the scope due to the high frequency components.
It should be noted the ADC and DAC may not be able to sample at that speed, it was simply the fastest speed attained through use of the oscilloscope to see how accurate and how fast I could read a signal.


## Testing
The circuit was able to be tested under different environments.
By testing a sinewave at 3.3V, and raising the volume, measure, it could go from no output to a full repeat of the wave, and was visible on the oscilloscope.
The below image demonstrates the volume attenuation.
Here the yellow is the input, and blue is the output.
![image](https://github.com/user-attachments/assets/e2efdbb6-5527-457a-8a6b-44bad039b912)

To test the distortion effect, it was given the same treatment, however the distortion led to clipping as it started to try sum sinewaves.
There was a problem that without a variable that stated "if (audio > 4096); audio = 4096".
This stopped the DAC outputting 0 when it went too high on the input.
The passthrough was testing though simple visually inspecting the oscilloscope.
Below shows the waveform flattening out, as well as the slightly larger waveform compared to a simple volume control passthrough, with the blue waveform as the input, and yellow as the output.

![image](https://github.com/user-attachments/assets/b2449089-24c6-44ba-938c-036850e729fd)


## Debugging
There we some notable issues in the debugging process. An interrupt in the delay_ms combined with the DMA caused the code to crash inside the delay.
This would happen in the while loop, as the asm("wfi") interrupt was never triggered. 
As a result, it crashed. 
By commenting out the place it got stuck, the code worked.
It could be assumed that it was due to the clock speed of the interrupts, and the DMA system not being quite on the same stage, as it would only happen on varying times after using the decoder too fast, causing it to update quickly.
![image](https://github.com/user-attachments/assets/c8af4fc1-b4f8-4dbd-b5ba-0570fadda8e4)

Another issue to arise was when switching modes, some modes either would not call or would switch, and do nothing.
With 3 modes, 0,1 and 2, it should ideally be able to switch easily.
However, the code would not switch into mode 2 after switching from mode 0 to 1.
A breakpoint was put into the mode 2, which then ran.
It happened to be that the mode switching got commented out.
This made for a simple use of breakpoint, and demonstrated its usefulness.

The final major bug found in the project was when the DMA was outputting what looked to be "the average part of a waveform".
It demonstrated clear interference with any waveform, and had this unique jumping to a certain level of the output displayed on the oscilloscope.
![image](https://github.com/user-attachments/assets/83a753c2-a2ca-4df3-a3f2-297ad5673c4a)

This image is what happened when a triangle wave was put through.
Changing the size of the buffer made the distance between the lines more separate in the oscilloscope.
This was due to the use of a float to remember the position of the rotary encoder.

![image](https://github.com/user-attachments/assets/40e85352-d6ac-41e8-924f-d1980def1e6a)

This image showed the same effect upon a sinewave, but with a larger buffer size.
Floating point operations would be called constantly in the loop, required computational power which slowed down the output greater.
Switching the value of the volume multiplier to use fixed point arithmetic provided a far greater performance boost.
This was then able to be used for the distortion effect too, as the encoder could easily set a value without causing any further errors.

## Demonstration

![TEK00000](https://github.com/user-attachments/assets/ff042cc7-0b89-43c7-9abf-ee9513e5d677)

![TEK00001](https://github.com/user-attachments/assets/8c51c586-f085-4280-b151-a8c4d43b9edf)

These images are proof of the output power of the DMA system.
The interference seen at jumping around the sine wave is miniscule compared to the original systick.
It can be calculated to be at around 1MHz, which is beyond audible.
Humans can only hear up to around 20KHz, hence why the sampling rate of audio on CDs is 44.1KHz, to avoid any aliasing.

![image](https://github.com/user-attachments/assets/6e8e0d11-a798-4561-b031-1fa5436b1285)

This result was the final version of the code, able to accurately reproduce waveforms at a speed of almost 100KHz.
# Ethical Considerations
A strong consideration for the product was reliability, as products should be able to work reliably with any consumer without the consumer needing to know about software debugging.
Another consideration was the ease of use, hence the three uses of the controller.
Having a distortion effect, a volume control and a passthrough all controllable by the encoder's single button make it easy for any potential user to figure out easily.
Future work would involve a simple debouncing program, as that was the only main issue at the time of finishing was that sometimes the encoder could debounce, even with a large delay.
# Conclusion
The project suffers from audible interference.
More than likely, it is due to the electrical tape on the audio cables, as proper shrink wrap or other forms of insulation with little to no movement would create less interference.
This project demonstrates the effective application of advanced microcontroller peripherals for real-time audio processing. 
The core achievement lies in the successful use of the STM32L432KC's DMA controller, synchronized by hardware timers (TIM15, TIM7), to manage ADC and DAC data flow efficiently. 
Implementing ping-pong buffering via DMA interrupts was essential for enabling concurrent data transfer and CPU-based effects processing (Volume, Distortion).
Fixed-point arithmetic was adopted to optimize performance critical calculations. 
User interaction was significantly enhanced through a rotary encoder, decoded using TIM2's hardware capabilities, and an SPI LCD providing visual feedback. 
Despite challenges in debugging and residual noise likely related to external factors, the system successfully meets the primary goals of increased sampling performance and real-time effect control.

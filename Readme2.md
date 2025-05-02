README 2

- Introduction
The goal of this project was to improve upon the shortcomings of the previous guitar pedal design. 
The main issues to arise during the previous pedal was sampling frequency, as it was too slow for any real sound quality, only producing at about 30Khz, at max speed, which was due to the systick.
The second issue to arise was the lack of functionality in the previous code, with any effects being manually coded in, and no live adjustments possible.
The goal of the project was to design a LCD that would act as the user interface, such as a volume bar, so that any settings were visibile on the breadboard unit.
To edit it with live adjustments back and foward, in addition to changing modes, a rotary encoder was used.
Finally to get better speeds, and bring up the standard of the audio, use of the DMA abilities in the  STM43 L432kc were crucial in the project. 
Due to the pure direct signalling of DMA, double buffering had to be used as a way to alter the live value.
This was not an inbuilt feature of the l432 microprocesser, but using a buffer as a way to store and edit memory with half-transfers allowed it to be possible.

# LCD 
An Licquid Crystal Display (LCD) is a flat-panel display.
It uses liquid crystals that modulate light to create images and is common in TV's.
The LCD uses a backlight to illuminate the crystals.
They are very thing and can be applied wildly and cheaply.

To create colours on the LCD, shapes of various rectangles were layered on top of each other.
Adding a small rectangle starting from the left and making it "grow" to the right will create a loading bar.
To add colour to the text or shape used, RGB codes of the colour were used. 
Careful consideration, aswell as trial and error can be used to match colours.
Using a RGB code reference for colours can be a useful tool, and easily called with (XXX,YYY,ZZZ).
![LCD-Screen-Technology](https://github.com/user-attachments/assets/671eaa26-7662-4b65-bfd7-393951c3a662)

The current model used was the ST7735S, which has a resolution of 160x80, and a SPI interface, using SPI MOSI and SPI SCLK.
# Rotary Encoder
A rotary encoder is commonly found on car radios.
It provides a very simple and intuitive left-right directions, as well as a tertiary function on a button press.
It is an electromechanical device that rotates the rotational motion of a shaft into a clock signal.
Matching the signal against a reference clock, it will be capable of providing information of position, speed and direction.
This project will only use position and direction, as the sensitivity is not as fine as it would need to be to desire the use of speed. CHECK THIS PART BRO.
The rotary encoder, as it uses TIM2, is therefore not using any CPU polling, which allows for greater effiency.
# DMA
Direct Memory Access (DMA) is a feature in microcontrollers or computers that allows either peripherals, or memory, to transfer to another place without any CPU involvement.
On the L432, there is 4 types, Peripheral to Memory, Memory to Peripheral, Memory to Memory, and Peripheral to Peripheral.
It is based on the speed of a seperate clock rather than a system like the Systick, which is dependant on the CPU.
Hence, if there is a code in the main loop, the systick can slow down.
Therefore when adding an effect, it can actually slow down the processing, and hence outputting of the signal.
With audio, which should be at 44.1KHz, high speeds are necessary.
Testing was done with DMA first to determine if DMA was truly greater than a very fast systick.
A systick code was set up, compared to a peripheral to peripheral DMA setup.
[INSERT IMAGE OF SYSTICK]
[INSERT IMAGE OF DMA P2P]
As visibile on the oscilloscope readings, there is a far greater resolution on DMA versus systick, even when the systick was set to a value of 400.
Calling DMA is a large task.
To call it, alot of use of the reference manual for the STM32L43XXX Arm-based 32 MCUs was needed.
Chapter 11 is dedicated to DMA.
![image](https://github.com/user-attachments/assets/92524a20-0b7f-43c3-9426-117782597d2a)
DMA works by;
1. The DMA is configures at it's channel level, composed of a sequence of AHB bus transfers.
2. To transfer, a peripheral sends a single DMA request to the DMA controller.
This controller serves the request.
However, there is channel priority associated with this request, hence it is served with a priority.
The priority is channel 1 to channel 7, on both DMA1 and DMA2.
3. When the DMA controller grants the peripheral, it serves an acknowledgement from the peripheral by the DMA controller.
4. The peripheral releases the request as when it gets this acknowledgment.
5. Then as the request is de-asserted by the peripheral, the DMA controller releases the acknowldgement.

A single data read can by defined as a byte, word or half-word
While there is 4 modes for DMA, which is MEM2MEM, MEM2PER, PER2MEM & PER2PER.
There is a circular mode, which is able to hand circular buffers and continuous data flows, such as sound or sinewaves.
However, the final issue is the need to access the buffer at any instance, so that the effect can be altered, such as volume.
This led to the need of double buffering.

- Double Buffering
Double buffering is commonly used in computer graphics, seen in technology such as VSync on videogames to smooth screen tearing.
On monitors it will synchronise the framerate with the displays refresh rate, which is typically 144Hz.
For two buffers, a front buffer is displayed on the screen, and the second buffer is used to render the next frame.
On the code, we use half the buffer to render the ADC in, and the other half to apply an effect to the output, which gets loaded back on to the DAC.
Expensive models of microcontrollers have double buffering enabled.

For the STM32L432kc there is no double buffering, and a faux verision of the double buffering refered to as "ping-pong" buffering was used.
It uses the buffers halves enabled by DMA interrupts [insert dma interrupts part]
The goal is seamless audio flow.
To do this, the ADC needs to be triggered at a fixed rate, set by timer 15.
Then timer 7 is set to this same rate, and it triggers the DAC continuously.
There is also time needed to process the data, which has to be done on the CPU.
Trying to process data from the same memory buffer the ADC is writing to and the DAC is reading lead to the DMA overwriting the data before anything could happen.
The code seperates the buffer, treating it as two halves, the adc_buffer, and the dac_buffer.
The DMA and CPU can work alternately then.
The process works by the DMA filling the first half, and then triggers a Half Transfer Interupt, which is triggered by the buffer being half filled.
![image](https://github.com/user-attachments/assets/a4cad1db-c646-4e20-9043-4d5723aacf26)
The CPU can read the stable first half, and process it into the second half of the buffer. 
The Transfer Complete Interrupt (TCIF1) fires, and fills the rest of the buffer.
At the same time the DMA is able to send out the first half through the DAC.
The ADC then wraps around the the process repeats.
Hence it ping-pongs.

-USART2
The use of USART2 was crucial to the production of the project, as it allowed for debugging using printf. This allowed for the development of the decoder, as it could show if the button was pressed, or if it was counting up and down.
It also read the DAC and ADC, and could display any features yet to be implemented.
It substituted the LCD display as a way of measuring position.

- Timers
The timers on the ADC and DAC, using DMA, allowed for it to sample data far faster, by setting the timer to 800Khz, it was able to accurately visualise a sine wave at above 100KHz. However there was interference, but it was above the audible range, as it was at around 1 MHz.

- Schematic

- Testing
The circuit was able to be tested under different enviroments
-Problems

- Demonstration
- Conclusion
The project suffers from audible interference.
More than likely it is due to the electrical tape on the audio cables, as proper shrink wrap or other forms of insulation with little to no movement would create less interfernce.

Issues
Pins needed to be set - diagram
Getting a vairable as set by the encoder - counter to tim2
"Wipping" of LCD display as encoder setting was changed.
Issue with the systick timer and the LCD timer being called at the same time. - Switched the LCD timer




    // EXTI->FTSR1 |= (1 << 4); // select falling edge trigger for PB4 input
    // EXTI->IMR1 |= (1 << 4);  // enable PB4 interrupt
    // NVIC->ISER[0] |= (1 << 10); // IRQ 10 maps to EXTI4

using the EXTI on PB4 causes a polling issue and shutdown the fprint when working on the encoder.

Why is that?

E_S image 1 error appeared, why is that?

DMA - Is timer 7 counting? - Check breakpoint.

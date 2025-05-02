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

- LCD 
An Licquid Crystal Display (LCD) is a flat-panel display.
It uses liquid crystals that modulate light to create images and is common in TV's.
The LCD uses a backlight to illuminate the crystals.
They are very thing and can be applied wildly and cheaply.

To create colours on the LCD, shapes of various rectangles were layered on top of each other.
Adding a small rectangle starting from the left and making it "grow" to the right will create a loading bar.
To add colour to the text or shape used, RGB codes of the colour were used. 
Careful consideration, aswell as trial and error can be used to match colours.
Using a RGB code reference for colours can be a useful tool, and easily called with (XXX,YYY,ZZZ).
https://www.google.com/url?sa=i&url=https%3A%2F%2Fwww.linsnled.com%2Fdifference-between-lcd-and-led.html&psig=AOvVaw3LcaP_Jem97OfHqLf6a9eg&ust=1746281332129000&source=images&cd=vfe&opi=89978449&ved=0CBAQjRxqFwoTCLi08eP7hI0DFQAAAAAdAAAAABAE

- Rotary Encoder
A rotary encoder is commonly found on car radios.
It provides a very simple and intuitive left-right directions, as well as a tertiary function on a button press.
It is an electromechanical device that rotates the rotational motion of a shaft into a clock signal.
Matching the signal against a reference clock, it will be capable of providing information of position, speed and direction.
This project will only use position and direction, as the sensitivity is not as fine as it would need to be to desire the use of speed. CHECK THIS PART BRO.

- DMA
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

The DMA is configures at it's channel level, composed of a sequence of AHB bus transfers
- Double Buffering

- Schematic
- Testing
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

Spent 4 hours debugging the set up, the pins for PA5 and PA4 did not like being changed to PB6 and PB1.
Why is that?

E_S image 1 error appeared, why is that?

getting caught in mode 0?
--Restored to previous code

Percent_int, the original variable is causing issues on the classic IO ADC 2 DAC. Why is that?

DMA - Is timer 7 counting? - Check breakpoint.

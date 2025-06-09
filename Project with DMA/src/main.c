// STM32L432 - Simple ADC (PA5) to DAC (PA4) Polling Passthrough

#include <stm32l432xx.h>
#include <stdint.h>

// --- Prototypes ---
void setup_adc_dac_simple(void);
void initClocks(void);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void init_gpio_adc_dac(void);
void init_adc_simple(void);
void init_dac_simple(void);
void delay_us(volatile uint32_t us);

// --- Main ---
int main() {
    initClocks();            // Essential: Setup system clock first
    setup_adc_dac_simple();  // Configure GPIO, ADC, DAC

    uint16_t adc_value;

    while (1) {
        // 1. Start ADC Conversion
        ADC1->CR |= ADC_CR_ADSTART;

        // 2. Wait for End of Conversion (EOC) flag
        while (!(ADC1->ISR & ADC_ISR_EOC));
        // Note: EOC flag is automatically cleared by reading DR

        // 3. Read ADC value
        adc_value = ADC1->DR;

        // 4. Write ADC value to DAC Data Holding Register
        DAC1->DHR12R1 = adc_value;

        // 5. Optional: Software Trigger DAC update (often not needed if TEN1=0)
        // DAC1->SWTRIGR |= DAC_SWTRIGR_SWTRIG1;

        // No delay needed, ADC conversion time provides inherent delay
    }
}

// --- Combined Setup ---
void setup_adc_dac_simple(void) {
    init_gpio_adc_dac(); // Configure PA4 (DAC), PA5 (ADC)
    init_dac_simple();   // Setup DAC for simple manual writes
    init_adc_simple();   // Setup ADC for simple polling reads
}


// --- Initialization Functions ---

void initClocks() {
    // Initialize the clock system to 80MHz using PLL sourced from MSI (4MHz)
    // (Using the provided initClocks function content)

    // Ensure MSI is on and selected as system clock source initially
    RCC->CR |= RCC_CR_MSION;
    while (!(RCC->CR & RCC_CR_MSIRDY));
    RCC->CFGR &= ~RCC_CFGR_SW; // Select MSI as system clock (00)
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_MSI);

    // Disable PLL before configuration
    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY);

    // Configure PLL: 80MHz from 4MHz MSI
    RCC->PLLCFGR = (RCC_PLLCFGR_PLLSRC_MSI) |
                   (0 << RCC_PLLCFGR_PLLM_Pos) | // PLLM = /1
                   (40 << RCC_PLLCFGR_PLLN_Pos) | // PLLN = x40
                   (0 << RCC_PLLCFGR_PLLR_Pos) | // PLLR = /2
                   RCC_PLLCFGR_PLLREN;

    // Enable PLL and wait for it to be ready
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // Configure Flash wait states (3 WS for 80MHz VCORE range 1)
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_3WS;
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_3WS);

    // Select PLL as system clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    SystemCoreClockUpdate(); // Update SystemCoreClock variable
}

void init_gpio_adc_dac(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; // Enable GPIOA clock

    // PA5 = ADC1_IN10 = Analog Input
    pinMode(GPIOA, 5, 3); // Set PA5 to Analog mode (11)

    // PA4 = DAC1_OUT1 = Analog Output
    pinMode(GPIOA, 4, 3); // Set PA4 to Analog mode (11)
}

void init_dac_simple(void) {
    // Enable DAC clock
    RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN;

    // Disable DAC channel 1 before configuring
    DAC1->CR &= ~DAC_CR_EN1;

    // Configure for simple manual update (NO DMA, NO TRIGGER)
    DAC1->CR &= ~DAC_CR_TEN1;    // Ensure Trigger is DISABLED for Chan 1
    DAC1->CR &= ~DAC_CR_DMAEN1;  // Ensure DMA requests are DISABLED for Chan 1
    // TSEL1 doesn't matter if TEN1 = 0

    // Enable DAC channel 1
    DAC1->CR |= DAC_CR_EN1;
    delay_us(10); // Small delay for DAC startup
}

void init_adc_simple(void) {
    // 1. Enable ADC Clock and Set Clock Source
    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    RCC->CCIPR |= RCC_CCIPR_ADCSEL; // Bits 29:28 = 11: System Clock selected for ADC
    ADC1_COMMON->CCR |= ADC_CCR_VREFEN; // Enable VRefInt

    // 2. Power Up ADC Regulator
    ADC1->CR &= ~ADC_CR_DEEPPWD;
    ADC1->CR |= ADC_CR_ADVREGEN;
    delay_us(20); // Wait for regulator startup

    // 3. Calibrate ADC
    ADC1->CR &= ~ADC_CR_ADEN;
    ADC1->CR |= ADC_CR_ADCAL;
    while (ADC1->CR & ADC_CR_ADCAL);

    // 4. Configure ADC Settings for Polling (NO DMA, NO TRIGGER)
    ADC1->CFGR &= ~ADC_CFGR_DMAEN;     // Ensure DMA requests are DISABLED
    ADC1->CFGR &= ~ADC_CFGR_DMACFG;    // Ensure Circular DMA mode is DISABLED
    ADC1->CFGR &= ~ADC_CFGR_EXTEN;    // Ensure External Trigger is DISABLED
    ADC1->CFGR &= ~ADC_CFGR_CONT;     // Ensure Continuous mode is DISABLED (we want single shots)
    ADC1->CFGR &= ~ADC_CFGR_RES;      // Ensure 12-bit resolution (00)
    ADC1->CFGR &= ~ADC_CFGR_ALIGN;    // Ensure Right alignment (0)

    // 5. Configure ADC Channel Sequence (Channel 10 = PA5)
    ADC1->SQR1 = (10 << ADC_SQR1_SQ1_Pos); // Channel 10 is first and only conversion
    ADC1->SQR1 &= ~ADC_SQR1_L;        // Ensure sequence length = 1 (0000)

    // 6. Enable ADC
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)); // Wait for ADC Ready flag
}


// --- Utility Functions ---

// Basic microsecond delay - TUNE a_constant BASED ON YOUR SystemCoreClock (80MHz here)
void delay_us(volatile uint32_t us) {
    volatile uint32_t delay_loops = us * 16; // Approximation for 80MHz
    while (delay_loops--) {
        __NOP();
    }
}

// pinMode Helper
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode) {
    uint32_t mode_value = Port->MODER;
    uint32_t clear_mask = ~(3u << (BitNumber * 2));
    uint32_t set_mask = (Mode & 3u) << (BitNumber * 2);
    Port->MODER = (mode_value & clear_mask) | set_mask;
}

// You will also need the definition for SystemCoreClockUpdate() if you use it,
// or remove the call from initClocks() if it's not needed/available.
// Often provided by CMSIS headers/startup files.
// Dummy definition if not available:
// uint32_t SystemCoreClock = 80000000; // Assume 80MHz
// void SystemCoreClockUpdate(void) {
//     // Typically this function reads registers to calculate the actual clock.
//     // For this example, we can just keep the assumed value.
//     SystemCoreClock = 80000000;
// }
// ****************************************************************
// * Carlos Munar
// * Scanning Radar - 10/23/2026
// ****************************************************************
#include "stm32f4xx.h"
#include "OLED.h"
#include "eyes.h"
#include "audio.h"
#include "SysClock180MHz.h"
#include <stdint.h> // Include stdint.h for uint32_t type
#include <stdio.h> // Include stdio.h for snprintf function

/* Switch Definitions */
#define S1 8
#define SWITCH_PORT GPIOC

void delay_ms(uint32_t ms); // Function prototype for delay function
char buffer[90];
uint32_t initialClock; // Initial system clock frequency (16 MHz)
uint32_t finalClock;   // Final system clock frequency (180 MHz)

int main(void){
    /* Enable the Cortex-M4 floating-point unit. */
    SCB->CPACR |= (0xFU << 20U);
    __DSB();
    __ISB();

    SystemCoreClockUpdate(); // CMSIS function to update SystemCoreClock variable
    initialClock = SystemCoreClock; // Store the initial clock frequency (16 MHz)
    SystemClock_Config(); // Configure the system clock to 180 MHz
    SystemCoreClockUpdate(); // Update SystemCoreClock variable after clock change
    finalClock = SystemCoreClock; // Store the final clock frequency (180 MHz)

    /* Configure S1 on PC8 as an input. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    SWITCH_PORT->MODER &= ~(3U << (S1 * 2U));
    SWITCH_PORT->PUPDR &= ~(3U << (S1 * 2U));

    OLED_Init();
    SSD_Init();
    SSD_Timer_Init();
    eyes_Init();
    Audio_Init();

    // SH1106_RenderFullScreenLogo();
    // delay_ms(2000); // Delay for 2 seconds to display the logo
    // OLED_Clear();
    OLED_Print(0, 0, "Range Finder Demo");
    OLED_Print(1, 0, "C. Munar: 10/23/26");
    
    while(1){
        // Main loop
        uint32_t pulse_us;
        uint32_t distance_hundredths;
        uint32_t divisor = 148U;

        /* Start a new measurement. */
        eyes_Trigger();
        Audio_UpdateSwitch();

        delay_ms(40U); // Allow time for echo to return.

        pulse_us = eyes_GetPulseWidth();

        if (pulse_us == 0U)
        {
            OLED_Print(3, 1*6, "No echo detected ");
            snprintf(buffer, sizeof(buffer), "Dist: ??.?? %s   ", "in");
            OLED_Print(4, 1*6, buffer);
        } else {
            /* Rounded distance in hundredths of the selected unit. */
            distance_hundredths =
                (pulse_us * 100U + divisor / 2U) / divisor;

            snprintf(buffer, sizeof(buffer),
                     "Pulse: %lu us    ",
                     (unsigned long)pulse_us);
            OLED_Print(3, 1*6, buffer);

            snprintf(buffer, sizeof(buffer),
                     "Dist: %lu.%02lu %s   ",
                     (unsigned long)(distance_hundredths / 100U),
                     (unsigned long)(distance_hundredths % 100U),
                     "in");
            OLED_Print(4, 1*6, buffer);

            snprintf(buffer, sizeof(buffer),
                     "Pulse: %lu us | Dist: %lu.%02lu %s\r\n",
                     (unsigned long)pulse_us,
                     (unsigned long)(distance_hundredths / 100U),
                     (unsigned long)(distance_hundredths % 100U),
                     "in");

            uint32_t note = Audio_UpdateDistance(distance_hundredths);
            snprintf(buffer, sizeof(buffer), "Note: %lu   ", (unsigned long)note);
            OLED_Print(5, 1*6, buffer);

            Audio_UpdateDistance(distance_hundredths);
        }

        /* Approximately 40 ms measurement wait + 210 ms delay. */
        delay_ms(210U);

        SSD_Refresh();
        SSD_DisplayValue(pulse_us);
    }   

    return 0; // Program should never reach here due to infinite loop
}
void delay_ms(uint32_t ms){
    if(SystemCoreClock == 16000000U){ // If the system clock is 16 MHz
        for(uint32_t i = 0; i < ms * 2000; i++){
            __NOP();
        }
    } else if(SystemCoreClock == 180000000U){ // If the system clock is 180 MHz
        for(uint32_t i = 0; i < ms * 22500; i++){
            __NOP();
        }
    }
}
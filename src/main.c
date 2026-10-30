// ****************************************************************
// * Carlos Munar
// * Scanning Radar - 10/23/2026
// ****************************************************************
#include "stm32f4xx.h"
#include "OLED.h"
#include "eyes.h"
#include "audio.h"
#include "NeoPixel.h"
#include "SysClock180MHz.h"
#include <stdint.h> // Include stdint.h for uint32_t type
#include <stdio.h> // Include stdio.h for snprintf function

/* Switch Definitions */
#define S1 8
#define SWITCH_PORT GPIOC

void delay_ms(uint32_t ms); // Function prototype for delay function
char buffer[90];
static const char *noteSymbols[] = {"C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4", "OFF"};

typedef struct {
    uint8_t     r, g, b;
} RGB_Colors;

static const RGB_Colors color_table[13] = {
    {  0,   0, 255},
    {  0,  93, 255},
    {  0, 185, 255},
    {  0, 255, 232},
    {  0, 255, 139},
    {  0, 255,  46},
    { 46, 255,   0},
    {139, 255,   0},
    {232, 255,   0},
    {255, 185,   0},
    {255,  93,   0},
    {255,   0,   0},
    {  0,   0,   0}
};
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
    NeoPixel_Init();

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
            if ((unsigned long)note > 12) {note = 12;}
            
            snprintf(buffer, sizeof(buffer), "Note: %s   ", noteSymbols[note]);
            OLED_Print(5, 1*6, buffer);

            // Fixed-color test
            NeoPixel_SetPixel(0, color_table[note].r, color_table[note].g, color_table[note].b);
            NeoPixel_SetPixel(1, color_table[note].r, color_table[note].g, color_table[note].b);
            NeoPixel_SetPixel(2, color_table[note].r, color_table[note].g, color_table[note].b);
            NeoPixel_SetPixel(3, color_table[note].r, color_table[note].g, color_table[note].b);
            NeoPixel_Show();

            Audio_UpdateDistance(distance_hundredths);
        }

        /* Approximately 40 ms measurement wait + 210 ms delay. */
        delay_ms(210U);

        SSD_Refresh();
        SSD_DisplayValue((int32_t)(Audio_GetCurrentNote() * 100)/10);
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
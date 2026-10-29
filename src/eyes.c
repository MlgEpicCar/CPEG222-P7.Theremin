#include "stm32f4xx.h"
#include "eyes.h"
// I'm calling the HCSR04 "eyes" bc i want to, deal with it.
// HCSR04

#define TRIG_PIN 3U
#define ECHO_PIN 1U

/* TIM7 runs at 1 MHz: one count per microsecond. */
static volatile uint16_t echo_start_us = 0U;
static volatile uint32_t echo_pulse_width_us = 0U;
static volatile uint8_t echo_rising_seen = 0U;

void eyes_Init(void)
{
    /* Enable GPIOE, GPIOG, SYSCFG, and TIM7 clocks. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOGEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;

    /* PE3: general-purpose output for TRIG. */
    GPIOE->MODER &= ~(3U << (TRIG_PIN * 2U));
    GPIOE->MODER |=  (1U << (TRIG_PIN * 2U));
    GPIOE->OTYPER &= ~(1U << TRIG_PIN);
    GPIOE->OSPEEDR |= (2U << (TRIG_PIN * 2U));
    GPIOE->BSRR = 1U << (TRIG_PIN + 16U);

    /* PG1: input for ECHO. */
    GPIOG->MODER &= ~(3U << (ECHO_PIN * 2U));
    GPIOG->PUPDR &= ~(3U << (ECHO_PIN * 2U));

    /*
     * Map EXTI line 1 to GPIOG.
     * GPIO port encoding: A=0, B=1, ..., G=6.
     */
    SYSCFG->EXTICR[0] &= ~(0xFU << 4U);
    SYSCFG->EXTICR[0] |=  (6U << 4U);

    /* Interrupt on both rising and falling edges of PG1. */
    EXTI->IMR  |=  (1U << ECHO_PIN);
    EXTI->RTSR |=  (1U << ECHO_PIN);
    EXTI->FTSR |=  (1U << ECHO_PIN);
    EXTI->PR = (1U << ECHO_PIN);  /* Clear pending flag. */

    /* TIM7: 90 MHz / (89 + 1) = 1 MHz. */
    TIM7->CR1 = 0U;
    TIM7->PSC = 89U;
    TIM7->ARR = 0xFFFFU;
    TIM7->CNT = 0U;
    TIM7->EGR = TIM_EGR_UG;
    TIM7->SR = 0U;
    TIM7->CR1 = TIM_CR1_CEN;

    /* Enable EXTI1 interrupt in the NVIC. */
    NVIC_ClearPendingIRQ(EXTI1_IRQn);
    NVIC_SetPriority(EXTI1_IRQn, 2U);
    NVIC_EnableIRQ(EXTI1_IRQn);
}

void eyes_Trigger(void)
{
    echo_pulse_width_us = 0U;
    echo_rising_seen = 0U;
    EXTI->PR = (1U << ECHO_PIN);

    /* Ensure a clean LOW-to-HIGH trigger pulse. */
    GPIOE->BSRR = 1U << (TRIG_PIN + 16U);

    /* Timer-based delay: approximately 2 microseconds. */
    uint16_t start = (uint16_t)TIM7->CNT;
    while ((uint16_t)((uint16_t)TIM7->CNT - start) < 2U)
    {
    }

    GPIOE->BSRR = 1U << TRIG_PIN;

    /* HC-SR04 requires a trigger pulse of at least 10 us. */
    start = (uint16_t)TIM7->CNT;
    while ((uint16_t)((uint16_t)TIM7->CNT - start) < 10U)
    {
    }

    GPIOE->BSRR = 1U << (TRIG_PIN + 16U);
}

void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & (1U << ECHO_PIN))
    {
        /* Clear EXTI1 pending flag. */
        EXTI->PR = (1U << ECHO_PIN);

        uint16_t now = (uint16_t)TIM7->CNT;

        if (GPIOG->IDR & (1U << ECHO_PIN))
        {
            /* Rising edge: echo pulse begins. */
            echo_start_us = now;
            echo_rising_seen = 1U;
        }
        else if (echo_rising_seen)
        {
            /* Falling edge: echo pulse ends. */
            echo_pulse_width_us =
                (uint16_t)(now - echo_start_us);
            echo_rising_seen = 0U;
        }
    }
}

uint32_t eyes_GetPulseWidth(void)
{
    return echo_pulse_width_us;
}

float eyes_GetDistanceCM(void)
{
    return (float)echo_pulse_width_us / 58.0f;
}
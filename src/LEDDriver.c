/**
  ******************************************************************************
  * @file    LEDDriver.c
  * @author  Deepak Ravikumar
  * @version V1.0
  * @date    24-Apr-2019
  * @brief   Device driver for LED and PWM control
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "LEDDriver.h"

void initLED() {
    // Using GPIOB PB0, PB5 and PB4
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOB->MODER |= GPIO_MODER_MODER0_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1;

    // Set alternate function
    // Clear
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFRL0 | GPIO_AFRL_AFRL4 | GPIO_AFRL_AFRL5);

    // Set
    GPIOB->AFR[0] |= (1 << 0) | (1 << (4*4)) | (1 << (4*5));

    // Enable Timer 3
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Configure TIM3
    TIM3->PSC = 47;
    TIM3->ARR = 255;


    // Using Channel 1,2,3
    // Channel 1
    TIM3->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
    TIM3->CCR1 = 4;

    // Channel 2
    TIM3->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
    TIM3->CCR2 = 4;

    // Channel 3
    TIM3->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3PE;
    TIM3->CCR3 = 4;

    TIM3->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;
    TIM3->BDTR |= TIM_BDTR_MOE;
    TIM3->CR1 |= TIM_CR1_CEN;

}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    TIM3->CCR1 = r;
    TIM3->CCR2 = g;
    TIM3->CCR3 = b;
}

void colorTest() {
    uint8_t r, g, b;
    r = g = b = 0;
    COLOR_STATE state = R_UP;
    while(state != END) {
        switch(state) {
        case R_UP:
            r++;
            if(r == 255)
                state = R_DWN_G_UP;
            break;

        case R_DWN_G_UP:
            r--;
            g++;

            if(g == 255)
                state = G_DWN_B_UP;
            break;

        case G_DWN_B_UP:
            g--;
            b++;

            if(b == 255)
                state = B_DWN;
            break;

        case B_DWN:
            b--;
            if(b == 0)
                state = END;
            break;

        default:
            state = END;
            break;
       }

       setColor(r, g, b);
       micro_wait(2000);
    }
}

/**
  ******************************************************************************
  * @file    main.c
  * @author  Deepak Ravikumar
  * @version V1.0
  * @date    22-Apr-2019
  * @brief   Default main function, initializes everything
  ******************************************************************************
*/

#include "graph.h"
#include "rfmodule.h"
#include "LEDDriver.h"

void initSPI1() {
    /* manually rig CS and run SPI blocking because it doesn't work quite right in the peripheral? */

    RCC->AHBENR |= 0x20000;     // Enable GPIOA in RCC
    GPIOA->MODER |= 0xA900;     // Set PA5-7 AF, PA4 OUT
    GPIOA->ODR |= 0x10;

    RCC->APB2ENR |= 0x1000;     // Enable SPI1 in RCC
    SPI1->CR1 = 0x001C;         // BR 4 div, master
    SPI1->CR2 = 0x0F04;         // 16-bit word, SSOE
    //SPI1->CR1 |= 0x40;            // SPI Enable
}

void initDevLED()
{
    RCC->AHBENR |= 0x80000;     // Enable GPIOA in RCC
    GPIOC->MODER |= 0x50000;    // PC8-9 OUT
}

void extiInit()
{
    RCC->AHBENR |= 0x20000;     // Enable GPIOA in RCC

    // for user pushbutton - PA0
    EXTI->RTSR |= 0x01;
    EXTI->IMR |= 0x01;
    NVIC->ISER[0] |= (1 << 5);

    // for RF - PA2
    EXTI->FTSR |= (1 << 2);
    EXTI->IMR |= (1 << 2);
    NVIC->ISER[0] |= (1 << 6);
}

void EXTI0_1_IRQHandler(void) {
    EXTI->PR |= 0x01;
    rfSendTestMsg();
}

void EXTI2_3_IRQHandler(void) {
    EXTI->PR |= (1 << 3);
    rfHandleTxRxInterrupt();
}

void initSystem() {
    initSPI1();
    initLED();
    initDevLED();
    colorTest();

    micro_wait(2000);

    rfInit();
    extiInit();
}


int main(void)
{
    initSystem();
    rfJoinNetwork();
    while(1)
    {
        rfProcessRxQueue();
        rfProcessTxQueue();
    }

    // Negotiate RfAddr
    // Build graph
    buildDummyGraph();
    colorGraph();



    for(;;)
        asm ("wfi");
}

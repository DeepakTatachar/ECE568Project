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

extern VERTEX* networkGraph;

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

handleGenericPacket(pPacket* packet) {
    switch(packet->pType) {
    case PACKET_SETCOLOR:
        // packet 0 is s in sRGB
        setColor(packet->pData[1], packet->pData[2], packet->pData[3]);
        break;
    default:
        break;
    }
}

void sendVertexColor() {
    VERTEX* graph = networkGraph;
    while(graph->next != NULL) {
        pPacket packet;
        packet.pDAddr = graph->label;
        packet.pData = &(graph->color);
        packet.pDataLength = 4;
        rfSendPacket(&packet);
        graph = graph->next;
    }
}

int main(void)
{
    initSystem();
    rfJoinNetwork();
    while(1)
    {
        rfProcessRxQueue();
        rfProcessTxQueue();

        if(isCoordinator() && shouldRecolorGraph()) {
            colorGraph();
            sendVertexColor();
            graphRecolored();
        }
    }

    for(;;)
        asm ("wfi");
}

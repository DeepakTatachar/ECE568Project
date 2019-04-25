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
#include "sensor.h"

#define MAXTRIG 8

extern VERTEX* networkGraph;
extern int rfAddr;
extern char isEstablished;
extern network_neighbor_t neighbors[];
extern int nextNeighbor;

int assignedColor;
int triggers[MAXTRIG];
int trigColors[MAXTRIG];

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
    RCC->AHBENR |= 0x80000;     // Enable GPIOC in RCC
    GPIOC->MODER |= 0x50000;    // PC8-9 OUT
}

void extiInit()
{
    RCC->AHBENR |= 0x20000;     // Enable GPIOA in RCC
    RCC->AHBENR |= 0x40000;		// Enable GPIOB in RCC

    // for user pushbutton - PA0
    EXTI->RTSR |= 0x01;
    EXTI->IMR |= 0x01;
    NVIC->ISER[0] |= (1 << 5);

    // for RF - PA2
    EXTI->FTSR |= (1 << 2);
    EXTI->IMR |= (1 << 2);
    NVIC->ISER[0] |= (1 << 6);

    // for sensor - PB13
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->EXTICR[3] |= 0x0010;
    EXTI->FTSR |= (1 << 13);
	EXTI->IMR |= (1 << 13);
    NVIC->ISER[0] |= (1 << 7);

}

void EXTI0_1_IRQHandler(void) {
    EXTI->PR |= 0x01;
    rfSendTestMsg();
}

void EXTI2_3_IRQHandler(void) {
    EXTI->PR |= (1 << 3);
    rfHandleTxRxInterrupt();
}

void EXTI4_15_IRQHandler(void) {
    EXTI->PR |= (1 << 13);

    if(isEstablished && handleSensorTriggerInterrupt())
    {
    	addTrigger(rfAddr, assignedColor);
    	triggerNeighbors(1);
    }
}

void TIM14_IRQHandler(void) {
	TIM14->SR &= ~(0x01);
	handleSensorUntriggerInterrupt();
	removeTrigger(rfAddr);
	triggerNeighbors(0);
}

void initSystem() {
    initSPI1();
    initLED();
    initDevLED();
    initSensor();
    colorTest();

    micro_wait(2000);

    rfInit();
    extiInit();
}



handleGenericPacket(pPacket* packet) {
    switch(packet->pType) {
    case PACKET_SETCOLOR: {

//        uint8_t r = packet->pData[0];
//        uint8_t g = packet->pData[1];
//        uint8_t b = packet->pData[2];
//        setColor(r, g, b);
    	assignedColor = packet->pData[0] << 16 | packet->pData[1] << 8 | packet->pData[2];
    	clearAllTriggers();
        break;
    }
    case PACKET_TRIGGER: {
    	addTrigger(packet->pData[0], packet->pData[1] << 16 | packet->pData[2] << 8 | packet->pData[3]);
    	break;
    }
    case PACKET_UNTRIGGER: {
    	removeTrigger(packet->pData[0]);
    	break;
    }
    default:
        break;
    }
}

void sendVertexColor() {
    VERTEX* graph = networkGraph;
    while(graph != NULL) {
        pPacket packet;

        packet.pType = PACKET_SETCOLOR;
        packet.pDAddr = graph->label;
        packet.pData = &(graph->color);
        packet.pDataLength = 4;

        if(graph->label == rfAddr) {
        	assignedColor = graph->color;
            //setColor32(graph->color);
            graph= graph->next;
            continue;
        }
        rfSendPacket(&packet);
        graph = graph->next;
    }
}

void addTrigger(int id, int color)
{
	int cx = 0;
	while(triggers[cx] && cx < MAXTRIG) { cx++; }
	if(cx < MAXTRIG)
	{
		triggers[cx] = id;
		trigColors[cx] = color;
		setColorFromTriggers();
	}
}

void removeTrigger(int id)
{
	for(int cx = 0; cx < MAXTRIG; cx++)
	{
		if(triggers[cx] == id)
		{
			triggers[cx] = 0;
			trigColors[cx] = 0;
			setColorFromTriggers();
			break;
		}
	}
}

void clearAllTriggers()
{
	for(int cx = 0; cx < MAXTRIG; cx++)
	{
		triggers[cx] = 0;
		trigColors[cx] = 0;
	}

	setColorFromTriggers();
}

void setColorFromTriggers()
{
	int r = 0, g = 0, b = 0;

	for(int cx = 0; cx < MAXTRIG; cx++)
	{
		if(triggers[cx])
		{
			r += trigColors[cx] & 0xff;
			g += (trigColors[cx] >> 8) & 0xff;
			b += (trigColors[cx] >> 16);
		}
	}

	r = r > 255 ? 255 : r;
	g = g > 255 ? 255 : g;
	b = b > 255 ? 255 : b;

	setColor(r,g,b);
}

void triggerNeighbors(int trigger)
{
	for(int cx = 0; cx < MAXNETSIZE && cx < nextNeighbor; cx++)
	{
		uint8_t idcolr[4] = {rfAddr, assignedColor >> 16, assignedColor >> 8, assignedColor};
		pPacket packet;

		packet.pType = trigger ? PACKET_TRIGGER : PACKET_UNTRIGGER;
		packet.pDAddr = neighbors[cx].addr;
		packet.pData = idcolr;
		packet.pDataLength = 4;

		rfSendPacket(&packet);
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
            clearAllTriggers();
        }
    }

    for(;;)
        asm ("wfi");
}

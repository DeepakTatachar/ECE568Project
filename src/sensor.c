
#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "sensor.h"

void initSensor()
{
	// init timer 14 for trigger timeout
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
	TIM14->PSC = 47999;		// scale to 1kHz
	TIM14->ARR = 1500;		// reset after 1500ms
	TIM14->DIER |= 0x01;	// enable update event interrupt

	NVIC->ISER[0] |= (1 << 19);
}

int handleSensorTriggerInterrupt()
{
	int performTriggerAction = 0;

	TIM14->CNT = 0;		// reset counter

	if(!(TIM14->CR1 & TIM_CR1_CEN))
	{
		performTriggerAction = 1;
		TIM14->CR1 |= TIM_CR1_CEN;
	}

	return performTriggerAction;
}

void handleSensorUntriggerInterrupt()
{
	TIM14->CR1 &= ~TIM_CR1_CEN;
}

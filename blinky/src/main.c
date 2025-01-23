#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
int main()
{
    setup();
    while(1)
    {
        GPIOB->ODR |= (1 << 3);
        delay(100000);
        GPIOB->ODR &= ~(1 << 3);
        delay(100000);
    }
}
void setup(void)
{
    RCC->AHB2ENR |= (1 << 1); // turn on GPIOB
  //  GPIOB->MODER |= (1 << 6); // Configure GPIOB, bit 3 as an output
  //  GPIOB->MODER &= ~(1 << 7);
    pinMode(GPIOB,3,1);
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
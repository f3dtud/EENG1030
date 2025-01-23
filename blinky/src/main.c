#include <stm32l432xx.h>
#include <stdint.h>
void setup(void);
void delay(volatile uint32_t dly);
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
    GPIOB->MODER |= (1 << 6); // Configure GPIOB, bit 3 as an output
    GPIOB->MODER &= ~(1 << 7);

}
void delay(volatile uint32_t dly)
{
    while(dly--);
}

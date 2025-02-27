/*
I2S test program
I/O List:
[ SAI1_MCKA_A :   PA3, AF 13      Master clock - perhaps not necessary ] Not doing this for now
SAI1_SCK_A  :   PA8, AF 13      Bit clock 
SAI1_FS_A   :   PA9, AF 13      Selects Left/Right channel
SAI1_SD_A   :   PA10, AF 13 .   This is the audio data pin.
Using SAI1_A in master mode
Will attempt 48kHz sampling rate.  BCLK (SAI1_SCK) = 3.072MHz, LRCLK = 48kHz
*/
#include <stm32l432xx.h>
#include <stdint.h>
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF);
void initClocks(void);
void setup(void);
void delay(volatile uint32_t dly);
void initSAI(void);
int main()
{
    setup();
    initSAI();
    while(1)
    {
        GPIOB->ODR |= (1 << 3);        
        //SAI1_Block_A->CLRFR=0xff; // clear all flags
        SAI1_Block_A->DR=0x12345678;
        //SAI1_Block_A->DR=0x12345678;
        //SAI1_Block_A->DR=0x12345678;
        GPIOB->ODR &= ~(1 << 3);
        delay(100);

    }
}
void initSAI(void)
{
    pinMode(GPIOA,8,2);
    pinMode(GPIOA,9,2);
    pinMode(GPIOA,10,2);
    selectAlternateFunction(GPIOA,8,13);
    selectAlternateFunction(GPIOA,9,13);
    selectAlternateFunction(GPIOA,10,13);    

    RCC->APB2ENR |= (1 << 21);
    
    RCC->PLLSAI1CFGR = (2 << 27)+(127  << 8)+(1 << 16); // set SAI bit clock to 48kHz
    RCC->CR |= (1 << 26); // turn on SAI1 PLL
    SAI1_Block_A->CR1 = (4 << 5) ; // 16 bit data
    SAI1_Block_A->SLOTR |= (1 << 8) + (1 << 16)+(1 << 17);
    SAI1_Block_A->FRCR = 16-1; // 16 bit frame length
    //SAI1_Block_A->FRCR |= (1 << 16);
    SAI1_Block_A->CR1 |= (1 << 16); // turn on block A of SAI1

    SAI1->GCR = (1 << 4);

}
void setup()
{
    initClocks();
    RCC->AHB2ENR |= (1 << 1) | (1 << 0); // enable GPIOA and GPIOB
    pinMode(GPIOB,3,1);
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
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF)
{
    // The alternative function control is spread across two 32 bit registers AFR[0] and AFR[1]
    // There are 4 bits for each port bit.
    if (BitNumber < 8)
    {
        Port->AFR[0] &= ~(0x0f << (4*BitNumber));
        Port->AFR[0] |= (AF << (4*BitNumber));
    }
    else
    {
        BitNumber = BitNumber - 8;
        Port->AFR[1] &= ~(0x0f << (4*BitNumber));
        Port->AFR[1] |= (AF << (4*BitNumber));
    }
}
void initClocks()
{
	// Initialize the clock system to a higher speed.
	// At boot time, the clock is derived from the MSI clock 
	// which defaults to 4MHz.  Will set it to 80MHz
	// See chapter 6 of the reference manual (RM0393)
	    RCC->CR &= ~(1 << 24); // Make sure PLL is off
	
	// PLL Input clock = MSI so BIT1 = 1, BIT 0 = 0
	// PLLM = Divisor for input clock : set = 1 so BIT6,5,4 = 0
	// PLL-VCO speed = PLL_N x PLL Input clock
	// This must be < 344MHz
	// PLL Input clock = 4MHz from MSI
	// PLL_N can range from 8 to 86.  
	// Will use 80 for PLL_N as 80 * 4 = 320MHz
	// Put value 80 into bits 14:8 (being sure to clear bits as necessary)
	// PLLP : Must pick a value that divides 320MHz down to <= 80MHz
	// If BIT17 = 1 then divisor is 17; 320/17 = 18.82MHz : ok (PLLP used by SAI)
	// PLLQEN : Don't need this so set BIT20 = 0
	// PLLQ : Must divide 320 down to value <=80MHz.  
	// Set BIT22,21 to 1 to get a divisor of 8 : ok
	// PLLREN : This enables the PLLCLK output of the PLL
	// I think we need this so set to 1. BIT24 = 1 
	// PLLR : Pick a value that divides 320 down to <= 80MHz
	// Choose 4 to give an 80MHz output.  
	// BIT26 = 0; BIT25 = 1
	// All other bits reserved and zero at reset
        RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) + (1 << 17) + (80 << 8) + (1 << 0);
	    RCC->CR |= (1 << 24); // Turn PLL on
	    while( (RCC->CR & (1 << 25))== 0); // Wait for PLL to be ready
	// configure flash for 4 wait states (required at 80MHz)
	    FLASH->ACR &= ~((1 << 2)+ (1 << 1) + (1 << 0));
	    FLASH->ACR |= (1 << 2); 
	    RCC->CFGR |= (1 << 1)+(1 << 0); // Select PLL as system clock
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
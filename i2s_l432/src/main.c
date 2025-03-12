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
#define NUM_SAMPLES 180
const uint16_t samples[NUM_SAMPLES*2]={0,0,142,142,285,285,428,428,570,570,711,711,851,851,990,990,1128,1128,1265,1265,1400,1400,1534,1534,1665,1665,1795,1795,1922,1922,2047,2047,2170,2170,2290,2290,2407,2407,2521,2521,2632,2632,2740,2740,2845,2845,2946,2946,3043,3043,3137,3137,3227,3227,3313,3313,3395,3395,3473,3473,3547,3547,3616,3616,3681,3681,3741,3741,3797,3797,3848,3848,3895,3895,3937,3937,3974,3974,4006,4006,4033,4033,4056,4056,4073,4073,4085,4085,4093,4093,4095,4095,4093,4093,4085,4085,4073,4073,4056,4056,4033,4033,4006,4006,3974,3974,3937,3937,3895,3895,3848,3848,3797,3797,3741,3741,3681,3681,3616,3616,3547,3547,3473,3473,3395,3395,3313,3313,3227,3227,3137,3137,3043,3043,2946,2946,2845,2845,2740,2740,2632,2632,2521,2521,2407,2407,2290,2290,2170,2170,2047,2047,1922,1922,1795,1795,1665,1665,1534,1534,1400,1400,1265,1265,1128,1128,990,990,851,851,711,711,570,570,428,428,285,285,142,142,0,0,-142,-142,-285,-285,-428,-428,-570,-570,-711,-711,-851,-851,-990,-990,-1128,-1128,-1265,-1265,-1400,-1400,-1534,-1534,-1665,-1665,-1795,-1795,-1922,-1922,-2047,-2047,-2170,-2170,-2290,-2290,-2407,-2407,-2521,-2521,-2632,-2632,-2740,-2740,-2845,-2845,-2946,-2946,-3043,-3043,-3137,-3137,-3227,-3227,-3313,-3313,-3395,-3395,-3473,-3473,-3547,-3547,-3616,-3616,-3681,-3681,-3741,-3741,-3797,-3797,-3848,-3848,-3895,-3895,-3937,-3937,-3974,-3974,-4006,-4006,-4033,-4033,-4056,-4056,-4073,-4073,-4085,-4085,-4093,-4093,-4095,-4095,-4093,-4093,-4085,-4085,-4073,-4073,-4056,-4056,-4033,-4033,-4006,-4006,-3974,-3974,-3937,-3937,-3895,-3895,-3848,-3848,-3797,-3797,-3741,-3741,-3681,-3681,-3616,-3616,-3547,-3547,-3473,-3473,-3395,-3395,-3313,-3313,-3227,-3227,-3137,-3137,-3043,-3043,-2946,-2946,-2845,-2845,-2740,-2740,-2632,-2632,-2521,-2521,-2407,-2407,-2290,-2290,-2170,-2170,-2047,-2047,-1922,-1922,-1795,-1795,-1665,-1665,-1534,-1534,-1400,-1400,-1265,-1265,-1128,-1128,-990,-990,-851,-851,-711,-711,-570,-570,-428,-428,-285,-285,-142,-142};


int main()
{
    int i;
    setup();
    initSAI();
// DMA for SAI1 Channel A:
// DMA2, Channel 6, DMA Req 5
    RCC->AHB1ENR |= (1 << 1); // enable DMA2
    DMA2_Channel6->CMAR=samples;
    DMA2_Channel6->CPAR=&SAI1_Block_A->DR;
    DMA2_Channel6->CNDTR = NUM_SAMPLES;
    DMA2_CSELR->CSELR = (1 << 20);
    DMA2_Channel6->CCR=(1 << 11)+(1<<8)+(1 << 7)+(1<<5)+(1 << 4)+(1<<0);
    
    while(1)
    {
        GPIOB->ODR |= (1 << 3);        
       // SAI1_Block_A->DR=0xefffa0a8;
        GPIOB->ODR &= ~(1 << 3);
        delay(1000);

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
    // SAI1 has an independent PLL.  Its input clock speed is 4MHz after initClocks is called (MSI clock)
    // This is multiplied up by 62 and divided by 2 to give 124MHz.

    RCC->PLLSAI1CFGR = (2 << 27)+(37  << 8)+(1 << 16); // set SAI1 PLL output 74MHz - can this be true - meant to be less than 80MHz
    RCC->CR |= (1 << 26); // turn on SAI1 PLL
    SAI1_Block_A->CR2 = 0;
    SAI1_Block_A->CR1 = (7 << 5) + (3 << 20) + (1 << 9); // 32 bit frame  + add in a divider to get 47.9kHz sampling rate + set clock edge
    SAI1_Block_A->SLOTR |= (1 << 8) + (1 << 16)+(1 << 17)+(1 << 7);    
    // 32 bit frame length.  FS selected BEFORE MSB of slot 0.  15+1 clock cycles in FS signal (L/R)
    SAI1_Block_A->FRCR = (1 << 18) + (1 << 16)+(15<<8)+ 32-1; 
    SAI1_Block_A->CR1 |= (1 << 17); // enable DMA mode
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
	// If BIT17 = 1 then divisor is 17; 320/17 = 18.82MHz : ok (PLLP used by SAI2)
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
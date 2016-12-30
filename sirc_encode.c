/*
 * author: Mehmet ASLAN
 * date: December 16, 2016
 *
 * no warranty, no licence agreement
 * use it at your own risk
 */

#include "sirc_encode.h"
#include "stm32f0xx.h"

/* stm32f0 irtim used */
#define TIM_CAR TIM17
#define TIM_ENV TIM16

/* private variables */
static uint8_t reg = 0;
static uint16_t frame;
static uint8_t encode_lock;
static uint8_t frame_end_flag;
static uint8_t biti; 		/* bit iterator, when 12 done */

void sirc_encode_init(void)
{
	/* apb2 freq = 48 Mhz */
	/* clocks */
        RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
        RCC->APB2ENR |= RCC_APB2ENR_TIM16EN | RCC_APB2ENR_TIM17EN;

        /* af0 pb9 */
        GPIOB->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR9;

        GPIOB->AFR[1] &= ~0xf0;

        /* timer car pwm1 constant update 36khz %25 duty*/
        TIM_CAR->PSC = 0;
        TIM_CAR->ARR = 1333;

        TIM_CAR->CR1 |= TIM_CR1_ARPE;

        TIM_CAR->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE;
        TIM_CAR->CCR1 = (1333 / 4);

        /* timer env pwm1 one pulse mode ue_int 1 Mhz */
        TIM_ENV->PSC = 47;
        TIM_ENV->ARR = 1;
        /* only timer overflow creates update interrupt*/
        TIM_ENV->CR1 |= TIM_CR1_ARPE | TIM_CR1_OPM | TIM_CR1_URS;
        TIM_ENV->DIER |= TIM_DIER_UIE;

        TIM_ENV->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE;

        NVIC_EnableIRQ(TIM16_IRQn);
        NVIC_SetPriority(TIM16_IRQn, 0);

        /* enable outputs */
        TIM_CAR->CCER |= TIM_CCER_CC1E;
        TIM_ENV->CCER |= TIM_CCER_CC1E;

        TIM_CAR->EGR |= TIM_EGR_UG;
        TIM_ENV->EGR |= TIM_EGR_UG;

        TIM_CAR->BDTR |= TIM_BDTR_MOE;
        TIM_ENV->BDTR |= TIM_BDTR_MOE;
}

int8_t sirc_encode(uint8_t cmd, uint8_t addr)
{
	/* another frame is being send */
	if (encode_lock)
		return -1;

	frame = 0;
	biti = 0;

	frame |= (cmd & 0x7f);
	frame |= ((addr & 0x1f) << 7);

	/* start timer */
        GPIOB->MODER &= ~GPIO_MODER_MODER9;
        GPIOB->MODER |= GPIO_MODER_MODER9_1;

        TIM_ENV->ARR = 3000;
        TIM_ENV->CCR1 = 2400;

        TIM_ENV->EGR |= TIM_EGR_UG;
        TIM_ENV->CR1 |= TIM_CR1_CEN;
        TIM_CAR->CR1 |= TIM_CR1_CEN;

	encode_lock = 1;

	return 0;
}

static void set_timer(uint16_t v)
{
	TIM_ENV->ARR = v;
	/* update */
	TIM_ENV->EGR |= TIM_EGR_UG;

	TIM_ENV->CR1 |= TIM_CR1_CEN;
}

void TIM16_IRQHandler(void)
{
	/* dont have to check because I didnt set any other interrupt */
	TIM_ENV->SR &= ~TIM_SR_UIF;

        if (frame_end_flag) {
                frame_end_flag = 0;
                encode_lock = 0;
                return;
        }
	
	if (biti != 12) {
		/* write next bit to timer */
		if (frame & BIT16_0)
			TIM_ENV->CCR1 = 1200;
		else
			TIM_ENV->CCR1 = 600;

		set_timer(TIM_ENV->CCR1 + 600);

                frame >>= 1;
                ++biti;
	} else {
		/* one frame send put ir led idle */
		GPIOB->MODER &= ~GPIO_MODER_MODER9;
                GPIOB->MODER |= GPIO_MODER_MODER9_0;
                GPIOB->ODR |= GPIO_ODR_9;

		TIM_CAR->CR1 &= ~TIM_CR1_CEN;

		/* wait a little so receiver handles */
	        set_timer(FRAME_END_DELAY);

		frame_end_flag = 1;
	}
}

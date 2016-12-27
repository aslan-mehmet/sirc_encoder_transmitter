#include "sirc_encode.h"
#include "stm32f0xx.h"

#define TIM_CAR TIM17
#define TIM_ENV TIM16

uint16_t sirc_frame;
uint8_t sirci;
uint8_t sirc_encode_lock = 0;

/* this function error prone some reset values assumed
 * when setting peripheral registers
 */
void sirc_encode_init(void)
{
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

        /* timer env pwm1 opm ue_int 1 Mhz */
        TIM_ENV->PSC = 47;
        TIM_ENV->ARR = 1;
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


void sirc_encode(uint8_t cmd, uint8_t addr)
{
        if (sirc_encode_lock)
                return;

        /* encode logic */
        sirc_frame = 0;
        sirci = 0;

        sirc_frame |= (cmd & 0x7f);
        sirc_frame |= ((addr & 0x1f) << 7);

        /* start timer */
        GPIOB->MODER &= ~GPIO_MODER_MODER9;
        GPIOB->MODER |= GPIO_MODER_MODER9_1;

        TIM_ENV->ARR = 3000;
        TIM_ENV->CCR1 = 2400;

        TIM_ENV->EGR |= TIM_EGR_UG;
        TIM_ENV->CR1 |= TIM_CR1_CEN;
        TIM_CAR->CR1 |= TIM_CR1_CEN;

        sirc_encode_lock = 1;
}

void TIM16_IRQHandler(void)
{
        /* one pulse update handling */
        TIM_ENV->SR &= ~TIM_SR_UIF;

        if (sirci != 12) {
                /* write new values to timers */
                if (sirc_frame & 1)
                        TIM_ENV->CCR1 = 1200;
                else
                        TIM_ENV->CCR1 = 600;

                TIM_ENV->ARR = TIM_ENV->CCR1 + 600;
                /* update */
                TIM_ENV->EGR |= TIM_EGR_UG;

                TIM_ENV->CR1 |= TIM_CR1_CEN;

                sirc_frame >>= 1;
                ++sirci;
        } else {
                /* packet end */
                GPIOB->MODER &= ~GPIO_MODER_MODER9;
                GPIOB->MODER |= GPIO_MODER_MODER9_0;
                GPIOB->ODR |= GPIO_ODR_9;

                TIM_CAR->CR1 &= ~TIM_CR1_CEN;

                sirc_encode_lock = 0;
        }


}

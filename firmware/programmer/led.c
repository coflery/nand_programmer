/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "led.h"
#include <stm32f10x.h>

void led_init()
{
    GPIO_InitTypeDef led_gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, 1);

    led_gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    led_gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    led_gpio.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOC, &led_gpio);

    GPIO_SetBits(GPIOC, GPIO_Pin_6 | GPIO_Pin_7);
}

static void led_set(GPIO_TypeDef *gpiox, uint16_t pin, bool on)
{
    if (on)
        GPIO_SetBits(gpiox, pin);
    else
        GPIO_ResetBits(gpiox, pin);
}

void led_wr_set(bool on)
{
    led_set(GPIOC, GPIO_Pin_6, on);
}

void led_rd_set(bool on)
{
    led_set(GPIOC, GPIO_Pin_7, on);
}


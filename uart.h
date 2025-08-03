/*
 * uart.h
 *
 *  Created on: Nov 14, 2022
 *      Author: dans
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "stm32h7xx_hal.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <lwrb.h>

#define RING_BUFFER_SIZE 100

typedef struct
{
	UART_HandleTypeDef *phuart;

	lwrb_t lwrb;
	uint8_t ring_buffer[RING_BUFFER_SIZE];
}myUART_t;



void setup_uart(myUART_t * myUARTHander, UART_HandleTypeDef * huart);

uint8_t is_buffer_empty(myUART_t * myUARTHander);
int read_buffer_until(myUART_t * myUARTHander, uint8_t terminator, uint8_t * res);

void send_uart(myUART_t * myUARTHander, const char *format, ...);




#endif /* INC_UART_H_ */

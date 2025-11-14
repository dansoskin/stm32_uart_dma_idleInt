/*
 * uart.c
 *
 *  Created on: Nov 14, 2022
 *      Author: dans
 */

#include <uart.h>


//---------------------------------------------------------------------------
//--------------------------- GENERAL----------------------------------------
//---------------------------------------------------------------------------
//keep all the addresses of the instances of myuart for use in the callback
#define POINTER_ARRAY_SIZE 10
myUART_t * array_of_pointers[POINTER_ARRAY_SIZE] = {0};
uint8_t array_of_pointers_idx = 0;

volatile uint8_t sending_dma_busy = 0;


//this is the general callback fired at any uart transaction
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(Size == 0)
		return;

	HAL_UART_DMAStop(huart);

	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
	{
		if(0 == array_of_pointers[i])
			break;

		if(array_of_pointers[i]->phuart == huart)
		{
			lwrb_write(&array_of_pointers[i]->lwrb, array_of_pointers[i]->pDMABuffer, Size);
			HAL_UARTEx_ReceiveToIdle_DMA(huart, array_of_pointers[i]->pDMABuffer, array_of_pointers[i]->dma_buffer_size);
		}
	}

}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart->ErrorCode & HAL_UART_ERROR_PE)
	{
	// Parity error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_PEF);
	}
	if (huart->ErrorCode & HAL_UART_ERROR_FE)
	{
	// Frame error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF);
	}
	if(huart->ErrorCode & HAL_UART_ERROR_NE)
	{
	  // Noise error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_NEF);
	}
	if (huart->ErrorCode & HAL_UART_ERROR_ORE)
	{
	// Overrun error occurred
	  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF); // Clear Overrun Error Flag
	}
	if (huart->ErrorCode & HAL_UART_ERROR_DMA)
	{
	// DMA transfer error occurred
	}
	// Handle other error types as needed

	HAL_UART_DMAStop(huart);

	//restart the DMA
	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
	{
		if(0 == array_of_pointers[i])
			break;

		if(array_of_pointers[i]->phuart == huart)
		{
			HAL_UARTEx_ReceiveToIdle_DMA(huart, array_of_pointers[i]->pDMABuffer, array_of_pointers[i]->dma_buffer_size);
		}
	}

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
//	for(int i = 0; i < POINTER_ARRAY_SIZE; i++)
//	{
//		if(0 == array_of_pointers[i])
//			break;
//
//		if(array_of_pointers[i]->phuart == huart)
//		{
//			array_of_pointers[i]->sending_dma_busy = 0;
//		}
//	}

	sending_dma_busy = 0;
}
//---------------------------------------------------------------------------



//-----------------------------------------------------------------------
void setup_uart(myUART_t * myUARTHander, UART_HandleTypeDef * huart, uint32_t ring_buffer_size, uint32_t dma_buffer_size)
{
	myUARTHander->phuart = huart;
	myUARTHander->ring_buffer_size = ring_buffer_size;
	myUARTHander->dma_buffer_size = dma_buffer_size;

	myUARTHander->pRingBuffer = malloc(ring_buffer_size);
	myUARTHander->pDMABuffer = malloc(dma_buffer_size);

	if((myUARTHander->pRingBuffer == NULL) || (myUARTHander->pDMABuffer == NULL))
	{
		while(1);	//error
	}
	memset(myUARTHander->pRingBuffer, 0 , ring_buffer_size);
	memset(myUARTHander->pDMABuffer, 0 , dma_buffer_size);

	__HAL_UART_CLEAR_OREFLAG(huart);
	__HAL_UART_CLEAR_IDLEFLAG(huart);
	__HAL_UART_FLUSH_DRREGISTER(huart);
	HAL_UARTEx_ReceiveToIdle_DMA(huart, myUARTHander->pDMABuffer, myUARTHander->dma_buffer_size);

	//receive interrupts
	__HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT | DMA_IT_TC);

	//transfer interrupts
//    __HAL_DMA_DISABLE_IT(phuart->hdmatx, DMA_IT_HT);
//    __HAL_DMA_ENABLE_IT(phuart->hdmatx, DMA_IT_TC);

	//--------------------- ring buffer
	lwrb_init(&myUARTHander->lwrb, myUARTHander->pRingBuffer, myUARTHander->ring_buffer_size); // Initialize ring buffer

	//--------------------- save to array
	array_of_pointers[array_of_pointers_idx] = myUARTHander;
	array_of_pointers_idx++;
}

//-------------------------------------------------------------------------
uint32_t bytes_in_buffer(myUART_t * myUARTHander)
{
	int total_items_in_buffer = lwrb_get_full(&myUARTHander->lwrb);
	return total_items_in_buffer;
}

//returns number of bytes in result
int read_buffer_until(myUART_t * myUARTHander, uint8_t terminator, uint8_t * res, uint32_t res_size)
{
	int total_items_in_buffer = lwrb_get_full(&myUARTHander->lwrb);
	if(total_items_in_buffer)
	{
		//if there's something in the buffer, peek inside the buffer
		int peeked_number = lwrb_peek(&myUARTHander->lwrb, 0, res, total_items_in_buffer);

		//look for terminator character
		int i;
		for(i = 0; i < peeked_number; i++)
		{
			if(*(res + i) == terminator)  // Dereference the pointer to compare byte values
				break;
		}

		if(i == total_items_in_buffer)
			return 0;

		lwrb_skip(&myUARTHander->lwrb, i+1);
		return i + 1;
	}

	return 0;
}



void send_uart(myUART_t * myUARTHander, const char *format, ...)
{
	char buffer[256]; // Adjust the buffer size as needed
	va_list args;     // Declare a variable of type va_list

	va_start(args, format); // Initialize args to store all values after format
	int len = vsnprintf(buffer, sizeof(buffer), format, args); // Format the string with the arguments
	va_end(args); // Clean up the va_list variable

	HAL_UART_Transmit(myUARTHander->phuart, (uint8_t *) buffer, len, 0xFFFF);
}

void send_uart_dma(myUART_t * myUARTHander, const char *format, ...)
{
	static char buffer[256]; // Make static so it persists during DMA transfer
	va_list args;

	va_start(args, format);
	int len = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	
	// Ensure we don't exceed buffer size
//	if (len >= sizeof(buffer)) {
//		len = sizeof(buffer) - 1;
//	}

	while(sending_dma_busy) { __asm__("nop");}
	sending_dma_busy = 1;
	HAL_UART_Transmit_DMA(myUARTHander->phuart, (uint8_t *) buffer, len);
}

//----------------------------------------------------------------------------

/*
 * usage example:
 * char result[10][20] = {0};	//10 values of 20 characters
 * int count = split_csv_string((char*)input, result, ",\n");
 */

int split_csv_string(const char *input, char result[][20], const char *delimiter)
{
    // need to use a copy of the input to avoid modifying the original string
    if (input == NULL || result == NULL || delimiter == NULL) {
        return 0; // Invalid input
    }

    char *input_copy = strdup(input);
    int count = 0;

    char * token = strtok(input_copy, delimiter);

    while (token != NULL && count < 10)
    {
        strncpy(result[count], token, 19);
        result[count][19] = '\0'; // Ensure null-termination
        count++;
        token = strtok(NULL, delimiter);
    }

    free(input_copy);
    return count;
}

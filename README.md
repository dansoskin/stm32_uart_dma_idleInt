# STM32 UART DMA Idle Interrupt Helper

This library wraps STM32 HAL UART reception with `HAL_UARTEx_ReceiveToIdle_DMA()` and stores received bytes in an `lwrb` ring buffer for later parsing.

It is intended for variable-length UART packets such as line-based protocols.

## DMA Configuration

- mode: `DMA_NORMAL`
- memory increment: enabled
- peripheral increment: disabled
- data width: byte


## Setup Example

```c
#include "uart.h"

#define UART_RING_BUFFER_SIZE 200U
#define UART_DMA_BUFFER_SIZE  100U

myUART_t myUart;

static uint8_t uart_ring_buffer[UART_RING_BUFFER_SIZE];
static uint8_t uart_dma_buffer[UART_DMA_BUFFER_SIZE];

void setup(void)
{
    if (!setup_uart(&myUart,
                    &huart3,
                    uart_ring_buffer,
                    sizeof(uart_ring_buffer),
                    uart_dma_buffer,
                    sizeof(uart_dma_buffer)))
    {
        Error_Handler();
    }
}
```

## Listener Example

This example reads line-based messages terminated by `'\n'`.

```c
void uart_listener(void)
{
    uint32_t bytes = bytes_in_buffer(&myUart);
    if (bytes > 0U)
    {
        uint8_t input[100] = {0};
        int len = read_buffer_until(&myUart, '\n', input, sizeof(input));

        if (len > 0)
        {
            send_uart(&myUart, "UART<< %s\n", input);
        }
    }
}
```

## Important Note For STM32H7

On STM32H7, the DMA buffer must live in a DMA-accessible RAM region.

Do not place the UART RX DMA buffer in DTCM RAM.

One simple approach is to place RX DMA buffers in a dedicated linker section, for example:

```ld
.dma_buffer (NOLOAD) :
{
  . = ALIGN(4);
  *(.dma_buffer)
  *(.dma_buffer*)
  . = ALIGN(4);
} >RAM_D2
```

Then declare the DMA buffer like this:

```c
__attribute__((section(".dma_buffer"))) static uint8_t uart_dma_buffer[100];
```

## Credits

- Ring buffer implementation: [MaJerle/lwrb](https://github.com/MaJerle/lwrb)

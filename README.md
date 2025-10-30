# STM32 UART DMA Idle Interrupt Example

This project demonstrates efficient UART reception on STM32 microcontrollers using DMA (Direct Memory Access) and the UART idle line interrupt. It enables reception of variable-length data packets and places the received data into a ring buffer for easy processing.

The ring buffer implementation is based on the excellent [lwrb](https://github.com/MaJerle/lwrb) library by MaJerle.

---

## Features

- **DMA-based UART reception:** Offloads data transfer from CPU.
- **Idle line interrupt:** Detects end of variable-length packets without fixed-size framing.
- **Ring buffer storage:** Seamless buffering of incoming data, even at high speeds.
- **Configurable buffer sizes:** Easily adjust DMA and ring buffer sizes to fit your application.
- **Multi-UART support:** supports multiple UART instances.

---

## How It Works

1. **DMA is configured** in circular mode to continuously receive UART data into a memory buffer.
2. **UART idle line interrupt** triggers when a pause in data is detected, indicating the end of a packet.
3. **Received data is transferred** from the DMA buffer into a ring buffer for further processing.
4. **User code reads data** from the ring buffer as needed, supporting variable-length packets.

---

## Configuration

### DMA Settings

- **Mode:** Circular (overwrites buffer on overflow)
- **Memory increment:** Enabled
- **Data width:** Byte

### Buffer Sizes

- **DMA buffer size:** Default is 100 bytes. Change `DMA_RX_BUFFER_SIZE` in `uart.c` to adjust.
- **Ring buffer size:** Default is 100 bytes. Change `RING_BUFFER_SIZE` in `uart.h` to adjust.

---

## Usage Example

```c
#include "uart.h"

int main(void)
{
    myUART_t myUart;
    setup_uart(&myUart, &huart3, 200, 100);

    while (1)
    {
        uart_listener();
    }
}

void uart_listener(void)
{
	static uint8_t first_read = 1;
	uint32_t bytes = bytes_in_buffer(&myUart);
    if (bytes > 0)
    {
    	if(first_read) {myUart.lwrb.r = myUart.lwrb.w; first_read = 0; return;}

        uint8_t input[100] = {0};
        int len = read_buffer_until(&myUart, '\n', input, 100);

        if(len > 0)
        {
        	if(input[0] == '^')
			{
				 char result[10][20] = {0};	//10 values of 20 characters
				 int count = split_csv_string((char*)input, result, ",\r\n");
				 UNUSED(count);
				 __asm__("nop");
			}

			send_uart(&myUart, "UART<< %s\n", input);
        }
    }
}
```

---


## Credits

- Ring buffer implementation: [MaJerle/lwrb](https://github.com/MaJerle/lwrb)


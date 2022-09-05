/* 
 * "Small Hello World" example. 
 * 
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example 
 * designs. It requires a STDOUT  device in your system's hardware. 
 *
 * The purpose of this example is to demonstrate the smallest possible Hello 
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard 
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete 
 * description.
 * 
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION 
 *      This removes software exception handling, which means that you cannot 
 *      run code compiled for Nios II cpu with a hardware multiplier on a core 
 *      without a the multiply unit. Check the Nios II Software Developers 
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks  
 *      support for buffering, file IO, floating point and getch(), etc. 
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program 
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include "sys/alt_stdio.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "io.h"
#include "system.h"

#define UART_CLK 50000000
#define UART_BASE REG32_0_BASE

#define UART_CONTROL_REG 0x0
#define UART_STATUS_REG 0x1
#define UART_TX_REG 0x2
#define UART_RX_REG 0x3

#define UART_PARITY_NONE 0x0
#define UART_PARITY_ON 0x1
#define UART_PARITY_BIT_MASK 0x00000100
#define UART_PARITY_BIT_OFFSET 8

#define UART_8_BIT_FRAME 0x8
#define UART_N_DATA_BITS_MASK 0x00003C00
#define UART_N_DATA_BITS_OFFSET 10

#define UART_STOP_SINGLE 0x0
#define UART_START_DOUBLE 0x1
#define UART_STOP_BIT_MASK 0x00000200
#define UART_STOP_BIT_OFFSET 9

#define UART_CLK_DIV_MASK 0xFFFF0000
#define UART_CLK_DIV_OFFSET 16
#define UART_BAUD_RATE(rate) (UART_CLK/rate)

#define UART_ENABLE 0x7
#define UART_DISABLE 0x0
#define UART_ENABLE_MASK 0x7
#define UART_ENABLE_OFFSET 0

#define MSG_SIZE 32
/*
 * Enables uart operation with 115200 baud rate, 0 parity bits, 1 stop bit.
 */

int read_uart_register(int addr)
{
  return IORD(UART_BASE, addr);
}
void write_uart_register(int addr, int val)
{
  IOWR(UART_BASE, addr, val);
}


void uart_set_default_config(void)
{
  int control =
  (
    (UART_PARITY_NONE << UART_PARITY_BIT_OFFSET) |
	(UART_STOP_SINGLE << UART_STOP_BIT_OFFSET) |
	(UART_BAUD_RATE(115200) << UART_CLK_DIV_OFFSET) |
	(UART_ENABLE << UART_ENABLE_OFFSET) |
	(UART_8_BIT_FRAME << UART_N_DATA_BITS_OFFSET)
  );

  write_uart_register(UART_CONTROL_REG, control);
}

void uart_set_baud_rate(int rate)
{
  int control = read_uart_register(UART_CONTROL_REG);
  control &= (~UART_CLK_DIV_MASK);
  control |= (UART_BAUD_RATE(rate) << UART_CLK_DIV_OFFSET);
  write_uart_register(UART_CONTROL_REG, control);
}

int uart_poll(char* data)
{
  int read = read_uart_register(UART_STATUS_REG);
  if (read & 0x80000000)
  {
    *data = (char) read_uart_register(UART_RX_REG);
    return 0;
  }
  return 1;
}

void uart_write(char data)
{
	write_uart_register(UART_TX_REG, (int) data);
}

void print_uart(char *buf)
{
  int msg_len = strlen(buf);

  for (int i = 0; i < msg_len; i++)
  {
    uart_write(buf[i]);
  }
}

void delay(int ms)
{
  volatile int i = 0;
  volatile int j = 0;
  for (i = 0; i < ms; i++)
  {
    for (j = 0; j < 500; j++)
    {

    }
  }
}

void read_n_bytes_from_uart(char* buff, int size)
{
  int count = 0;
  while(count < size)
  {
    int read = read_uart_register(UART_STATUS_REG);
    if (read & 0x80000000)
    {
       int var = read_uart_register(UART_RX_REG);
       buff[count] = (char) var;
       count++;
    }
  }
}

int main()
{ 
  alt_putstr("Hello from reg32!\n");

	char in_char;
	char msg_buf[MSG_SIZE + 1];
	int msg_buf_pos = 0;

	uart_set_default_config();
	//uart_set_baud_rate(460800);

	while (true) {
		/*if (uart_poll(&in_char) == 0){
			//uart_write(in_char);
			write_uart_register(UART_TX_REG, (int) in_char);
		}*/
		if (uart_poll(&in_char) == 0) {
			if (in_char == '\n' || in_char == '\r') {

				msg_buf[msg_buf_pos] = '\0';

				int rate = atoi(msg_buf);
				if (rate != 0) {
					print_uart("ACK");
					delay(1000);
					uart_set_baud_rate(rate);
				} else {
					print_uart(msg_buf);
				}

				msg_buf_pos = 0;
			} else {
				if (msg_buf_pos < MSG_SIZE) {
					msg_buf[msg_buf_pos++] = in_char;
				} else {
					print_uart("ERROR: msg too long. Character dropped\r\n");
				}
			}
		}
	}
}

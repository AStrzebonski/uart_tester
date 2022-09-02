#include <zephyr/drivers/uart.h>

#include <string.h>
#include <stdlib.h>

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define MSG_SIZE 32
#define CHANGE_BAUD_MSG "change baudrate to "

static const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/*
 * Print a null-terminated string character by character to the UART interface
 */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}
}

void main(void)
{
	char in_char;
	char msg_buf[MSG_SIZE + 1];
	int msg_buf_pos = 0;
	struct uart_config cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};
	int err;

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return;
	}

	/* indefinitely wait for input from the user */
	while (true) {
		if (uart_poll_in(uart_dev, &in_char) == 0) {
			if (in_char == '\n' || in_char == '\r') {
				/* Terminate string */
				msg_buf[msg_buf_pos] = '\0';

				if (!strncmp(msg_buf, CHANGE_BAUD_MSG, strlen(CHANGE_BAUD_MSG))) {
					int rate = atoi(&msg_buf[strlen(CHANGE_BAUD_MSG)]);
					if (rate == 0) {
						print_uart("ERROR: received incorrect baudrate\r\n");
						return;
					}

					cfg.baudrate = rate;
					err = uart_configure(uart_dev, &cfg);
					if (err) {
						print_uart("ERROR: can not set baudrate\r\n");
						return;
					}
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

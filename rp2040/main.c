#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "device/usbd.h"
#include "class/hid/hid_device.h"

#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define SYNC1 0xAA
#define SYNC2 0x55

int main(void)
{
    stdio_init_all();

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    tusb_init();

    uint8_t report[8];
    uint8_t index = 0;
    uint8_t state = 0;

    while (true)
    {
        tud_task();

        while (uart_is_readable(UART_ID))
        {
            uint8_t byte = uart_getc(UART_ID);

            if (state == 0)
            {
                if (byte == SYNC1)
                {
                    state = 1;
                }
            }
            else if (state == 1)
            {
                if (byte == SYNC2)
                {
                    index = 0;
                    state = 2;
                }
                else if (byte == SYNC1)
                {
                    state = 1;
                }
                else
                {
                    state = 0;
                }
            }
            else
            {
                report[index++] = byte;

                if (index == 8)
                {
                    if (tud_hid_ready())
                    {
                        tud_hid_report(0, report, 8);
                    }

                    index = 0;
                    state = 0;
                }
            }
        }

        sleep_ms(1);
    }
}

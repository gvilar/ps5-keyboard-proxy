#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "device/usbd.h"
#include "class/hid/hid_device.h"

#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

int main(void)
{
    stdio_init_all();

    // UART : Pi TX -> RP2040 GPIO1 (RX)
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // USB HID
    tusb_init();

    uint8_t report[8];
    uint8_t index = 0;

    while (true)
    {
        tud_task();

        while (uart_is_readable(UART_ID))
        {
            report[index++] = uart_getc(UART_ID);

            if (index == 8)
            {
                if (tud_hid_ready())
                {
                    tud_hid_keyboard_report(
                        0,
                        report[0],
                        &report[2]
                    );
                }

                index = 0;
            }
        }

        sleep_ms(1);
    }
}

#include "tusb.h"
#include "device/usbd.h"
#include "class/hid/hid_device.h"

enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID 0x81

uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const desc_device[] = {
    0x12, 0x01,
    0x00, 0x02,
    0x00, 0x00, 0x00,
    0x40,
    0x8A, 0x2E,
    0x0A, 0x00,
    0x01, 0x02,
    0x00,
    0x01
};

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(
        1,
        ITF_NUM_TOTAL,
        0,
        CONFIG_TOTAL_LEN,
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
        100
    ),

    TUD_HID_DESCRIPTOR(
        ITF_NUM_HID,
        4,
        HID_ITF_PROTOCOL_KEYBOARD,
        sizeof(desc_hid_report),
        EPNUM_HID,
        8,
        10
    )
};

uint16_t _desc_str[32];

uint8_t const *tud_descriptor_device_cb(void)
{
    return desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return desc_hid_report;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    const char *str;

    switch (index) {
        case 0:
            _desc_str[1] = 0x0409;
            return _desc_str;

        case 1:
            str = "Raspberry Pi";
            break;

        case 2:
            str = "UART Keyboard";
            break;

        case 3:
            str = "0001";
            break;

        default:
            return NULL;
    }

    uint8_t len = 0;

    while (str[len] && len < 31) {
        _desc_str[1 + len] = str[len];
        len++;
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * len + 2);

    return _desc_str;
}

void tud_hid_set_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t const *buffer,
    uint16_t bufsize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

uint16_t tud_hid_get_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t *buffer,
    uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}


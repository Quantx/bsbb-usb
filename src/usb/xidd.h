#ifndef BSBB_XIDD_H
#define BSBB_XIDD_H

#include <cstdint>
#include <tusb.h>

#include "xid.h"
#include "../mj9k/mj9k.h"

#define XIDD_DRIVER_MAX 1
#define XIDD_EP_BUFFER_BUFSIZE 32

static const tusb_desc_device_t XID_DESC_DEVICE = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0110,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0x0A7B,
    .idProduct = 0xD000,
    .bcdDevice = 0x0121,

    .iManufacturer = 0x00,
    .iProduct = 0x00,
    .iSerialNumber = 0x00,

    .bNumConfigurations = 0x01
};

#define TUD_XID_SB_DESC_LEN (sizeof(tusb_desc_interface_t) + 2 * sizeof(tusb_desc_endpoint_t))
#define TUD_XID_SB_DESCRIPTOR(_itfnum, _epout, _epin) \
    /* Interface */\
    sizeof(tusb_desc_interface_t), TUSB_DESC_INTERFACE, _itfnum, 0, 2, XID_INTERFACE_CLASS, XID_INTERFACE_SUBCLASS, 0x00, 0x00,\
    /* Endpoint In */\
    sizeof(tusb_desc_endpoint_t), TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4, \
    /* Endpoint Out */\
    sizeof(tusb_desc_endpoint_t), TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4

#define TUD_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_XID_SB_DESC_LEN)

static uint8_t const XID_DESC_CONFIGURATION[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, XIDD_DRIVER_MAX, 0, TUD_CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface id, EP OUT, EP IN
    TUD_XID_SB_DESCRIPTOR(0, 0x01, 0x82),
};

// These reports should match up to the interfaces listed in XID_DESC_CONFIGURATION
static const xid_descriptor_t _xidd_reports_table[XIDD_DRIVER_MAX] = {
    {
        .bLength = 0x10,
        .bDescriptorType = 0x42,
        .bcdXid = 0x100,
        .bType = XID_TYPE_MJ9K,
        .bSubType = XID_SUBTYPE_MJ9K,
        .bMaxInputReportSize = 0x1A,
        .bMaxOutputReportSize = 0x16,
        .wAlternateProductIds = {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
    }
};

// TinyUSB Device Driver interface functions
void xidd_init(void);
bool xidd_deinit(void);
void xidd_reset(uint8_t rhport);
uint16_t xidd_open(uint8_t rhport, tusb_desc_interface_t const * desc_itf, uint16_t max_len);
bool xidd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request);
bool xid_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);

bool xidd_is_interface_open(uint8_t itf_num);
bool xidd_send_report(uint8_t itf_num, void * report, uint8_t report_len);

#endif

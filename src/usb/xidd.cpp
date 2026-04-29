#include <cstdint>
#include <cstdio>
#include <cstring>
#include <tusb.h>
#include <usbd_pvt.h>
#include "xidd.h"

#define ITF_NUM_MAX 0xFF

typedef struct {
    uint8_t rhport;
    uint8_t itf_num;
    const xid_descriptor_t * descriptor;

    uint8_t ep_in;
    uint8_t ep_out;
} xidd_interface_t;

// This struct has special alignment requirements
typedef struct {
    TUH_EPBUF_DEF(ep_in, XIDD_EP_BUFFER_BUFSIZE); // Data transmited to XBOX
    TUH_EPBUF_DEF(ep_out, XIDD_EP_BUFFER_BUFSIZE); // Data received from XBOX
} xidd_epbuf_t;

xidd_interface_t _xidd_itf_table[XIDD_DRIVER_MAX];
xidd_epbuf_t _xidd_epbuf_table[XIDD_DRIVER_MAX]; // Must be declared separately due to alignment requirements

static xidd_interface_t * get_xid_itf(uint8_t itf_num) {
    for (uint8_t i = 0; i < XIDD_DRIVER_MAX; i++) {
        if (_xidd_itf_table[i].itf_num == itf_num) {
            return &_xidd_itf_table[i];
        }
    }
    return NULL;
}

static xidd_epbuf_t * get_xid_buf(uint8_t itf_num) {
    for (uint8_t i = 0; i < XIDD_DRIVER_MAX; i++) {
        if (_xidd_itf_table[i].itf_num == itf_num) {
            return &_xidd_epbuf_table[i];
        }
    }
    return NULL;
}

static uint8_t get_itf_from_ep(uint8_t ep_addr) {
    for (uint8_t i = 0; i < XIDD_DRIVER_MAX; i++) {
        if (_xidd_itf_table[i].ep_in == ep_addr || _xidd_itf_table[i].ep_out == ep_addr) {
            return _xidd_itf_table[i].itf_num;
        }
    }
    return ITF_NUM_MAX;
}

bool xidd_is_interface_open(uint8_t itf_num) {
    return get_xid_itf(itf_num) != NULL;
}

static inline xidd_interface_t * find_new_driver(void) {
    return get_xid_itf(ITF_NUM_MAX);
}

void xidd_init(void) {
    xidd_reset(0);
}

bool xidd_deinit(void) {
    xidd_reset(0);
    return true;
}

void xidd_reset(uint8_t rhport) {
    (void) rhport;

    for (uint8_t i = 0; i < XIDD_DRIVER_MAX; i++) {
        _xidd_itf_table[i].itf_num = ITF_NUM_MAX;
        _xidd_itf_table[i].descriptor = &_xidd_reports_table[i];
    }
}

uint16_t xidd_open(uint8_t rhport, tusb_desc_interface_t const * desc_itf, uint16_t max_len) {
    if (desc_itf->bInterfaceClass != XID_INTERFACE_CLASS || desc_itf->bInterfaceSubClass != XID_INTERFACE_SUBCLASS) {
        return 0; // Not an Xbox Controller itf_num
    }

    printf("[DEV] XID opening interface %u\r\n", desc_itf->bInterfaceNumber);

    if (desc_itf->bNumEndpoints != 2) {
        printf("[DEV] XID invalid number of endpoints %u, expected 2\r\n", desc_itf->bNumEndpoints);
        return 0;
    }


    const uint16_t drv_len = (uint16_t)(TUD_XID_SB_DESC_LEN);
    if (drv_len > max_len) {
        printf("[DEV] XID expected descriptor length %u exceeds max length %u\r\n", drv_len, max_len);
        return 0;
    }

    xidd_interface_t * xid_itf = find_new_driver();
    if (!xid_itf) {
        printf("[DEV] XID failed to find new driver\r\n");
        return 0;
    }

    const tusb_desc_endpoint_t * desc_ep_a = (const tusb_desc_endpoint_t *)tu_desc_next(desc_itf);
    const tusb_desc_endpoint_t * desc_ep_b = (const tusb_desc_endpoint_t *)tu_desc_next(desc_ep_a);

    tusb_dir_t dir_ep_a = tu_edpt_dir(desc_ep_a->bEndpointAddress);
    tusb_dir_t dir_ep_b = tu_edpt_dir(desc_ep_b->bEndpointAddress);

    if (dir_ep_a == TUSB_DIR_IN) {
        if (dir_ep_b == TUSB_DIR_IN) {
            printf("[DEV] XID both endpoints are inputs\r\n");
            return 0;
        }

        xid_itf->ep_in = desc_ep_a->bEndpointAddress;
        xid_itf->ep_out = desc_ep_b->bEndpointAddress;
    } else {
        if (dir_ep_b == TUSB_DIR_OUT) {
            printf("[DEV] XID both endpoints are outputs\r\n");
            return 0;
        }

        xid_itf->ep_in = desc_ep_b->bEndpointAddress;
        xid_itf->ep_out = desc_ep_a->bEndpointAddress;
    }

    if (!usbd_edpt_open(rhport, desc_ep_a)) {
        printf("[DEV] XID Cannot open endpoint address 0x%02X\r\n", desc_ep_a->bEndpointAddress);
        return 0;
    }


    if (!usbd_edpt_open(rhport, desc_ep_b)) {
        printf("[DEV] XID Cannot open endpoint address 0x%02X\r\n", desc_ep_b->bEndpointAddress);
        usbd_edpt_close(rhport, desc_ep_a);
        return 0;
    }

    xid_itf->rhport = rhport;

    // Set the itf_num last
    xid_itf->itf_num = desc_itf->bInterfaceNumber;
    return drv_len;
}

static uint16_t xid_descriptor_write_capabilities(const xid_descriptor_t * descriptor, bool out, uint8_t * buf, uint16_t buf_len) {
    uint8_t report_size = out ? descriptor->bMaxOutputReportSize : descriptor->bMaxInputReportSize;
    if (buf_len < report_size + 2) {
        return 0;
    }

    buf[0] = 0x00;
    buf[1] = report_size;

    memset(&buf[2], 0xFF, report_size);
    return report_size + 2;
}

static void xidd_send_report_cb(uint8_t itf_num, xfer_result_t result, void * ep_in, uint8_t xferred_bytes) {
    printf("[DEV] XID Transfer callback (IN) for interface %u\r\n", itf_num);
}

bool xidd_receive_report(uint8_t itf_num) {
    xidd_interface_t * xid_itf = get_xid_itf(itf_num);
    if (!xid_itf) {
        printf("[DEV] XID Receive Report cannot open interface %u\r\n", itf_num);
        return false;
    }

    if (!usbd_edpt_claim(xid_itf->rhport, xid_itf->ep_out)) {
        printf("[DEV] XID Receive Report failed to claim interface %u output endpoint 0x%02X\r\n", itf_num, xid_itf->ep_out);
        return false;
    }

    xidd_epbuf_t * xid_epbuf = get_xid_buf(itf_num);

    if (!usbd_edpt_xfer(xid_itf->rhport, xid_itf->ep_out, xid_epbuf->ep_out, xid_itf->descriptor->bMaxOutputReportSize, false)) {
        printf("[DEV] XID Receive Report interface %u endpoint 0x%02X trasnfer failed\r\n", itf_num, xid_itf->ep_out);
        return false;
    }

    return true;
}

static void xidd_receive_report_cb(uint8_t itf_num, xfer_result_t result, void * ep_out, uint8_t xferred_bytes) {
    printf("[DEV] XID Transfer callback (OUT) for interface %u\r\n", itf_num);

    xidd_interface_t * xid_itf = get_xid_itf(itf_num);
    if (!xid_itf) {
        printf("[DEV] XID failed to get driver for interface %u\r\n", itf_num);
        return;
    }

    if (result == XFER_RESULT_SUCCESS) {
        const xid_descriptor_t * descriptor = xid_itf->descriptor;
        if (descriptor->bType == XID_TYPE_MJ9K && descriptor->bSubType == XID_SUBTYPE_MJ9K) {
            mj9k_virtual::handle_out_report(itf_num, ep_out, xferred_bytes);
        }
    }

    // Recevie the next report
    xidd_receive_report(itf_num);
}

bool xidd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    (void) rhport;

    if (request->bmRequestType_bit.recipient != TUSB_REQ_RCPT_INTERFACE) {
        return false;
    }

    uint8_t itf_num = (uint8_t)request->wIndex;

    xidd_interface_t * xid_itf = get_xid_itf(itf_num);
    if (!xid_itf) {
        printf("[DEV] XID failed to get driver for interface %u\r\n", itf_num);
        return false;
    }

    xidd_epbuf_t * xid_epbuf = get_xid_buf(itf_num);
    if (!xid_epbuf) {
        printf("[DEV] XID failed to get endpoint buffers for interface %u\r\n", itf_num);
        return false;
    }

    uint16_t length = request->wLength < XIDD_EP_BUFFER_BUFSIZE ? request->wLength : XIDD_EP_BUFFER_BUFSIZE;

    bool ret = false;

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS) {
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
            // 0xA1
            if (request->bRequest == 0x01 && request->wValue == 0x0100) {
                // GET_REPORT
                if (stage != CONTROL_STAGE_SETUP) return true;

                // Just send an empty report, since this is only used once followed by normal transfers
                memset(xid_epbuf->ep_in, 0, length);
                ret = tud_control_xfer(rhport, request, xid_epbuf->ep_in, length);
                printf("[DEV] XID sent GET_REPORT\r\n");
            }
        } else {
            // 0x21
            if (request->bRequest == 0x09 && request->wValue == 0x0200) {
                // SET_REPORT
                printf("[DEV] XID got SET_REPORT\r\n");
            }
        }
    } else if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR) {
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
            // 0xC1
            if (request->bRequest == 0x06 && request->wValue == 0x4200) {
                // GET_DESCRIPTOR
                if (stage != CONTROL_STAGE_SETUP) return true;

                // Convert descriptor to USB byte order
                xid_descriptor_t xid_descriptor;
                memcpy(&xid_descriptor, xid_itf->descriptor, sizeof(xid_descriptor_t));
                xid_descriptor.bcdXid = tu_htole16(xid_descriptor.bcdXid);
                for (uint8_t i = 0; i < XID_ALTERNATE_PRODUCT_ID_COUNT; i++) {
                    xid_descriptor.wAlternateProductIds[i] = tu_htole16(xid_descriptor.wAlternateProductIds[i]);
                }

                memcpy(xid_epbuf->ep_in, &xid_descriptor, sizeof(xid_descriptor_t));
                ret = tud_control_xfer(rhport, request, xid_epbuf->ep_in, sizeof(xid_descriptor_t));
                printf("[DEV] XID sent GET_DESCRIPTOR\r\n");

                xidd_receive_report(itf_num);
            } else if (request->bRequest == 0x01) {
                // GET_CAPABILITIES
                if (stage != CONTROL_STAGE_SETUP) return true;

                if (request->wValue == 0x0100) {
                    // CAPABILITIES IN
                    uint16_t buf_len = xid_descriptor_write_capabilities(xid_itf->descriptor, false, xid_epbuf->ep_in, length);
                    if (!buf_len) {
                        printf("[DEV] XID failed to write capabilities (IN)\r\n");
                        return false;
                    }

                    ret = tud_control_xfer(rhport, request, xid_epbuf->ep_in, buf_len);
                    printf("[DEV] XID sent GET_CAPABILITIES IN\r\n");
                } else if (request->wValue == 0x0200) {
                    // CAPABILITIES OUT
                    uint16_t buf_len = xid_descriptor_write_capabilities(xid_itf->descriptor, true, xid_epbuf->ep_in, length);
                    if (!buf_len) {
                        printf("[DEV] XID failed to write capabilities (OUT)\r\n");
                        return false;
                    }

                    ret = tud_control_xfer(rhport, request, xid_epbuf->ep_in, buf_len);
                    printf("[DEV] XID sent GET_CAPABILITIES OUT\r\n");
                }
            }
        }
    }

    if (!ret) {
        printf("[DEV] XID unhandled control xfer: Stage: %u, Request Type 0x%02X, Request: 0x%02X, Value 0x%04X, Index 0x%04X, Length 0x%04X\r\n",
               stage, request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength);
    }
    return ret;
}

bool xidd_send_report(uint8_t itf_num, void * report, uint8_t report_len) {
    xidd_interface_t * xid_itf = get_xid_itf(itf_num);
    if (!xid_itf) {
        printf("[DEV] XID Send Report cannot open device for interface %u\r\n", itf_num);
        return false;
    }

    if (report_len > xid_itf->descriptor->bMaxInputReportSize) {
        printf("[DEV] XID Send Report length %u exceeds max input report size of %u\r\n", report_len, xid_itf->descriptor->bMaxInputReportSize);
        return false;
    }

    if (tud_suspended())
        tud_remote_wakeup();

    if (!usbd_edpt_claim(xid_itf->rhport, xid_itf->ep_in)) {
        printf("[DEV] XID Send Report failed to claim interface %u endpoint 0x%02X\r\n", itf_num, xid_itf->ep_in);
        return false;
    }

    xidd_epbuf_t * xid_epbuf = get_xid_buf(itf_num);
    memcpy(xid_epbuf->ep_in, report, report_len);

    if (!usbd_edpt_xfer(xid_itf->rhport, xid_itf->ep_in, xid_epbuf->ep_in, report_len, false)) {
        printf("[DEV] XID Send Report interface %u endpoint 0x%02X trasnfer failed\r\n", itf_num, xid_itf->ep_in);
        return false;
    }

    return true;
}

bool xid_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    (void) rhport;

    uint8_t itf_num = get_itf_from_ep(ep_addr);

    xidd_epbuf_t * xid_epbuf = get_xid_buf(itf_num);
    if (!xid_epbuf) {
        printf("[DEV] XID failed to get endpoint buffers for interface %u\r\n", itf_num);
        return false;
    }

    tusb_dir_t dir_ep = tu_edpt_dir(ep_addr);
    if (dir_ep == TUSB_DIR_IN) {
        xidd_send_report_cb(itf_num, result, xid_epbuf->ep_in, (uint8_t)xferred_bytes);
    } else {
        xidd_receive_report_cb(itf_num, result, xid_epbuf->ep_out, (uint8_t)xferred_bytes);
    }

    return true;
}

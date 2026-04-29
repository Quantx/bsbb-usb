#include <stdio.h>
#include <tusb.h>
#include <usbh_pvt.h>
#include <cstdint>

#include "xid.h"
#include "xidh.h"
#include "../mj9k/mj9k.h"

typedef struct {
    uint8_t daddr;
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    uint8_t port;

    bool mounted;
    uint8_t type;
    uint8_t sub_type;
    uint8_t max_input_report_size;
    uint8_t max_output_report_size;
} xidh_interface_t;

// This struct has special alignment requirements
typedef struct {
    TUH_EPBUF_DEF(ep_in, XIDH_EP_BUFFER_BUFSIZE);
    TUH_EPBUF_DEF(ep_out, XIDH_EP_BUFFER_BUFSIZE);
} xidh_epbuf_t;

xidh_interface_t _xidh_itf_table[XIDH_DRIVER_MAX];
xidh_epbuf_t _xidh_epbuf_table[XIDH_DRIVER_MAX]; // Must be declared separately due to alignment requirements

static xidh_interface_t * get_xid_itf(uint8_t daddr) {
    for (uint8_t i = 0; i < XIDH_DRIVER_MAX; i++) {
        if (_xidh_itf_table[i].daddr == daddr) {
            return &_xidh_itf_table[i];
        }
    }
    return NULL;
}

static xidh_epbuf_t * get_xid_buf(uint8_t daddr) {
    for (uint8_t i = 0; i < XIDH_DRIVER_MAX; i++) {
        if (_xidh_itf_table[i].daddr == daddr) {
            return &_xidh_epbuf_table[i];
        }
    }
    return NULL;
}

static inline xidh_interface_t * find_new_driver(void) {
    return get_xid_itf(0);
}

uint8_t xidh_dev_addr_from_port(uint8_t port) {
    if (!port) return 0; // Port cannot be zero

    for (uint8_t i = 0; i < XIDH_DRIVER_MAX; i++) {
        if (_xidh_itf_table[i].port == port) {
            return _xidh_itf_table[i].daddr;
        }
    }

    return 0;
}

bool xidh_get_type(uint8_t daddr, uint8_t * type, uint8_t * sub_type) {
    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        return false;
    }

    if (type) *type = xid_itf->type;
    if (sub_type) *sub_type = xid_itf->sub_type;

    return true;
}

bool xidh_get_port(uint8_t daddr, uint8_t * port) {
    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        return false;
    }

    if (port) *port = xid_itf->port;

    return true;
}

bool xidh_is_mounted(uint8_t daddr) {
    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        return false;
    }

    return xid_itf->mounted;
}

bool xidh_send_report(uint8_t daddr, void * report, uint8_t report_len) {
    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        printf("[%u] XID Send Report cannot open device\r\n", daddr);
        return false;
    }

    if (report_len > xid_itf->max_output_report_size) {
        printf("[%u] XID Send Report length %u exceeds max output report size of %u\r\n", daddr, report_len, xid_itf->max_output_report_size);
        return false;
    }

    if (!usbh_edpt_claim(daddr, xid_itf->ep_out)) {
        printf("[%u] XID Send Report failed to claim output endpoint\r\n", daddr);
        return false;
    }

    xidh_epbuf_t * xid_epbuf = get_xid_buf(daddr);

    memcpy(&xid_epbuf->ep_out, report, report_len);

    if (!usbh_edpt_xfer(daddr, xid_itf->ep_out, xid_epbuf->ep_out, report_len)) {
        printf("[%u] XID Send Report transfer failed\r\n", daddr);
        usbh_edpt_release(daddr, xid_itf->ep_out);
        return false;
    }

    return true;
}

static void xidh_send_report_cb(uint8_t daddr, xfer_result_t result, void * ep_out, uint8_t xferred_bytes) {
    printf("[%u] XID Transfer callback OUT\r\n", daddr);
}

bool xidh_receive_report(uint8_t daddr) {
    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        printf("[%u] XID Receive Report cannot open device\r\n", daddr);
        return false;
    }

    if (!usbh_edpt_claim(daddr, xid_itf->ep_in)) {
        printf("[%u] XID Receive Report failed to claim input endpoint\r\n", daddr);
        return false;
    }

    xidh_epbuf_t * xid_epbuf = get_xid_buf(daddr);

    if (!usbh_edpt_xfer(daddr, xid_itf->ep_in, xid_epbuf->ep_in, xid_itf->max_input_report_size)) {
        printf("[%u] XID Receive Report transfer failed\r\n", daddr);
        return false;
    }

    return true;
}

static void xidh_receive_report_cb(uint8_t daddr, xfer_result_t result, void * ep_in, uint8_t xferred_bytes) {
    printf("[%u] XID Transfer callback IN\r\n", daddr);

    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        printf("[%u] XID Receive Report Callback cannot open device\r\n", daddr);
        return;
    }

    if (result == XFER_RESULT_SUCCESS) {
        if (xid_itf->type == XID_TYPE_MJ9K && xid_itf->sub_type == XID_SUBTYPE_MJ9K) {
            mj9k_physical::handle_in_report(daddr, ep_in, xferred_bytes);
        }
    }

    // Recevie the next report
    xidh_receive_report(daddr);
}

static void xidh_parse_descriptor(tuh_xfer_t * xfer) {
    if (xfer->result != XFER_RESULT_SUCCESS) {
        printf("Failed to get XID Descriptor, bad response\r\n");
        return;
    }

    const uint8_t daddr = xfer->daddr;
    xid_descriptor_t * desc_xid = (xid_descriptor_t *)xfer->buffer;

    if (desc_xid->bLength != sizeof(xid_descriptor_t)) {
        printf("[%u] XID Descriptor length was %u not %u\r\n", daddr, desc_xid->bLength, sizeof(xid_descriptor_t));
        return;
    }

    if (desc_xid->bDescriptorType != XID_DESCRIPTOR_TYPE) {
        printf("[%u] XID Descriptor Type was 0x%02X not 0x%02X\r\n", daddr, desc_xid->bDescriptorType, XID_DESCRIPTOR_TYPE);
        return;
    }

    printf("[%u] XID Report Version: %04X\r\n", daddr, desc_xid->bcdXid);
    printf("[%u] XID Report Type: 0x%02X, Sub-Type: 0x%02X\r\n", daddr, desc_xid->bType, desc_xid->bSubType);
    printf("[%u] XID Report Size Max: Input %u, Output %u\r\n", daddr, desc_xid->bMaxInputReportSize, desc_xid->bMaxOutputReportSize);

    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        printf("[%u] XID Descriptor parsed, but driver interface not found\r\n", daddr);
        return;
    }

    uint8_t port = 0;
    {
        tuh_bus_info_t bus_info;
        if (tuh_bus_info_get(daddr, &bus_info)) {
            port = bus_info.hub_port;
            printf("[%u] XID Hub Port: %u\r\n", daddr, port);
        } else {
            printf("[%u] XID Failed to get Hub Port\r\n", daddr);
        }
    }

    xid_itf->port = port;

    xid_itf->type = desc_xid->bType;
    xid_itf->sub_type = desc_xid->bSubType;

    xid_itf->max_input_report_size = desc_xid->bMaxInputReportSize;
    xid_itf->max_output_report_size = desc_xid->bMaxOutputReportSize;

    xid_itf->mounted = true;

    // notify usbh that driver enumeration is complete
    usbh_driver_set_config_complete(daddr, xid_itf->itf_num);

    // Receive the first report
    xidh_receive_report(daddr);
}

static inline void reset_xid_itf(void) {
    memset(_xidh_itf_table, 0, sizeof(_xidh_itf_table));
}

bool xidh_init(void) {
    reset_xid_itf();
    return true;
}

bool xidh_deinit(void) {
    reset_xid_itf();
    return true;
}

uint16_t xidh_open(uint8_t rhport, uint8_t daddr, const tusb_desc_interface_t * desc_itf, uint16_t max_len) {
    if (desc_itf->bInterfaceClass != XID_INTERFACE_CLASS || desc_itf->bInterfaceSubClass != XID_INTERFACE_SUBCLASS) {
        return 0; // Not an Xbox Controller itf_num
    }

    printf("[%u] XID opening interface %u\r\n", daddr, desc_itf->bInterfaceNumber);

    if (desc_itf->bNumEndpoints != 2) {
        printf("[%u] XID invalid number of endpoints %u, expected 2\r\n", daddr, desc_itf->bNumEndpoints);
        return 0;
    }

    const uint16_t drv_len = (uint16_t)(sizeof(tusb_desc_interface_t) + 2 * sizeof(tusb_desc_endpoint_t));
    if (drv_len > max_len) {
        printf("[%u] XID expected descriptor length %u exceeds max length %u\r\n", daddr, drv_len, max_len);
        return 0;
    }

    xidh_interface_t * xid_itf = find_new_driver();
    if (!xid_itf) {
        printf("[%u] XID Cannot open device, all drivers interfaces in use\r\n", daddr);
        return 0;
    }

    const tusb_desc_endpoint_t * desc_ep_a = (const tusb_desc_endpoint_t *)tu_desc_next(desc_itf);
    const tusb_desc_endpoint_t * desc_ep_b = (const tusb_desc_endpoint_t *)tu_desc_next(desc_ep_a);

    tusb_dir_t dir_ep_a = tu_edpt_dir(desc_ep_a->bEndpointAddress);
    tusb_dir_t dir_ep_b = tu_edpt_dir(desc_ep_b->bEndpointAddress);

    if (dir_ep_a == TUSB_DIR_IN) {
        if (dir_ep_b == TUSB_DIR_IN) {
            printf("[%u] XID both endpoints are inputs\r\n", daddr);
            return 0;
        }

        xid_itf->ep_in = desc_ep_a->bEndpointAddress;
        xid_itf->ep_out = desc_ep_b->bEndpointAddress;
    } else {
        if (dir_ep_b == TUSB_DIR_OUT) {
            printf("[%u] XID both endpoints are outputs\r\n", daddr);
            return 0;
        }

        xid_itf->ep_in = desc_ep_b->bEndpointAddress;
        xid_itf->ep_out = desc_ep_a->bEndpointAddress;
    }

    if (!tuh_edpt_open(daddr, desc_ep_a)) {
        printf("[%u] XID Cannot open endpoint address 0x%02X\r\n", daddr, desc_ep_a->bEndpointAddress);
        return 0;
    }


    if (!tuh_edpt_open(daddr, desc_ep_b)) {
        printf("[%u] XID Cannot open endpoint address 0x%02X\r\n", daddr, desc_ep_b->bEndpointAddress);
        tuh_edpt_close(daddr, desc_ep_a);
        return 0;
    }

    xid_itf->daddr = daddr;
    xid_itf->itf_num = desc_itf->bInterfaceNumber;

    return drv_len;
}

bool xidh_set_config(uint8_t daddr, uint8_t itf_num) {
    tusb_control_request_t const request = {
        .bmRequestType_bit = {
            .recipient = TUSB_REQ_RCPT_INTERFACE,
            .type      = TUSB_REQ_TYPE_VENDOR,
            .direction = TUSB_DIR_IN
        }, // 0xC1
        .bRequest = TUSB_REQ_GET_DESCRIPTOR,
        .wValue   = tu_htole16(tu_u16(XID_DESCRIPTOR_TYPE, 0)),
        .wIndex   = tu_htole16(itf_num),
        .wLength  = tu_htole16(sizeof(xid_descriptor_t)),
    };

    uint8_t * enum_buf = usbh_get_enum_buf();
    tuh_xfer_t xfer = {
        .daddr       = daddr,
        .ep_addr     = 0,
        .setup       = &request,
        .buffer      = enum_buf,
        .complete_cb = xidh_parse_descriptor,
        .user_data   = 0,
    };

    return tuh_control_xfer(&xfer);
}

bool xidh_xfer_cb(uint8_t daddr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    xidh_epbuf_t * xid_epbuf = get_xid_buf(daddr);
    if (!xid_epbuf) {
        printf("[%u] XID Transfer callback, driver not found for this address\r\n", daddr);
        return false;
    }

    tusb_dir_t dir_ep = tu_edpt_dir(ep_addr);
    if (dir_ep == TUSB_DIR_IN) {
        xidh_receive_report_cb(daddr, result, xid_epbuf->ep_in, (uint8_t)xferred_bytes);
    } else {
        xidh_send_report_cb(daddr, result, xid_epbuf->ep_out, (uint8_t)xferred_bytes);
    }

    return true;
}

void xidh_close(uint8_t daddr) {
    xidh_interface_t * xid_itf = get_xid_itf(daddr);
    if (!xid_itf) {
        // This is called regardless of whether or not the driver is open
        return;
    }

    printf("[%u] XID driver closed\r\n", daddr);
    memset(xid_itf, 0, sizeof(xidh_interface_t));
}

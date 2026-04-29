#include <tusb.h>
#include <usbh_pvt.h>
#include <usbd_pvt.h>
#include <Arduino.h>
#include <cstdint>

#include "usb.h"
#include "xidd.h"
#include "xidh.h"

void usb_device_isr(void) {
    tusb_int_handler(BOARD_TUD_RHPORT, true);
}

void usb_host_isr(void) {
    tusb_int_handler(BOARD_TUH_RHPORT, true);
}

void usb_setup(void) {
    printf("* USB Host Stack Initialization Start\r\n");

    usb_platform_setup();

    // init host stack on configured roothub port
    tusb_rhport_init_t host_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUH_RHPORT, &host_init);

    // init device stack on configured roothub port
    tusb_rhport_init_t device_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_FULL,
    };
    tusb_init(BOARD_TUD_RHPORT, &device_init);

    printf("* USB Host Stack Initialization Complete\r\n");
}

void usb_loop(void) {
    tud_task();
    tuh_task();
}

void tuh_mount_cb(uint8_t daddr) {
    // application set-up
    printf("[%u] *** Device Mounted ***\r\n", daddr);

    tuh_bus_info_t bus_info;
    if (tuh_bus_info_get(daddr, &bus_info)) {
        printf("  RHPort %u, Hub Addr: %u, Hub Port: %u, Speed: %u\r\n",
            bus_info.rhport, bus_info.hub_addr, bus_info.hub_port, bus_info.speed);
    }

    tusb_desc_device_t desc_device;
    if (!tuh_descriptor_get_device_local(daddr, &desc_device)) {
        printf("Failed to get cached device descriptor for device %u\r\n", daddr);
    }

    printf("  bDescriptorType     %u\r\n"    , desc_device.bDescriptorType);
    printf("  bcdUSB              %04x\r\n"  , desc_device.bcdUSB);
    printf("  bDeviceClass        %u\r\n"    , desc_device.bDeviceClass);
    printf("  bDeviceSubClass     %u\r\n"    , desc_device.bDeviceSubClass);
    printf("  bDeviceProtocol     %u\r\n"    , desc_device.bDeviceProtocol);
    printf("  bMaxPacketSize0     %u\r\n"    , desc_device.bMaxPacketSize0);
    printf("  idVendor            0x%04x\r\n", desc_device.idVendor);
    printf("  idProduct           0x%04x\r\n", desc_device.idProduct);
    printf("  bcdDevice           %04x\r\n"  , desc_device.bcdDevice);
    printf("  iManufacturer       %u\r\n"    , desc_device.iManufacturer);
    printf("  iProduct            %u\r\n"    , desc_device.iProduct);
    printf("  iSerialNumber       %u\r\n"    , desc_device.iSerialNumber);
    printf("  bNumConfigurations  %u\r\n"    , desc_device.bNumConfigurations);
}

void tuh_umount_cb(uint8_t daddr) {
    // application tear-down
    printf("[%u] *** Device Unmounted ***\r\n", daddr);
}

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *)&XID_DESC_DEVICE;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return XID_DESC_CONFIGURATION;
}

uint16_t const * tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)index;
    (void)langid;
    return NULL;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    // This may need to be expanded at some point to service different driver types, but for now it's whatever'
    return xidd_control_xfer_cb(rhport, stage, request);
}

#define BSBB_HOST_DRIVER_COUNT 1
usbh_class_driver_t const bsbb_host_drivers[BSBB_HOST_DRIVER_COUNT] = {
    { // XID Host Driver
        .name = "XID Host",
        .init = xidh_init,
        .deinit = xidh_deinit,
        .open = xidh_open,
        .set_config = xidh_set_config,
        .xfer_cb = xidh_xfer_cb,
        .close = xidh_close,
    },
};

usbh_class_driver_t const * usbh_app_driver_get_cb(uint8_t * driver_count) {
    *driver_count = BSBB_HOST_DRIVER_COUNT;
    return bsbb_host_drivers;
}

#define BSBB_DEVICE_DRIVER_COUNT 1
usbd_class_driver_t const bsbb_device_drivers[BSBB_DEVICE_DRIVER_COUNT] = {
    {
        .name = "XID Device",
        .init = xidd_init,
        .deinit = xidd_deinit,
        .reset = xidd_reset,
        .open = xidd_open,
        .control_xfer_cb = xidd_control_xfer_cb,
        .xfer_cb = xid_xfer_cb,
    },
};


usbd_class_driver_t const * usbd_app_driver_get_cb(uint8_t * driver_count) {
    *driver_count = BSBB_DEVICE_DRIVER_COUNT;
    return bsbb_device_drivers;
}

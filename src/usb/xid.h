#ifndef BSBB_XID_H
#define BSBB_XID_H

#include <cstdint>

// XID Common
#define XID_INTERFACE_CLASS 88
#define XID_INTERFACE_SUBCLASS 66

// Corresponds to: xid_descriptor_t.bDescriptorType
#define XID_DESCRIPTOR_TYPE 0x42

#define XID_ALTERNATE_PRODUCT_ID_COUNT 4
typedef struct  __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdXid;
    uint8_t bType;
    uint8_t bSubType;
    uint8_t bMaxInputReportSize;
    uint8_t bMaxOutputReportSize;
    uint16_t wAlternateProductIds[XID_ALTERNATE_PRODUCT_ID_COUNT];
} xid_descriptor_t;



#endif

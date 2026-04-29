#ifndef BSBB_XIDH_H
#define BSBB_XIDH_H

#include <tusb.h>

// TinyUSB Host Driver interface functions
#define XIDH_DRIVER_MAX 2
#define XIDH_EP_BUFFER_BUFSIZE 128

bool xidh_init(void);
bool xidh_deinit(void);
uint16_t xidh_open(uint8_t rhport, uint8_t dev_addr, const tusb_desc_interface_t * itf_desc, uint16_t max_len);
bool xidh_set_config(uint8_t dev_addr, uint8_t itf_num);
bool xidh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
void xidh_close(uint8_t dev_addr);

// TinyUSB Host Driver user functions
uint8_t xidh_dev_addr_from_port(uint8_t port); // Returns 0 if no XID device is assigned to the given port
bool xidh_get_type(uint8_t daddr, uint8_t * type, uint8_t * sub_type);
bool xidh_get_port(uint8_t daddr, uint8_t * port);
bool xidh_is_mounted(uint8_t daddr);

bool xidh_send_report(uint8_t daddr, void * report, uint8_t report_len);
bool xidh_receive_report(uint8_t daddr);

#endif

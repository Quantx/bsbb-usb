#include <tusb.h>
#include <Arduino.h>
#include "usb.h"

extern "C" {
    uint32_t tusb_time_millis_api(void) {
        return millis();
    }
}

// WARNING: Here be dragons, do not touch this
void usb_platform_setup(void) {
    // USB Device Interface Init
    printf("  USB1 Reset\r\n");
    NVIC_DISABLE_IRQ(IRQ_USB1);
    USB1_USBCMD |= USB_USBCMD_RST;
    while (USB1_USBCMD & USB_USBCMD_RST);
    NVIC_CLEAR_PENDING(IRQ_USB1);
    printf("  USB1 Reset complete\r\n");
    delay(10);
    attachInterruptVector(IRQ_USB1, &usb_device_isr);
    NVIC_ENABLE_IRQ(IRQ_USB1);
    printf("  USB1 ISR Attached\r\n");

    // Borrowed from: https://github.com/PaulStoffregen/USBHost_t36/blob/master/ehci.cpp#L167
    printf("  USB2 PLL Starting\r\n");
    while (1) {
        uint32_t n = CCM_ANALOG_PLL_USB2;
        if (n & CCM_ANALOG_PLL_USB2_DIV_SELECT) {
            CCM_ANALOG_PLL_USB2_CLR = 0xC000; // get out of 528 MHz mode
            CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_BYPASS;
            CCM_ANALOG_PLL_USB2_CLR = CCM_ANALOG_PLL_USB2_POWER |
            CCM_ANALOG_PLL_USB2_DIV_SELECT |
            CCM_ANALOG_PLL_USB2_ENABLE |
            CCM_ANALOG_PLL_USB2_EN_USB_CLKS;
            continue;
        }
        if (!(n & CCM_ANALOG_PLL_USB2_ENABLE)) {
            CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_ENABLE; // enable
            continue;
        }
        if (!(n & CCM_ANALOG_PLL_USB2_POWER)) {
            CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_POWER; // power up
            continue;
        }
        if (!(n & CCM_ANALOG_PLL_USB2_LOCK)) {
            continue; // wait for lock
        }
        if (n & CCM_ANALOG_PLL_USB2_BYPASS) {
            CCM_ANALOG_PLL_USB2_CLR = CCM_ANALOG_PLL_USB2_BYPASS; // turn off bypass
            continue;
        }
        if (!(n & CCM_ANALOG_PLL_USB2_EN_USB_CLKS)) {
            CCM_ANALOG_PLL_USB2_SET = CCM_ANALOG_PLL_USB2_EN_USB_CLKS; // enable
            continue;
        }
        break;
    }
    printf("  USB2 PLL Running\r\n");

    // turn on USB clocks (should already be on)
    CCM_CCGR6 |= CCM_CCGR6_USBOH3(CCM_CCGR_ON);
    // turn on USB2 PHY
    USBPHY2_CTRL_CLR = USBPHY_CTRL_SFTRST | USBPHY_CTRL_CLKGATE;
    USBPHY2_CTRL_SET = USBPHY_CTRL_ENUTMILEVEL2 | USBPHY_CTRL_ENUTMILEVEL3;
    USBPHY2_PWD = 0;
    #ifdef ARDUINO_TEENSY41
    IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_40 = 5;
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_40 = 0x0008; // slow speed, weak 150 ohm drive
    GPIO8_GDIR |= 1<<26;
    GPIO8_DR_SET = 1<<26;
    #endif
    printf("  USB2 PHY Enabled\r\n");
    delay(10);

    NVIC_DISABLE_IRQ(IRQ_USB2);
    attachInterruptVector(IRQ_USB2, usb_host_isr);
    NVIC_ENABLE_IRQ(IRQ_USB2);

    printf("  USB2 ISR Attached\r\n");
}

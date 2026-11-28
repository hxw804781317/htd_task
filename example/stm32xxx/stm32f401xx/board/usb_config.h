#ifndef _CHERRYUSB_CONFIG_H_
#define _CHERRYUSB_CONFIG_H_

/*Cherry-USB 配置文件*/
#include "htd_common.h"

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


//////////////////////////////////////////////////////////////////////
/* ================ USB common Configuration ================ */
#define CONFIG_USB_PRINTF(...)          htd_print(__VA_ARGS__)

#ifndef CONFIG_USB_DBG_LEVEL
    #define CONFIG_USB_DBG_LEVEL        USB_DBG_INFO
#endif

/* Enable print with color */
#define CONFIG_USB_PRINTF_COLOR_ENABLE

/* data align size when use dma */
#ifndef CONFIG_USB_ALIGN_SIZE
    #define CONFIG_USB_ALIGN_SIZE       4
#endif

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION         __attribute__((section(".noncacheable")))


/* use usb_memcpy default for high performance but cost more flash memory.
 * And, arm libc has a bug that memcpy() may cause data misalignment when the size is not a multiple of 4.
*/
//#define CONFIG_USB_MEMCPY_DISABLE


//////////////////////////////////////////////////////////////////////
/* ================= USB Device Stack Configuration ================ */
/* Ep0 max transfer buffer, specially for receiving data from ep0 out */
#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
    #define CONFIG_USBDEV_REQUEST_BUFFER_LEN    256
#endif

/* Send ep0 in data from user buffer instead of copying into ep0 reqdata
 * Please note that user buffer must be aligned with CONFIG_USB_ALIGN_SIZE
*/
// #define CONFIG_USBDEV_EP0_INDATA_NO_COPY

/* Check if the input descriptor is correct */
// #define CONFIG_USBDEV_DESC_CHECK

/* Enable test mode */
// #define CONFIG_USBDEV_TEST_MODE

/* enable advance desc register api */
#define CONFIG_USBDEV_ADVANCE_DESC

/* move ep0 setup handler from isr to thread */
// #define CONFIG_USBDEV_EP0_THREAD
#ifdef CONFIG_USBDEV_EP0_THREAD
    #ifndef CONFIG_USBDEV_EP0_PRIO
        #define CONFIG_USBDEV_EP0_PRIO          4
    #endif
    #ifndef CONFIG_USBDEV_EP0_STACKSIZE
        #define CONFIG_USBDEV_EP0_STACKSIZE     2048
    #endif
#endif




//////////////////////////////////////////////////////////////////////
/* move msc read & write from isr to while(1), you should call usbd_msc_polling in while(1) */
#define CONFIG_USBDEV_MSC_POLLING

/* move msc read & write from isr to thread */
//#define CONFIG_USBDEV_MSC_THREAD
#ifdef CONFIG_USBDEV_MSC_THREAD
    #ifndef CONFIG_USBDEV_MSC_PRIO
        #define CONFIG_USBDEV_MSC_PRIO          4
    #endif
    #ifndef CONFIG_USBDEV_MSC_STACKSIZE
        #define CONFIG_USBDEV_MSC_STACKSIZE     2048
    #endif
#endif

#ifdef BSP_USBD_MSC_DEV_COUNT
    #define CONFIG_USBDEV_MSC_MAX_LUN           BSP_USBD_MSC_DEV_COUNT
#endif
#ifndef CONFIG_USBDEV_MSC_MAX_LUN
    #define CONFIG_USBDEV_MSC_MAX_LUN           1
#endif
#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
    #define CONFIG_USBDEV_MSC_MAX_BUFSIZE       512
#endif
#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
    #define CONFIG_USBDEV_MSC_MANUFACTURER_STRING ""
#endif
#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
    #define CONFIG_USBDEV_MSC_PRODUCT_STRING    ""
#endif
#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
    #define CONFIG_USBDEV_MSC_VERSION_STRING    "0.01"
#endif



//////////////////////////////////////////////////////////////////////
#define CONFIG_USBDEV_MTP_THREAD
#ifdef CONFIG_USBDEV_MTP_THREAD
    #ifndef CONFIG_USBDEV_MTP_PRIO
        #define CONFIG_USBDEV_MTP_PRIO          4
    #endif
    #ifndef CONFIG_USBDEV_MTP_STACKSIZE
        #define CONFIG_USBDEV_MTP_STACKSIZE     4096
    #endif
#endif

#ifndef CONFIG_USBDEV_MTP_MAX_BUFSIZE
    #define CONFIG_USBDEV_MTP_MAX_BUFSIZE       2048
#endif
#ifndef CONFIG_USBDEV_MTP_MAX_OBJECTS
    #define CONFIG_USBDEV_MTP_MAX_OBJECTS       256
#endif
#ifndef CONFIG_USBDEV_MTP_MAX_PATHNAME
    #define CONFIG_USBDEV_MTP_MAX_PATHNAME      256
#endif







//////////////////////////////////////////////////////////////////////
#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
    #define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE    156
#endif
/* rndis transfer buffer size, must be a multiple of (1536 + 44)*/
#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
    #define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE  1580
#endif
#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
    #define CONFIG_USBDEV_RNDIS_VENDOR_ID       0x0000ffff
#endif
#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
    #define CONFIG_USBDEV_RNDIS_VENDOR_DESC     "CherryUSB"
#endif

#define CONFIG_USBDEV_RNDIS_USING_LWIP
#define CONFIG_USBDEV_CDC_ECM_USING_LWIP






//////////////////////////////////////////////////////////////////////
/* ================ USB Device Port Configuration ================*/
#ifndef CONFIG_USBDEV_MAX_BUS
    #if defined(BSP_USBD_MSC_DEV_COUNT)
        #define CONFIG_USBDEV_MAX_BUS           (1 + BSP_USBD_MSC_DEV_COUNT)
    #else
        #define CONFIG_USBDEV_MAX_BUS           1
    #endif
#endif

// #define CONFIG_USBDEV_SOF_ENABLE

/* ---------------- FSDEV Configuration ---------------- */
//#define CONFIG_USBDEV_FSDEV_PMA_ACCESS 2 // maybe 1 or 2, many chips may have a difference


/* ---------------- MUSB Configuration ---------------- */
#define CONFIG_USB_MUSB_EP_NUM                  8
//#define CONFIG_USB_MUSB_SUNXI



/* ---------------- EHCI Configuration ---------------- */
#define CONFIG_USB_EHCI_HCCR_OFFSET             (0x0)
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE         1024
#define CONFIG_USB_EHCI_QH_NUM                  10
#define CONFIG_USB_EHCI_QTD_NUM                 (CONFIG_USB_EHCI_QH_NUM * 3)
#define CONFIG_USB_EHCI_ITD_NUM                 4
// #define CONFIG_USB_EHCI_HCOR_RESERVED_DISABLE
// #define CONFIG_USB_EHCI_CONFIGFLAG
// #define CONFIG_USB_EHCI_ISO
// #define CONFIG_USB_EHCI_WITH_OHCI
// #define CONFIG_USB_EHCI_DESC_DCACHE_ENABLE

/* ---------------- OHCI Configuration ---------------- */
#define CONFIG_USB_OHCI_HCOR_OFFSET             (0x0)
#define CONFIG_USB_OHCI_ED_NUM                  10
#define CONFIG_USB_OHCI_TD_NUM                  3
// #define CONFIG_USB_OHCI_DESC_DCACHE_ENABLE

/* ---------------- XHCI Configuration ---------------- */
#define CONFIG_USB_XHCI_HCCR_OFFSET             (0x0)



/* ---------------- MUSB Configuration ---------------- */
#define CONFIG_USB_MUSB_PIPE_NUM                8
// #define CONFIG_USB_MUSB_SUNXI
// #define CONFIG_USB_MUSB_WITHOUT_MULTIPOINT

/* When your chip hardware supports high-speed and wants to initialize it in high-speed mode,
 * the relevant IP will configure the internal or external high-speed PHY according to CONFIG_USB_HS.
 *
 * in xxx32 chips, only pb14/pb15 can support hs mode, pa11/pa12 is not supported(only a few supports, but we ignore them).
*/
// #define CONFIG_USB_HS

#ifndef usb_phyaddr2ramaddr
    #define usb_phyaddr2ramaddr(addr)           (addr)
#endif
#ifndef usb_ramaddr2phyaddr
    #define usb_ramaddr2phyaddr(addr)           (addr)
#endif



#ifdef __cplusplus
}
#endif /*__cplusplus*/
#endif /**/


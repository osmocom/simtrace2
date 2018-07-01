#ifndef SIMTRACE_H
#define SIMTRACE_H

#include "ringbuffer.h"
#include "board.h"
#include <usb/device/dfu/dfu.h>

#define BUFLEN  512

#define CLK_MASTER      true
#define CLK_SLAVE       false

/* ===================================================*/
/*                      Taken from iso7816_4.c        */
/* ===================================================*/
/** Flip flop for send and receive char */
#define USART_SEND 0
#define USART_RCV  1


extern volatile ringbuf sim_rcv_buf;

extern volatile bool rcvdChar;
extern volatile uint32_t char_stat;

extern const Pin pinPhoneRST;

enum confNum {
	CFG_NUM_NONE	= 0,
#ifdef HAVE_SNIFFER
	CFG_NUM_SNIFF,
#endif
#ifdef HAVE_CCID
	CFG_NUM_CCID,
#endif
#ifdef HAVE_CARDEM
	CFG_NUM_PHONE,
#endif
#ifdef HAVE_MITM
	CFG_NUM_MITM,
#endif
	NUM_CONF
};

/// CCIDDriverConfiguration Descriptors
/// List of descriptors that make up the configuration descriptors of a
/// device using the CCID driver.
typedef struct {

    /// Configuration descriptor
    USBConfigurationDescriptor configuration;
    /// Interface descriptor
    USBInterfaceDescriptor     interface;
    /// CCID descriptor
    CCIDDescriptor             ccid;
    /// Bulk OUT endpoint descriptor
    USBEndpointDescriptor      bulkOut;
    /// Bulk IN endpoint descriptor
    USBEndpointDescriptor      bulkIn;
    /// Interrupt OUT endpoint descriptor
    USBEndpointDescriptor      interruptIn;
    DFURT_IF_DESCRIPTOR_STRUCT
} __attribute__ ((packed)) CCIDDriverConfigurationDescriptors;

extern const USBConfigurationDescriptor *configurationDescriptorsArr[];

/*! Update USART baud rate to Fi/Di ratio
 *  @param[io] usart USART peripheral base address
 *  @param[in] fidi FiDi value as provided in TA interface byte
 */
void update_fidi(Usart *usart, uint8_t fidi);

void ISR_PhoneRST( const Pin *pPin);

/*  Configure functions   */
extern void Sniffer_configure( void );
extern void CCID_configure( void );
extern void mode_cardemu_configure(void);
extern void MITM_configure( void );

/*  Init functions   */
extern void Sniffer_init( void );
extern void CCID_init( void );
extern void mode_cardemu_init(void);
extern void MITM_init( void );

extern void SIMtrace_USB_Initialize( void );

/*  Exit functions   */
extern void Sniffer_exit( void );
extern void CCID_exit( void );
extern void mode_cardemu_exit(void);
extern void MITM_exit( void );

/*  Run functions   */
extern void Sniffer_run( void );
extern void CCID_run( void );
extern void mode_cardemu_run(void);
extern void MITM_run( void );

/*  IRQ functions   */
extern void Sniffer_usart0_irq(void);
extern void Sniffer_usart1_irq(void);
extern void mode_cardemu_usart0_irq(void);
extern void mode_cardemu_usart1_irq(void);

/*  Timer helper function */
void Timer_Init( void );
void TC0_Counter_Reset( void );

#endif  /*  SIMTRACE_H  */

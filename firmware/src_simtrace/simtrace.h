#ifndef SIMTRACE_H
#define SIMTRACE_H

#include "ringbuffer.h"

/* Endpoint numbers */
#define DATAOUT     1
#define DATAIN      2
#define INT         3

#define BUFLEN  512

#define PHONE_DATAOUT     4
#define PHONE_DATAIN      5
#define PHONE_INT         6

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
    CFG_NUM_SNIFF = 1, CFG_NUM_CCID, CFG_NUM_PHONE, CFG_NUM_MITM, NUM_CONF
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
} __attribute__ ((packed)) CCIDDriverConfigurationDescriptors;

extern const USBConfigurationDescriptor *configurationDescriptorsArr[];

int check_data_from_phone();
void update_fidi(uint8_t fidi);

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

/*  Timer helper function */
void Timer_Init( void );
void TC0_Counter_Reset( void );

int usb_to_host(void);
int usb_from_host(int ep);

#endif  /*  SIMTRACE_H  */

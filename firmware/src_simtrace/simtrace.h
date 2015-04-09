#ifndef SIMTRACE_H
#define SIMTRACE_H

/* Endpoint numbers */
#define DATAOUT     1
#define DATAIN      2
#define INT         3

#define BUFLEN  5

#define PHONE_DATAOUT     4
#define PHONE_DATAIN      5
#define PHONE_INT         6

typedef struct ring_buffer
{
    uint8_t     buf[BUFLEN*2];   // data buffer
    uint8_t     idx;                // number of items in the buffer
} ring_buffer;

extern volatile ring_buffer buf;

extern volatile bool rcvdChar;
extern volatile uint32_t char_stat;
extern volatile enum confNum simtrace_config;

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

/*  Helper functions    */

// FIXME: static function definitions
extern uint32_t _ISO7816_GetChar( uint8_t *pCharToReceive );
extern uint32_t _ISO7816_SendChar( uint8_t CharToSend );

/*  Configure functions   */
extern void Sniffer_configure( void );
extern void CCID_configure( void );
extern void Phone_configure( void );
extern void MITM_configure( void );

/*  Init functions   */
extern void Sniffer_init( void );
extern void CCID_init( void );
extern void Phone_init( void );
extern void MITM_init( void );

extern void SIMtrace_USB_Initialize( void );
extern void _ISO7816_Init( void );

/*  Exit functions   */
extern void Sniffer_exit( void );
extern void CCID_exit( void );
extern void Phone_exit( void );
extern void MITM_exit( void );

/*  Run functions   */
extern void Sniffer_run( void );
extern void CCID_run( void );
extern void Phone_run( void );
extern void MITM_run( void );

/*  Timer helper function */
void Timer_Init( void );
void TC0_Counter_Reset( void );

#endif  /*  SIMTRACE_H  */

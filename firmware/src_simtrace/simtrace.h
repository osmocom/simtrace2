#ifndef SIMTRACE_H
#define SIMTRACE_H

#include "ringbuffer.h"

/* Endpoint numbers */
#define DATAOUT     1
#define DATAIN      2
#define INT         3

#define BUFLEN  64

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
extern volatile enum confNum simtrace_config;

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

/***    PTS parsing     ***/
/* detailed sub-states of ISO7816_S_IN_PTS */
enum pts_state {
	PTS_S_WAIT_REQ_PTSS,
	PTS_S_WAIT_REQ_PTS0,
	PTS_S_WAIT_REQ_PTS1,
	PTS_S_WAIT_REQ_PTS2,
	PTS_S_WAIT_REQ_PTS3,
	PTS_S_WAIT_REQ_PCK,
	PTS_S_WAIT_RESP_PTSS = PTS_S_WAIT_REQ_PTSS | 0x10,
	PTS_S_WAIT_RESP_PTS0 = PTS_S_WAIT_REQ_PTS0 | 0x10,
	PTS_S_WAIT_RESP_PTS1 = PTS_S_WAIT_REQ_PTS1 | 0x10,
	PTS_S_WAIT_RESP_PTS2 = PTS_S_WAIT_REQ_PTS2 | 0x10,
	PTS_S_WAIT_RESP_PTS3 = PTS_S_WAIT_REQ_PTS3 | 0x10,
	PTS_S_WAIT_RESP_PCK = PTS_S_WAIT_REQ_PCK | 0x10,
	PTS_END
};

struct iso7816_3_handle {
	uint8_t fi;
	uint8_t di;

	enum pts_state pts_state;
	uint8_t pts_req[6];
	uint8_t pts_resp[6];
        uint8_t pts_bytes_processed;
};

int check_data_from_phone();
enum pts_state process_byte_pts(struct iso7816_3_handle *ih, uint8_t byte);

void ISR_PhoneRST( const Pin *pPin);

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

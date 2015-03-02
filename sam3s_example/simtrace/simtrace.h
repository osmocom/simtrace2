#ifndef SIMTRACE_H
#define SIMTRACE_H

/* Endpoint numbers */
#define DATAOUT     1
#define DATAIN      2
#define INT         3

#define BUFLEN  64
typedef struct ring_buffer
{
    uint8_t     buf[BUFLEN*2];   // data buffer
    uint8_t     idx;                // number of items in the buffer
} ring_buffer;


enum confNum {
    CFG_NUM_SNIFF = 1, CFG_NUM_PHONE, CFG_NUM_MITM, NUM_CONF
};

// FIXME: static function definitions
extern uint32_t _ISO7816_GetChar( uint8_t *pCharToReceive );
extern uint32_t _ISO7816_SendChar( uint8_t CharToSend );

/*  Init functions   */
extern void Phone_Master_Init( void );
extern void Sniffer_Init( void );
extern void MITM_init( void );

extern void SIMtrace_USB_Initialize( void );
extern void _ISO7816_Init( void );

/*  Run functions   */
extern void Sniffer_run( void );
// extern void CCID_run( void );
extern void Phone_run( void );
extern void MITM_run( void );

#endif  /*  SIMTRACE_H  */

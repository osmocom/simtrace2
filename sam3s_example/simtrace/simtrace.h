#ifndef SIMTRACE_H
#define SIMTRACE_H

// FIXME: static function definitions
extern uint32_t _ISO7816_GetChar( uint8_t *pCharToReceive );
extern uint32_t _ISO7816_SendChar( uint8_t CharToSend );
extern void Phone_Master_Init( void );

extern void Sniffer_Init( void );

extern void SIMtrace_USB_Initialize( void );

extern void _ISO7816_Init( void );

#endif

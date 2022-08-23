#ifndef bsc_protocol_h
#define bsc_protocol_h

#define BSC_CONTROL_SYN     0x32
#define BSC_CONTROL_IDLE    BSC_CONTROL_SYN

#define BSC_CONTROL_SOH     0x01 
#define BSC_CONTROL_STX     0x02
#define BSC_CONTROL_ETB     0x26
#define BSC_CONTROL_ENQ     0x2D
#define BSC_CONTROL_ETX     0x03
#define BSC_CONTROL_DLE     0x10
#define BSC_CONTROL_PAD     0xFF
#define BSC_CONTROL_NAK     0x3D
#define BSC_CONTROL_ITB     0x1F
#define BSC_CONTROL_EOT     0x37

// These are preceeded by a DLE
#define BSC_CONTROL_ACK0    0x70   
#define BSC_CONTROL_ACK1    0x61      // EBCDIC '/'
#define BSC_CONTROL_WACK    0x6B      // EBCDIC ','
#define BSC_CONTROL_RVI     0x7C      // EBCDIC '@'

// This is preceeded by a STX
#define BSC_CONTROL_TTD     BSC_CONTROL_ENQ

#endif

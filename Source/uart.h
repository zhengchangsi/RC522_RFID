#ifndef  __uart_h__
#define  __uart_h__

#define	UART_MAX_LEN	50

extern INT8U  UartBuf[UART_MAX_LEN];

extern INT8U UartCount, UartLen, UartStart, UartComp;

extern void init_uart(void);
extern void send_byte(INT8U  SendData);
extern void send_bytes( INT8U *Buf, INT8U Len );

#endif

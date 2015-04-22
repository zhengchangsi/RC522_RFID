#include "reg52.h"
#include "string.h"
#include "depend.h"
#include "uart.h"  
#include "rc522.h"
#include "ctrl.h"
#include "beep.h"

const INT8U DefaultKeyABuf[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

INT8U CardRevBuf[16] = { 0 };
INT8U const CardKeyABlockBuf[16] = {
								     0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
								     0xff,0x07,0x80,0x69,
							         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	
								};

INT8U CtrlMode = CTRL_PC_MODE;//CTRL_BOARD_MODE;
INT8U OptMode  = OPT_INC_MODE;
INT8U bPass = 0; bWarn = 0;
INT8U LedOnType = LED_LONG;

void pass( void )
{
   beep1();
   LED_BLINK_1();
}

void warn(void)
{
  LED_BLINK_2();
}
	
void cal_keyA( INT8U *DestBuf )
{
	const INT8U KeyABuf[] = { 0x20, 0x12, 0x10, 0x01, 0x00, 0x00 };

	memcpy( DestBuf, KeyABuf, 6 );
}
			 
void send_rebck( psUartData psUartDataRevSend )
{
	INT8U * pTmp;
	INT8U Len,Check;
	INT8U i;

	Len = psUartDataRevSend->UartCmdLen	+ psUartDataRevSend->UartDataLen + 2;
	pTmp = ( INT8U * )psUartDataRevSend;

	send_byte( 0xFC );
	Check = 0xFC;
	for( i = 0; i < Len; i++ )
	{
		send_byte( *pTmp );
		Check ^= *pTmp++;
	}
	send_byte( Check );	
}

void uart_over( void )	//串口数据还原
{
	UartCount = 0;
	UartStart = FALSE;
	UartComp = FALSE;
}

INT8U check_com( psUartData psUartDataRevSend )
{
	psUartDataRevSend->UartDataBuf[0] = 0xAA; //返回数据AA,说明串口通信成功

	return TRUE;
}

INT8U req_card_sn( psUartData psUartDataRevSend )
{
	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( psUartDataRevSend->UartDataBuf, 0x00, psUartDataRevSend->UartDataLen );//将 s 中前 n 个字节用 ch 替换并返回 s 
			bWarn = 1;		//读取失败
			return FALSE;
		}	
	}
	
	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //防冲撞，返回卡的序列号 4字节 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( psUartDataRevSend->UartDataBuf, 0x00, psUartDataRevSend->UartDataLen );
		bWarn = 1;
		return FALSE;	
	}

	memcpy( psUartDataRevSend->UartDataBuf, &CardRevBuf[2], 4 ); //将卡号放到数据位置

	bPass = 1;

	return TRUE;
}

INT8U updata_key( psUartData psUartDataRevSend )
{
	INT8U CardKeyABuf[6];
	INT8U KeyBlockAddr;

	psUartDataRevSend->UartDataLen = 7;
	KeyBlockAddr = psUartDataRevSend->UartDataBuf[0];
  
	if( KeyBlockAddr % 4 != 3 )
	{
		psUartDataRevSend->UartErrCode = ERROR_BLOCK_ADDR;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;	
	}
	
	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
			bWarn = 1;
			return FALSE;
		}	
	}

	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //防冲撞，返回卡的序列号 4字节 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdSelect( &CardRevBuf[2] ) != MI_OK )//选卡
	{
		psUartDataRevSend->UartErrCode = ERROR_SLCT;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;
	}

	memcpy( CardKeyABuf, &psUartDataRevSend->UartDataBuf[1], 6 );

	if( PcdAuthState( PICC_AUTHENT1A, KeyBlockAddr, CardKeyABuf, &CardRevBuf[2] ) != MI_OK )// 验证密码
	{
		psUartDataRevSend->UartErrCode = ERROR_KEYA_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;	
	}

	memcpy( CardKeyABlockBuf, &psUartDataRevSend->UartDataBuf[7], 6 );

	if( PcdWrite( KeyBlockAddr, CardKeyABlockBuf ) != MI_OK )// 写卡
	{
		psUartDataRevSend->UartErrCode = ERROR_WRITE_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;
	}

   	memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
	bPass = 1;
	return TRUE;
}

INT8U block_write( psUartData psUartDataRevSend )
{
	INT8U CardWriteBuf[16];
	INT8U CardKeyABuf[6];
	INT8U KeyBlockAddr;

	psUartDataRevSend->UartDataLen = 0x17;
	KeyBlockAddr = psUartDataRevSend->UartDataBuf[0];
   
   /*
	if( KeyBlockAddr % 4 == 3 )		 这里锁定密码块
	{
		psUartDataRevSend->UartErrCode = ERROR_BLOCK_ADDR;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}
	*/

	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
			bWarn = 1;
			return FALSE;
		}	
	}

	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //防冲撞，返回卡的序列号 4字节 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdSelect( &CardRevBuf[2] ) != MI_OK )//选卡
	{
		psUartDataRevSend->UartErrCode = ERROR_SLCT;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;
	}

	memcpy( CardKeyABuf, &psUartDataRevSend->UartDataBuf[1], 6 );

	if( PcdAuthState( PICC_AUTHENT1A, KeyBlockAddr, CardKeyABuf, &CardRevBuf[2] ) != MI_OK )// 验证密码
	{
		psUartDataRevSend->UartErrCode = ERROR_KEYA_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	memcpy( CardWriteBuf, &psUartDataRevSend->UartDataBuf[7], 16 );
	if( PcdWrite( KeyBlockAddr, CardWriteBuf ) != MI_OK )//写卡
	{
		psUartDataRevSend->UartErrCode = ERROR_WRITE_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;
	}
	bPass = 1;
	
	return TRUE;
}

INT8U block_read( psUartData psUartDataRevSend )
{
	INT8U CardReadBuf[16];
	INT8U CardKeyABuf[6];
	INT8U KeyBlockAddr;

	psUartDataRevSend->UartDataLen = 0x11;
	KeyBlockAddr = psUartDataRevSend->UartDataBuf[0];

/*	if( KeyBlockAddr % 4 == 3 )	  这里锁定密码块
	{
		psUartDataRevSend->UartErrCode = ERROR_BLOCK_ADDR;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}
  */
	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//寻天线区内未进入休眠状态的卡，返回卡片类型 2字节	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
			bWarn = 1;
			return FALSE;
		}	
	}

	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //防冲撞，返回卡的序列号 4字节 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdSelect( &CardRevBuf[2] ) != MI_OK )//选卡
	{
		psUartDataRevSend->UartErrCode = ERROR_SLCT;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;
	}

	memcpy( CardKeyABuf, &psUartDataRevSend->UartDataBuf[1], 6 );

	if( PcdAuthState( PICC_AUTHENT1A, KeyBlockAddr, CardKeyABuf, &CardRevBuf[2] ) != MI_OK )// 验证密码
	{
		psUartDataRevSend->UartErrCode = ERROR_KEYA_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdRead( KeyBlockAddr, CardReadBuf ) != MI_OK )// 读卡
	{
		psUartDataRevSend->UartErrCode = ERROR_READ_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;
	}
	memcpy( &psUartDataRevSend->UartDataBuf[1], CardReadBuf, 16 );
	bPass = 1;
			
	return TRUE;
}			


void pc_process( void )
{
	psUartData psUartDataRevSend;  //定义一个命令格式的结构体，详细请看串口格式定义。
	INT8U i;
	INT8U Check = 0xFE;	           //指令起始符
	INT8U Len;

	if( UartComp != TRUE )         //判断串口是否已经收到命令，没收到则退出。
	{
		return;
	}

	psUartDataRevSend = (psUartData)UartBuf; //将串口指令转化为当前命令格式
	Len = psUartDataRevSend->UartCmdLen + psUartDataRevSend->UartDataLen + 3; //字符长度为总长度减1

	for( i = 0; i < ( Len - 1 ); i ++ )	//异或校验计算
	{
	  Check ^= UartBuf[i];
	}

 /*判断计算值和接收到的校验码是否相同，是则指令正确，跳过此代码；否则指令错误，执行此代码，退出*/
	if( Check != UartBuf[Len - 1] )
	{
		psUartDataRevSend->UartErrCode = ERROR_CHECK; //将标示符判定为 ERROR_CHECK （校验错误）
		send_rebck( psUartDataRevSend );              //返回给指令发送方
		uart_over();								  //串口数据清0
		return;
	}
	
	//指令正确时候执行

	switch( psUartDataRevSend->UartCmd )
	{	
		case 0x0002:  //检测串口状态 -> fe 03 01 c1 c0 er 00 ck,  <- fc 03 01 c1 c0 er aa ck
			check_com( psUartDataRevSend );
			break;

		case 0x0003: //查询卡号  ->	fe 03 04 c1 c0 er 00 00 00 00 ck, <- fc 03 04 c1 c0 er sn3 sn2 sn1 sn0 ck
			req_card_sn( psUartDataRevSend );
			break;

		case 0x0110: //修改密码方式0 fe 03 0d c1 c0 er kn o5 o4.. o0 n5 n4.. n0 ck, <-fe 03 07 c1 c0 er kn n5 n4.. n0 ck	
			updata_key( psUartDataRevSend ); //修改密码 kn%4 == 3	
			break;

		case 0x0120: //读数据块方式0  -> fe 03 07 c1 c0 er kn ky5 ... ky0 ck, <- fc 03 11 c1 c0 er kn d15...d0 ck 
			block_read( psUartDataRevSend ); //读数据块 kn%4 != 3	
			break;

		case 0x0130: //写数据块方式0  -> fe 03 07 c1 c0 er kn ky5 ... ky0 ck, <- fc 03 11 c1 c0 er kn d15...d0 ck 
			block_write( psUartDataRevSend ); //读数据块 kn%4 != 3	
			break;

		default:
			psUartDataRevSend->UartErrCode = ERROR_NOCMD;	
			break;
	}
	send_rebck( psUartDataRevSend );
	uart_over();
}


void ctrl_process( void )
{		    
	pc_process();    //串口处理

	if( bPass )		 //处理成功
	{
		bPass = 0;
		pass();	
	}

	if( bWarn )	     //处理失败
	{
		bWarn = 0;
		warn();
	}
}


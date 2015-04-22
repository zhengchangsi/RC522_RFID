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

void uart_over( void )	//�������ݻ�ԭ
{
	UartCount = 0;
	UartStart = FALSE;
	UartComp = FALSE;
}

INT8U check_com( psUartData psUartDataRevSend )
{
	psUartDataRevSend->UartDataBuf[0] = 0xAA; //��������AA,˵������ͨ�ųɹ�

	return TRUE;
}

INT8U req_card_sn( psUartData psUartDataRevSend )
{
	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( psUartDataRevSend->UartDataBuf, 0x00, psUartDataRevSend->UartDataLen );//�� s ��ǰ n ���ֽ��� ch �滻������ s 
			bWarn = 1;		//��ȡʧ��
			return FALSE;
		}	
	}
	
	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //����ײ�����ؿ������к� 4�ֽ� 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( psUartDataRevSend->UartDataBuf, 0x00, psUartDataRevSend->UartDataLen );
		bWarn = 1;
		return FALSE;	
	}

	memcpy( psUartDataRevSend->UartDataBuf, &CardRevBuf[2], 4 ); //�����ŷŵ�����λ��

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
	
	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
			bWarn = 1;
			return FALSE;
		}	
	}

	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //����ײ�����ؿ������к� 4�ֽ� 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdSelect( &CardRevBuf[2] ) != MI_OK )//ѡ��
	{
		psUartDataRevSend->UartErrCode = ERROR_SLCT;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;
	}

	memcpy( CardKeyABuf, &psUartDataRevSend->UartDataBuf[1], 6 );

	if( PcdAuthState( PICC_AUTHENT1A, KeyBlockAddr, CardKeyABuf, &CardRevBuf[2] ) != MI_OK )// ��֤����
	{
		psUartDataRevSend->UartErrCode = ERROR_KEYA_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 6 );
		bWarn = 1;
		return FALSE;	
	}

	memcpy( CardKeyABlockBuf, &psUartDataRevSend->UartDataBuf[7], 6 );

	if( PcdWrite( KeyBlockAddr, CardKeyABlockBuf ) != MI_OK )// д��
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
	if( KeyBlockAddr % 4 == 3 )		 �������������
	{
		psUartDataRevSend->UartErrCode = ERROR_BLOCK_ADDR;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}
	*/

	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
			bWarn = 1;
			return FALSE;
		}	
	}

	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //����ײ�����ؿ������к� 4�ֽ� 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdSelect( &CardRevBuf[2] ) != MI_OK )//ѡ��
	{
		psUartDataRevSend->UartErrCode = ERROR_SLCT;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;
	}

	memcpy( CardKeyABuf, &psUartDataRevSend->UartDataBuf[1], 6 );

	if( PcdAuthState( PICC_AUTHENT1A, KeyBlockAddr, CardKeyABuf, &CardRevBuf[2] ) != MI_OK )// ��֤����
	{
		psUartDataRevSend->UartErrCode = ERROR_KEYA_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	memcpy( CardWriteBuf, &psUartDataRevSend->UartDataBuf[7], 16 );
	if( PcdWrite( KeyBlockAddr, CardWriteBuf ) != MI_OK )//д��
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

/*	if( KeyBlockAddr % 4 == 3 )	  �������������
	{
		psUartDataRevSend->UartErrCode = ERROR_BLOCK_ADDR;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}
  */
	if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
	{
		if( PcdRequest( PICC_REQIDL, &CardRevBuf[0] ) != MI_OK )//Ѱ��������δ��������״̬�Ŀ������ؿ�Ƭ���� 2�ֽ�	
		{
			psUartDataRevSend->UartErrCode = ERROR_NOCARD;
			memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
			bWarn = 1;
			return FALSE;
		}	
	}

	if( PcdAnticoll( &CardRevBuf[2] ) != MI_OK ) //����ײ�����ؿ������к� 4�ֽ� 
	{
		psUartDataRevSend->UartErrCode = ERROR_ATCLL;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdSelect( &CardRevBuf[2] ) != MI_OK )//ѡ��
	{
		psUartDataRevSend->UartErrCode = ERROR_SLCT;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;
	}

	memcpy( CardKeyABuf, &psUartDataRevSend->UartDataBuf[1], 6 );

	if( PcdAuthState( PICC_AUTHENT1A, KeyBlockAddr, CardKeyABuf, &CardRevBuf[2] ) != MI_OK )// ��֤����
	{
		psUartDataRevSend->UartErrCode = ERROR_KEYA_IC;
		memset( &psUartDataRevSend->UartDataBuf[1], 0x00, 16 );
		bWarn = 1;
		return FALSE;	
	}

	if( PcdRead( KeyBlockAddr, CardReadBuf ) != MI_OK )// ����
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
	psUartData psUartDataRevSend;  //����һ�������ʽ�Ľṹ�壬��ϸ�뿴���ڸ�ʽ���塣
	INT8U i;
	INT8U Check = 0xFE;	           //ָ����ʼ��
	INT8U Len;

	if( UartComp != TRUE )         //�жϴ����Ƿ��Ѿ��յ����û�յ����˳���
	{
		return;
	}

	psUartDataRevSend = (psUartData)UartBuf; //������ָ��ת��Ϊ��ǰ�����ʽ
	Len = psUartDataRevSend->UartCmdLen + psUartDataRevSend->UartDataLen + 3; //�ַ�����Ϊ�ܳ��ȼ�1

	for( i = 0; i < ( Len - 1 ); i ++ )	//���У�����
	{
	  Check ^= UartBuf[i];
	}

 /*�жϼ���ֵ�ͽ��յ���У�����Ƿ���ͬ������ָ����ȷ�������˴��룻����ָ�����ִ�д˴��룬�˳�*/
	if( Check != UartBuf[Len - 1] )
	{
		psUartDataRevSend->UartErrCode = ERROR_CHECK; //����ʾ���ж�Ϊ ERROR_CHECK ��У�����
		send_rebck( psUartDataRevSend );              //���ظ�ָ��ͷ�
		uart_over();								  //����������0
		return;
	}
	
	//ָ����ȷʱ��ִ��

	switch( psUartDataRevSend->UartCmd )
	{	
		case 0x0002:  //��⴮��״̬ -> fe 03 01 c1 c0 er 00 ck,  <- fc 03 01 c1 c0 er aa ck
			check_com( psUartDataRevSend );
			break;

		case 0x0003: //��ѯ����  ->	fe 03 04 c1 c0 er 00 00 00 00 ck, <- fc 03 04 c1 c0 er sn3 sn2 sn1 sn0 ck
			req_card_sn( psUartDataRevSend );
			break;

		case 0x0110: //�޸����뷽ʽ0 fe 03 0d c1 c0 er kn o5 o4.. o0 n5 n4.. n0 ck, <-fe 03 07 c1 c0 er kn n5 n4.. n0 ck	
			updata_key( psUartDataRevSend ); //�޸����� kn%4 == 3	
			break;

		case 0x0120: //�����ݿ鷽ʽ0  -> fe 03 07 c1 c0 er kn ky5 ... ky0 ck, <- fc 03 11 c1 c0 er kn d15...d0 ck 
			block_read( psUartDataRevSend ); //�����ݿ� kn%4 != 3	
			break;

		case 0x0130: //д���ݿ鷽ʽ0  -> fe 03 07 c1 c0 er kn ky5 ... ky0 ck, <- fc 03 11 c1 c0 er kn d15...d0 ck 
			block_write( psUartDataRevSend ); //�����ݿ� kn%4 != 3	
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
	pc_process();    //���ڴ���

	if( bPass )		 //����ɹ�
	{
		bPass = 0;
		pass();	
	}

	if( bWarn )	     //����ʧ��
	{
		bWarn = 0;
		warn();
	}
}


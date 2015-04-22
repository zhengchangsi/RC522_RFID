#include "reg52.h"
#include "depend.h"
#include "uart.h"
#include "timer.h"
#include "rc522.h"
#include "ctrl.h"
#include "lcd12864.h"
#include "beep.h"

void init_all(void)	//��ʼ��
{
	EA = 0;	      //�����ж�		 
	init_timer(); //��ʱ����ʼ��
	init_uart();  //���ڳ�ʼ��
	init_rc522(); //RC522��ʼ��	
	lcd_init();	  //LCD��ʼ��      
	EA = 1;		  //�����ж�
}

void main(void)	  //������
{	

	Delay_ms(500); //��Ӳ���ȶ�
	init_all();	   //ִ�г�ʼ������	
	LED_BLINK_1(); //led test
	beep1();	   //beep test 
	display();     //LCD TEST	
	while(1)
	{
		ctrl_process(); //����RC522����
	}
}	  


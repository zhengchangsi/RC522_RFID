#include "reg52.h"
#include "depend.h"
#include "uart.h"
#include "timer.h"
#include "rc522.h"
#include "ctrl.h"
#include "lcd12864.h"
#include "beep.h"

void init_all(void)	//初始化
{
	EA = 0;	      //关总中断		 
	init_timer(); //定时器初始化
	init_uart();  //串口初始化
	init_rc522(); //RC522初始化	
	lcd_init();	  //LCD初始化      
	EA = 1;		  //开总中断
}

void main(void)	  //主函数
{	

	Delay_ms(500); //让硬件稳定
	init_all();	   //执行初始化函数	
	LED_BLINK_1(); //led test
	beep1();	   //beep test 
	display();     //LCD TEST	
	while(1)
	{
		ctrl_process(); //进入RC522操作
	}
}	  


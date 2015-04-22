#include <reg52.h>
#include <intrins.h>
#define uchar unsigned char
#define uint  unsigned int

/*LCD12864  端口定义*/
#define LCD_data  P0            //数据口
sbit LCD_RS  =  P2^2;            //寄存器选择输入 
sbit LCD_RW  =  P2^1;            //液晶读/写控制
sbit LCD_EN  =  P2^0;            //液晶使能控制
//sbit LCD_PSB =  P0^4;            //串/并方式控制,部分液晶不需要

uchar code dis0[]=" 易控实验室RFID ";
uchar code dis1[]="                ";
uchar code dis2[]="                ";
uchar code dis3[]="                ";


void delay(int ms)	 //延时xms
{
    while(ms--)
	{
      uchar i;
	  for(i=0;i<250;i++)  
	   {
	    _nop_();			   
		_nop_();
		_nop_();
		_nop_();
	   }
	}
}	

void delayNOP()  //延时4us
{ _nop_(); _nop_(); _nop_(); _nop_();}

/*******************************************************************/
/*                                                                 */
/*检查LCD忙状态                                                    */
/*lcd_busy为1时，忙，等待。lcd-busy为0时,闲，可写指令与数据。      */
/*                                                                 */
/*******************************************************************/
bit lcd_busy()
 {                          
    bit result;
    LCD_RS = 0;
    LCD_RW = 1;
    LCD_EN = 1;
    delayNOP();
    result = (bit)(LCD_data&0x80);
    LCD_EN = 0;
    return(result); 
 }
/*******************************************************************/
/*                                                                 */
/*写指令数据到LCD                                                  */
/*RS=L，RW=L，E=高脉冲，D0-D7=指令码。                             */
/*                                                                 */
/*******************************************************************/
void write_cmd(uchar cmd)
{                          
   while(lcd_busy());
    LCD_RS = 0;
    LCD_RW = 0;
    LCD_EN = 0;
    _nop_();
    _nop_(); 
    LCD_data = cmd;
    delayNOP();
    LCD_EN = 1;
    delayNOP();
    LCD_EN = 0;  
}
/*******************************************************************/
/*                                                                 */
/*写显示数据到LCD                                                  */
/*RS=H，RW=L，E=高脉冲，D0-D7=数据。                               */
/*                                                                 */
/*******************************************************************/
void lcd_wdat(uchar dat)
{                          
   while(lcd_busy());
    LCD_RS = 1;
    LCD_RW = 0;
    LCD_EN = 0;
    LCD_data= dat;
    delayNOP();
    LCD_EN = 1;
    delayNOP();
    LCD_EN = 0; 
}
/*******************************************************************/
/*                                                                 */
/*  LCD初始化设定                                                  */
/*                                                                 */
/*******************************************************************/
void lcd_init()
{ 

 //   LCD_PSB = 1;         //并口方式	，部分液晶不需要
    write_cmd(0x36);      //扩充指令操作
    delay(5);
    write_cmd(0x30);      //基本指令操
	delay(5);
    write_cmd(0x0C);      //显示开，关光标
    delay(5);
    write_cmd(0x01);      //清除LCD的显示内容
    delay(5);
}	  
/*********************************************************/
/*                                                       */
/* 设定显示位置                                          */
/*                                                       */
/*********************************************************/
void lcd_pos(uchar X,uchar Y)
{                          
   uchar  pos;
   if (X==0)
     {X=0x80;}
   else if (X==1)
     {X=0x90;}
   else if (X==2)
     {X=0x88;}
   else if (X==3)
     {X=0x98;}
   pos = X+Y ;  
   write_cmd(pos);     //显示地址
}

/*********************************************************/
/*														 */
/* 主程序           									 */
/*                                                       */
/*********************************************************/
void display()
 {

    uchar i;          
	   	  
	  lcd_pos(0,0);             //第一行显示数字0~9
	  i = 0;   
      while(dis0[i] != '\0')
       {
         lcd_wdat(dis0[i]);      //显示字母
         i++;
       }
	     
      lcd_pos(1,0);             //第二行显示字母
	  i = 0;   
      while(dis1[i] != '\0')
       {
         lcd_wdat(dis1[i]);      //显示字母
         i++;
       }

 	  lcd_pos(2,0);             //第三行显示文字
	  i = 0;
      while(dis2[i] != '\0')
       {
         lcd_wdat(dis2[i]);     
         i++;
       }

	  lcd_pos(3,0);             //第四行显示广工无线电社
	  i = 0;
      while(dis3[i] != '\0')
       {
         lcd_wdat(dis3[i]);     
         i++;
       }
	  
}

#include <reg52.h>
#include <intrins.h>
#define uchar unsigned char
#define uint  unsigned int

/*LCD12864  �˿ڶ���*/
#define LCD_data  P0            //���ݿ�
sbit LCD_RS  =  P2^2;            //�Ĵ���ѡ������ 
sbit LCD_RW  =  P2^1;            //Һ����/д����
sbit LCD_EN  =  P2^0;            //Һ��ʹ�ܿ���
//sbit LCD_PSB =  P0^4;            //��/����ʽ����,����Һ������Ҫ

uchar code dis0[]=" �׿�ʵ����RFID ";
uchar code dis1[]="                ";
uchar code dis2[]="                ";
uchar code dis3[]="                ";


void delay(int ms)	 //��ʱxms
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

void delayNOP()  //��ʱ4us
{ _nop_(); _nop_(); _nop_(); _nop_();}

/*******************************************************************/
/*                                                                 */
/*���LCDæ״̬                                                    */
/*lcd_busyΪ1ʱ��æ���ȴ���lcd-busyΪ0ʱ,�У���дָ�������ݡ�      */
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
/*дָ�����ݵ�LCD                                                  */
/*RS=L��RW=L��E=�����壬D0-D7=ָ���롣                             */
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
/*д��ʾ���ݵ�LCD                                                  */
/*RS=H��RW=L��E=�����壬D0-D7=���ݡ�                               */
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
/*  LCD��ʼ���趨                                                  */
/*                                                                 */
/*******************************************************************/
void lcd_init()
{ 

 //   LCD_PSB = 1;         //���ڷ�ʽ	������Һ������Ҫ
    write_cmd(0x36);      //����ָ�����
    delay(5);
    write_cmd(0x30);      //����ָ���
	delay(5);
    write_cmd(0x0C);      //��ʾ�����ع��
    delay(5);
    write_cmd(0x01);      //���LCD����ʾ����
    delay(5);
}	  
/*********************************************************/
/*                                                       */
/* �趨��ʾλ��                                          */
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
   write_cmd(pos);     //��ʾ��ַ
}

/*********************************************************/
/*														 */
/* ������           									 */
/*                                                       */
/*********************************************************/
void display()
 {

    uchar i;          
	   	  
	  lcd_pos(0,0);             //��һ����ʾ����0~9
	  i = 0;   
      while(dis0[i] != '\0')
       {
         lcd_wdat(dis0[i]);      //��ʾ��ĸ
         i++;
       }
	     
      lcd_pos(1,0);             //�ڶ�����ʾ��ĸ
	  i = 0;   
      while(dis1[i] != '\0')
       {
         lcd_wdat(dis1[i]);      //��ʾ��ĸ
         i++;
       }

 	  lcd_pos(2,0);             //��������ʾ����
	  i = 0;
      while(dis2[i] != '\0')
       {
         lcd_wdat(dis2[i]);     
         i++;
       }

	  lcd_pos(3,0);             //��������ʾ�㹤���ߵ���
	  i = 0;
      while(dis3[i] != '\0')
       {
         lcd_wdat(dis3[i]);     
         i++;
       }
	  
}

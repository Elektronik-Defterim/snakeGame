#include "ili9341.h"
#include <string.h>
#include <stdlib.h>

uint8_t rotationNum = 1;
static bool _cp437    = false;

extern UART_HandleTypeDef huart2;

__STATIC_INLINE void DelayMicro(__IO uint32_t micros)
{
        micros *=(SystemCoreClock / 1000000) / 5;
        while (micros--);
}

void ILI9341_SendCommand(uint8_t com){
	*(__IO uint8_t*)(0x60000000) = com;
}

void ILI9341_SendData(uint8_t data){
	*(__IO uint8_t*)(0x60040000) = data;
}

uint32_t ILI9341_ReadReg(uint8_t r)
{
        uint32_t id;
        uint8_t x;

        ILI9341_SendCommand(r);
        DelayMicro(50);

    	x=ADDR_DATA;
        id=x;
        id<<=8;
        DelayMicro(1);

        x=ADDR_DATA;
        id|=x;
        id<<=8;
        DelayMicro(1);

		x=ADDR_DATA;
        id|=x;
        id<<=8;

		DelayMicro(1);
        x=ADDR_DATA;
        id|=x;

		if(r==0xEF)
        {
                id<<=8;
                DelayMicro(5);
                x=ADDR_DATA;
                id|=x;
        }
        DelayMicro(150);
        return id;
}

void ILI9341_reset(void)
{
        RESET_ACTIVE;
        HAL_Delay(2);
        RESET_IDLE;
        ILI9341_SendCommand(0x01);
        for (uint8_t i=0;i<3;i++)
        	ILI9341_SendData(0xFF);
}

void ILI9341_Init(void)
{
	    char str[10];
	    ILI9341_reset();
	    HAL_Delay(1000);
	    uint32_t dtt = ILI9341_ReadReg(0xD3);
	    sprintf(str,"0x%08lX",(unsigned long)dtt);
	    HAL_UART_Transmit(&huart2,(uint8_t *)str,strlen(str),1000);
}

void ILI9341_Configuration(){
	/* Steps
	 1:Software Reset
	 2:Display Off
	 3:POWER1 - POWER2 Configuration
	 4:VCOM1-VCOM2 Configuration
	 5: Memory Access Configuration(This command defines read/write scanning direction of frame memory.)
	 6:Pixel Format Set
	 7:Frame Rate Configuration
	 8:Column And Page Adress
	 9:Tearing Effect Off
	 10:Entry Mode Set
	 11:Display Function
	 12:Sleep Out
	 //13:Display On
	 */

	// 1: Software Reset
	ILI9341_SendCommand(ILI9341_RESET);
	HAL_Delay(120);

	// 2:Dislay Off
	ILI9341_SendCommand(ILI9341_DISPLAY_OFF);

	//3 POWER1 - POWER2 Configuration
	ILI9341_SendCommand(ILI9341_POWER1);
	ILI9341_SendData(0x26);
	DelayMicro(1);
	ILI9341_SendCommand(ILI9341_POWER2);
	ILI9341_SendData(0x11);       //BT[2:0] = 0 0 1 DDVDH=2 VGH=7 -VCI*3
	DelayMicro(1);

	//4 VCOM1-VCOM2
	ILI9341_SendCommand(ILI9341_VCOM1);
	ILI9341_SendData(0x35);				//VMH [6:0] = 0110101  VCOMH(V) =   4.025
	ILI9341_SendData(0x3e);				//VML [6:0] = 0111110  VCOML(V) =  -0.950v
	DelayMicro(1);

	ILI9341_SendCommand(ILI9341_VCOM2);
	ILI9341_SendData(0xbe);            // VMF[6:0]= 1000001 =>  VMH + 1 - VML + 1   When nVM(8.bit) set to “1”, setting
	DelayMicro(1);					  //of VMF [6:0] becomes valid and VCOMH/VCOML can be adjusted.


	/*5: Memory Access Configuration(This command defines read/write scanning direction of frame memory.)
	 [7:0]  ->
	           0-1.bit: Null
	           2.bit  : MH Horizontal Refresh ORDER LCD horizontal refreshing direction control
	           3.bit  : BGR RGB-BGR Order Color selector switch control(0=RGB color filter panel, 1=BGR color filter panel)
	           4.bit  : ML Vertical Refresh Order LCD vertical refresh direction control.
	           5.bit  : MV Row / Column Exchange
	           6.bit  : MX Column Address Order
	           7.bit  : MY Row Address Order
	 */
	ILI9341_SendCommand(ILI9341_MAC);
	ILI9341_SendData(0x48);        // [7:0] = 0 1 0 0 1 0 0 0
	DelayMicro(1);


	/*6: Pixel Format Configuration
	  [7:0] => DBI[2:0]  MCU Interface Format = 1 1 1 (Reserved)
	 	 	=> DPI[2:0]  RGB Interface Format = 1 1 0 (18 bits / pixel )
	 	 	=> 7.Bit     Null
	  */
	ILI9341_SendCommand(ILI9341_PIXEL_FORMAT);
	ILI9341_SendData(0x55); // [7:0] = 0 1 1 0 1 1 1 Pixel format 18bit seçildi
	DelayMicro(1);

	// 7:Frame Rate Configuration
	ILI9341_SendCommand(ILI9341_FRC);
	ILI9341_SendData(0);     //Clock Division 1
	ILI9341_SendData(0x3D);  //RTNA [4:0] = 1 0 1 1 1  => Frame Rate(Hz) = 83 Hz
	DelayMicro(1);			 //RTNA [4:0] = 1 0 0 0 0  => Frame Rate(Hz) = 119 Hz


	//8:Column And Page Adress
	ILI9341_SendCommand(ILI9341_COLUMN_ADDR);
	ILI9341_SendData(0x00);   // x0_HIGH---0
	ILI9341_SendData(0x00);	  // x0_LOW----0
	ILI9341_SendData(0x00);   // x1_HIGH---240
	ILI9341_SendData(0xEF);   // x1_LOW----240

	ILI9341_SendCommand (ILI9341_PAGE_ADDR); // page address set
	ILI9341_SendData   (0x00); // y0_HIGH---0
	ILI9341_SendData   (0x00); // y0_LOW----0
	ILI9341_SendData   (0x01); // y1_HIGH---320
    ILI9341_SendData   (0x3F); // y1_LOW----320


    //9:Tearing Effect Off
    ILI9341_SendCommand (ILI9341_TEARING_OFF);

    /* 10:Entry Mode Set
     D[7:0]  : 0 0 0 0 0 GON DTE GAS
     GAS    : Low voltage detection control => 1 low voltage detection disable
     GON/DTE: Set the output level of gate driver G1 =>1 1 = Normal display
     */
    ILI9341_SendCommand (ILI9341_Entry_Mode_Set);
    ILI9341_SendData(0x07);

    /*11:Display Function
     1st parameter : 0 0 0 0 PTG [1:0] PT [1:0]
     2nd parameter : REV GS SS SM ISC [3:0] 8
     3rd parameter : 0 0 NL [5:0]
     4th parameter : 0 0 PCDIV [5:0]
     * */
    ILI9341_SendCommand (ILI9341_DFC);
    ILI9341_SendData   (0x0a);
    ILI9341_SendData   (0x82);
    ILI9341_SendData   (0x27);
    ILI9341_SendData   (0x00); // clock divisor

    //12:Sleep Out
    ILI9341_SendCommand (ILI9341_SLEEP_OUT);
    HAL_Delay(100);

    //13:Display On
    ILI9341_SendCommand (ILI9341_DISPLAY_ON);
    HAL_Delay(100);

    //14:Memory Write
    ILI9341_SendCommand (ILI9341_GRAM); // memory write
    HAL_Delay(5);
}

void ILI9341_SetCursorPosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  ILI9341_SendCommand (ILI9341_COLUMN_ADDR);
  ILI9341_SendData(x1>>8);
  ILI9341_SendData(x1 & 0xFF);
  ILI9341_SendData(x2>>8);
  ILI9341_SendData(x2 & 0xFF);

  ILI9341_SendCommand (ILI9341_PAGE_ADDR);
  ILI9341_SendData(y1>>8);
  ILI9341_SendData(y1 & 0xFF);
  ILI9341_SendData(y2>>8);
  ILI9341_SendData(y2 & 0xFF);
  ILI9341_SendCommand (ILI9341_GRAM);
}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
  ILI9341_SetCursorPosition(x, y, x, y);
	ILI9341_SendData(color>>8);
	ILI9341_SendData(color&0xFF);
}

void ILI9341_Fill(uint16_t color){
	uint32_t n = ILI9341_PIXEL_COUNT;
	if(rotationNum==1 || rotationNum==3)
	{
		ILI9341_SetCursorPosition(0, 0,   ILI9341_WIDTH -1, ILI9341_HEIGHT -1);
	}
	else if(rotationNum==2 || rotationNum==4)
	{
		ILI9341_SetCursorPosition(0, 0, ILI9341_HEIGHT -1, ILI9341_WIDTH -1);
	}

	while(n){
		n--;
		ILI9341_SendData(color>>8);
		ILI9341_SendData(color);
	}
}

void ILI9341_setRotation(uint8_t rotation){

	switch (rotation) {
		case 2:
			rotationNum = 2;
			ILI9341_SendCommand(ILI9341_MEMCONTROL);
			ILI9341_SendData(ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
			break;
		case 3:
			rotationNum = 3;
			ILI9341_SendCommand(ILI9341_MEMCONTROL);
			ILI9341_SendData(ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);
			break;
		case 4:
			rotationNum = 4;
			ILI9341_SendCommand(ILI9341_MEMCONTROL);
			ILI9341_SendData(ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);
			break;
		default:
			rotationNum = 1;
			ILI9341_SendCommand(ILI9341_MEMCONTROL);
			ILI9341_SendData(ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
			break;

	}

}

void ILI9341_Fill_Rect(unsigned int x0,unsigned int y0, unsigned int x1,unsigned int y1, uint16_t color) {
	uint32_t n = ((x1+1)-x0)*((y1+1)-y0);
	if (n>ILI9341_PIXEL_COUNT) n=ILI9341_PIXEL_COUNT;
	ILI9341_SetCursorPosition(x0, y0, x1, y1);
	while (n) {
			n--;
      ILI9341_SendData(color>>8);
				ILI9341_SendData(color&0xff);
	}
}

void ILI9341_drawChar(int16_t x,int16_t y,unsigned char c,uint16_t color,uint16_t bg,uint16_t size){

	uint16_t width = ILI9341_WIDTH;
	uint16_t height = ILI9341_HEIGHT;

	if(rotationNum == 2 || rotationNum == 4){
		width = ILI9341_HEIGHT;
		height = ILI9341_WIDTH;

	}

	if(	(x>=width)  ||
		(y>=height) ||
		((x+6*size-1)<0)	||
		((y+8*size-1)<0)){
		return;
	}

	if((!_cp437)&&(c>=176)) c++;

	for(int8_t i = 0; i<6;i++){
		uint8_t line;
		if(i==5)
			line = 0x00;

		else
			line = pgm_read_byte(font1 + (c*5)+i);

		for(int8_t j = 0;j < 8;j++){
			if(line & 0x1){
				if(size == 1)
					ILI9341_DrawPixel(x+i,y+j,color);
		        else {  // big size
		          ILI9341_Fill_Rect(x+(i*size), y+(j*size), size + x+(i*size), size+1 + y+(j*size), color);
		        }
			}
			line >>= 1;
		}
	}
}

void ILI9341_printText(char text[], int16_t x, int16_t y, uint16_t color, uint16_t bg, uint8_t size)
{
	int16_t offset;
	offset = size*6;
	for(uint16_t i=0; i<40 && text[i]!='\0'; i++)
	{
		ILI9341_drawChar(x+(offset*i), y, text[i],color,bg,size);
	}
}








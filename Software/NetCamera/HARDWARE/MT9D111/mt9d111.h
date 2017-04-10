#ifndef _MT9D111_H
#define _MT9D111_H
#include "sys.h"
#include "sccb.h"


#define CAMERA_RST  	PDout(6)			//¸´Î»¿ØÖÆÐÅºÅ 


#define VGA_FRAME 1

 
#ifdef XGA_FRAME
		#define PIXELS_IN_X_AXIS        (1024)
		#define PIXELS_IN_Y_AXIS        (768)

#elif VGA_FRAME
		#define PIXELS_IN_X_AXIS        (640)
		#define PIXELS_IN_Y_AXIS        (480)

#elif QVGA_FRAME
		#define PIXELS_IN_X_AXIS        (320)
		#define PIXELS_IN_Y_AXIS        (240)
#elif MY_FRAME
		#define PIXELS_IN_X_AXIS        (1600)
		#define PIXELS_IN_Y_AXIS        (1200)
#endif
				


u8 MT9D111_Init(void);
uint8_t wrMT9D111RegBit(u8 Page, u8 regID, u16 C_Dat,u16 W_Dat);
void MT9D111_RGB_Config(void);
void MT9D111_Jpeg_Config(void);
u16 MT9D111_Greate_Header(void);

#endif






















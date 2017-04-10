#include "sys.h"
#include "mt9d111.h"
#include "mt9d111cfg.h"
#include "timer.h"	  
#include "delay.h"
#include "usart.h"			 
#include "sccb.h"	
#include <string.h>
#include "pcf8574.h"  
#include "stmflash.h"
#include "protocol.h"
 

 char g_header[1024] = {'\0'};
 unsigned long g_header_length;
 
 u16 read_reg;
 
extern SavePara_TypeDef SaveParaList;

 
/*MT9D111  JPEG Header NEEDED*/
#define FORMAT_YCBCR422   0
#define FORMAT_YCBCR420   1
#define FORMAT_MONOCHROME 2

unsigned char JPEG_StdQuantTblY[64] =
{
    16,  11,  10,  16,  24,  40,  51,  61,
    12,  12,  14,  19,  26,  58,  60,  55,
    14,  13,  16,  24,  40,  57,  69,  56,
    14,  17,  22,  29,  51,  87,  80,  62,
    18,  22,  37,  56,  68,  109, 103, 77,
    24,  35,  55,  64,  81,  104, 113, 92,
    49,  64,  78,  87, 103,  121, 120, 101,
    72,  92,  95,  98, 112,  100, 103,  99
};

unsigned char JPEG_StdQuantTblC[64] =
{
    17,  18,  24,  47,  99,  99,  99,  99,
    18,  21,  26,  66,  99,  99,  99,  99,
    24,  26,  56,  99,  99,  99,  99,  99,
    47,  66,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99,
    99,  99,  99,  99,  99,  99,  99,  99
};
//
// This table is used for regular-position to zigzagged-position lookup
//  This is Figure A.6 from the ISO/IEC 10918-1 1993 specification 
//
static unsigned char zigzag[64] =
{
    0, 1, 5, 6,14,15,27,28,
    2, 4, 7,13,16,26,29,42,
    3, 8,12,17,25,30,41,43,
    9,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54,
    20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61,
    35,36,48,49,57,58,62,63
};

unsigned int JPEG_StdHuffmanTbl[384] =
{
    0x100, 0x101, 0x204, 0x30b, 0x41a, 0x678, 0x7f8, 0x9f6,
    0xf82, 0xf83, 0x30c, 0x41b, 0x679, 0x8f6, 0xaf6, 0xf84,
    0xf85, 0xf86, 0xf87, 0xf88, 0x41c, 0x7f9, 0x9f7, 0xbf4,
    0xf89, 0xf8a, 0xf8b, 0xf8c, 0xf8d, 0xf8e, 0x53a, 0x8f7,
    0xbf5, 0xf8f, 0xf90, 0xf91, 0xf92, 0xf93, 0xf94, 0xf95,
    0x53b, 0x9f8, 0xf96, 0xf97, 0xf98, 0xf99, 0xf9a, 0xf9b,
    0xf9c, 0xf9d, 0x67a, 0xaf7, 0xf9e, 0xf9f, 0xfa0, 0xfa1,
    0xfa2, 0xfa3, 0xfa4, 0xfa5, 0x67b, 0xbf6, 0xfa6, 0xfa7,
    0xfa8, 0xfa9, 0xfaa, 0xfab, 0xfac, 0xfad, 0x7fa, 0xbf7,
    0xfae, 0xfaf, 0xfb0, 0xfb1, 0xfb2, 0xfb3, 0xfb4, 0xfb5,
    0x8f8, 0xec0, 0xfb6, 0xfb7, 0xfb8, 0xfb9, 0xfba, 0xfbb,
    0xfbc, 0xfbd, 0x8f9, 0xfbe, 0xfbf, 0xfc0, 0xfc1, 0xfc2,
    0xfc3, 0xfc4, 0xfc5, 0xfc6, 0x8fa, 0xfc7, 0xfc8, 0xfc9,
    0xfca, 0xfcb, 0xfcc, 0xfcd, 0xfce, 0xfcf, 0x9f9, 0xfd0,
    0xfd1, 0xfd2, 0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7, 0xfd8,
    0x9fa, 0xfd9, 0xfda, 0xfdb, 0xfdc, 0xfdd, 0xfde, 0xfdf,
    0xfe0, 0xfe1, 0xaf8, 0xfe2, 0xfe3, 0xfe4, 0xfe5, 0xfe6,
    0xfe7, 0xfe8, 0xfe9, 0xfea, 0xfeb, 0xfec, 0xfed, 0xfee,
    0xfef, 0xff0, 0xff1, 0xff2, 0xff3, 0xff4, 0xff5, 0xff6,
    0xff7, 0xff8, 0xff9, 0xffa, 0xffb, 0xffc, 0xffd, 0xffe,
    0x30a, 0xaf9, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
    0xfd0, 0xfd1, 0xfd2, 0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7,
    0x101, 0x204, 0x30a, 0x418, 0x419, 0x538, 0x678, 0x8f4,
    0x9f6, 0xbf4, 0x30b, 0x539, 0x7f6, 0x8f5, 0xaf6, 0xbf5,
    0xf88, 0xf89, 0xf8a, 0xf8b, 0x41a, 0x7f7, 0x9f7, 0xbf6,
    0xec2, 0xf8c, 0xf8d, 0xf8e, 0xf8f, 0xf90, 0x41b, 0x7f8,
    0x9f8, 0xbf7, 0xf91, 0xf92, 0xf93, 0xf94, 0xf95, 0xf96,
    0x53a, 0x8f6, 0xf97, 0xf98, 0xf99, 0xf9a, 0xf9b, 0xf9c,
    0xf9d, 0xf9e, 0x53b, 0x9f9, 0xf9f, 0xfa0, 0xfa1, 0xfa2,
    0xfa3, 0xfa4, 0xfa5, 0xfa6, 0x679, 0xaf7, 0xfa7, 0xfa8,
    0xfa9, 0xfaa, 0xfab, 0xfac, 0xfad, 0xfae, 0x67a, 0xaf8,
    0xfaf, 0xfb0, 0xfb1, 0xfb2, 0xfb3, 0xfb4, 0xfb5, 0xfb6,
    0x7f9, 0xfb7, 0xfb8, 0xfb9, 0xfba, 0xfbb, 0xfbc, 0xfbd,
    0xfbe, 0xfbf, 0x8f7, 0xfc0, 0xfc1, 0xfc2, 0xfc3, 0xfc4,
    0xfc5, 0xfc6, 0xfc7, 0xfc8, 0x8f8, 0xfc9, 0xfca, 0xfcb,
    0xfcc, 0xfcd, 0xfce, 0xfcf, 0xfd0, 0xfd1, 0x8f9, 0xfd2,
    0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7, 0xfd8, 0xfd9, 0xfda,
    0x8fa, 0xfdb, 0xfdc, 0xfdd, 0xfde, 0xfdf, 0xfe0, 0xfe1,
    0xfe2, 0xfe3, 0xaf9, 0xfe4, 0xfe5, 0xfe6, 0xfe7, 0xfe8,
    0xfe9, 0xfea, 0xfeb, 0xfec, 0xde0, 0xfed, 0xfee, 0xfef,
    0xff0, 0xff1, 0xff2, 0xff3, 0xff4, 0xff5, 0xec3, 0xff6,
    0xff7, 0xff8, 0xff9, 0xffa, 0xffb, 0xffc, 0xffd, 0xffe,
    0x100, 0x9fa, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff, 0xfff,
    0xfd0, 0xfd1, 0xfd2, 0xfd3, 0xfd4, 0xfd5, 0xfd6, 0xfd7,
    0x100, 0x202, 0x203, 0x204, 0x205, 0x206, 0x30e, 0x41e,
    0x53e, 0x67e, 0x7fe, 0x8fe, 0xfff, 0xfff, 0xfff, 0xfff,
    0x100, 0x101, 0x102, 0x206, 0x30e, 0x41e, 0x53e, 0x67e,
    0x7fe, 0x8fe, 0x9fe, 0xafe, 0xfff, 0xfff, 0xfff, 0xfff
};
static int CreateJpegHeader(char *header, int width, int height,
                            int format, int restart_int, int qscale);
static int DefineRestartIntervalMarker(char *pbuf, int ri);
static int DefineHuffmanTableMarkerAC
                               (char *pbuf, unsigned int *htable, int class_id);
static int DefineHuffmanTableMarkerDC
                               (char *pbuf, unsigned int *htable, int class_id);
static int DefineQuantizationTableMarker 
                                  (unsigned char *pbuf, int qscale, int format);
static int ScanHeaderMarker(char *pbuf, int format);
static int FrameHeaderMarker(char *pbuf, int width, int height, int format);
static int JfifApp0Marker(char *pbuf);

static int JfifApp0Marker(char *pbuf)
{
    *pbuf++= 0xFF;                  // APP0 marker 
    *pbuf++= 0xE0;
    *pbuf++= 0x00;                  // length 
    *pbuf++= 0x10;
    *pbuf++= 0x4A;                  // JFIF identifier 
    *pbuf++= 0x46;
    *pbuf++= 0x49;
    *pbuf++= 0x46;
    *pbuf++= 0x00;
    *pbuf++= 0x01;                  // version 
    *pbuf++= 0x02;
    *pbuf++= 0x00;                  // units 
    *pbuf++= 0x00;                  // X density 
    *pbuf++= 0x01;
    *pbuf++= 0x00;                  // Y density 
    *pbuf++= 0x01;
    *pbuf++= 0x00;                  // X thumbnail 
    *pbuf++= 0x00;                  // Y thumbnail 
    return 18;
}


static int DefineQuantizationTableMarker (unsigned char *pbuf, int qscale, int format)
{
    int i, length, temp;
    // temp array to store scaled zigzagged quant entries 
    unsigned char newtbl[64];


    if (format == FORMAT_MONOCHROME)    // monochrome 
        length  =  67;
    else
        length  =  132;

    *pbuf++  =  0xFF;                   // define quantization table marker 
    *pbuf++  =  0xDB;
    *pbuf++  =  length>>8;              // length field 
    *pbuf++  =  length&0xFF;
     // quantization table precision | identifier for luminance 
    *pbuf++  =  0;                     

    // calculate scaled zigzagged luminance quantisation table entries 
    for (i=0; i<64; i++) {
        temp = (JPEG_StdQuantTblY[i] * qscale + 16) / 32;
        // limit the values to the valid range 
        if (temp <= 0)
            temp = 1;
        if (temp > 255)
            temp = 255;
        newtbl[zigzag[i]] = (unsigned char) temp;
    }

    // write the resulting luminance quant table to the output buffer 
    for (i=0; i<64; i++)
        *pbuf++ = newtbl[i];

    // if format is monochrome we're finished, 
    // otherwise continue on, to do chrominance quant table 
    if (format == FORMAT_MONOCHROME)
        return (length+2);

    *pbuf++ = 1;   // quantization table precision | identifier for chrominance 

    // calculate scaled zigzagged chrominance quantisation table entries 
    for (i=0; i<64; i++) {
        temp = (JPEG_StdQuantTblC[i] * qscale + 16) / 32;
        // limit the values to the valid range 
        if (temp <= 0)
            temp = 1;
        if (temp > 255)
            temp = 255;
        newtbl[zigzag[i]] = (unsigned char) temp;
    }

    // write the resulting chrominance quant table to the output buffer 
    for (i=0; i<64; i++)
        *pbuf++ = newtbl[i];

    return (length+2);
}

static int FrameHeaderMarker(char *pbuf, int width, int height, int format)
{
    int length;
    if (format == FORMAT_MONOCHROME)
        length = 11;
    else
        length = 17;

    *pbuf++= 0xFF;                      // start of frame: baseline DCT 
    *pbuf++= 0xC0;
    *pbuf++= length>>8;                 // length field 
    *pbuf++= length&0xFF;
    *pbuf++= 0x08;                      // sample precision 
    *pbuf++= height>>8;                 // number of lines 
    *pbuf++= height&0xFF;
    *pbuf++= width>>8;                  // number of samples per line 
    *pbuf++= width&0xFF;

    if (format == FORMAT_MONOCHROME)    // monochrome 
    {
        *pbuf++= 0x01;              // number of image components in frame 
        *pbuf++= 0x00;              // component identifier: Y 
        *pbuf++= 0x11;              // horizontal | vertical sampling factor: Y 
        *pbuf++= 0x00;              // quantization table selector: Y 
    }
    else if (format == FORMAT_YCBCR422) // YCbCr422
    {
        *pbuf++= 0x03;        // number of image components in frame 
        *pbuf++= 0x00;        // component identifier: Y 
        *pbuf++= 0x21;        // horizontal | vertical sampling factor: Y 
        *pbuf++= 0x00;        // quantization table selector: Y 
        *pbuf++= 0x01;        // component identifier: Cb 
        *pbuf++= 0x11;        // horizontal | vertical sampling factor: Cb 
        *pbuf++= 0x01;        // quantization table selector: Cb 
        *pbuf++= 0x02;        // component identifier: Cr 
        *pbuf++= 0x11;        // horizontal | vertical sampling factor: Cr 
        *pbuf++= 0x01;        // quantization table selector: Cr 
    }
    else                                // YCbCr420 
    {
        *pbuf++= 0x03;         // number of image components in frame 
        *pbuf++= 0x00;         // component identifier: Y 
        *pbuf++= 0x22;         // horizontal | vertical sampling factor: Y 
        *pbuf++= 0x00;         // quantization table selector: Y 
        *pbuf++= 0x01;         // component identifier: Cb 
        *pbuf++= 0x11;         // horizontal | vertical sampling factor: Cb 
        *pbuf++= 0x01;         // quantization table selector: Cb 
        *pbuf++= 0x02;         // component identifier: Cr 
        *pbuf++= 0x11;         // horizontal | vertical sampling factor: Cr 
        *pbuf++= 0x01;        // quantization table selector: Cr 
    }

    return (length+2);
	}

static int DefineHuffmanTableMarkerDC(char *pbuf, unsigned int *htable, 
                                                                int class_id)
{
    int i, l, count;
    int length;
    char *plength;

    *pbuf++= 0xFF;                  // define huffman table marker 
    *pbuf++= 0xC4;
    plength = pbuf;                 // place holder for length field 
    *pbuf++;
    *pbuf++;
    *pbuf++= class_id;              // huffman table class | identifier 

    for (l = 0; l < 16; l++)
    {
        count = 0;
        for (i = 0; i < 12; i++)
        {
            if ((htable[i] >> 8) == l)
                count++;
        }
        *pbuf++= count;             // number of huffman codes of length l+1 
    }

    length = 19;
    for (l = 0; l < 16; l++)
    {
        for (i = 0; i < 12; i++)
        {
            if ((htable[i] >> 8) == l)
            {
                *pbuf++= i;         // HUFFVAL with huffman codes of length l+1 
                length++;
            }
        }
    }

    *plength++= length>>8;          // length field 
    *plength = length&0xFF;

    return (length + 2);
}

static int DefineHuffmanTableMarkerAC(char *pbuf, unsigned int *htable, 
                                                                int class_id)
{
    int i, l, a, b, count;
    char *plength;
    int length;

    *pbuf++= 0xFF;                      // define huffman table marker 
    *pbuf++= 0xC4;
    plength = pbuf;                     // place holder for length field 
    *pbuf++;
    *pbuf++;
    *pbuf++= class_id;                  // huffman table class | identifier 

    for (l = 0; l < 16; l++)
    {
        count = 0;
        for (i = 0; i < 162; i++)
        {
            if ((htable[i] >> 8) == l)
                count++;
        }

        *pbuf++= count;          // number of huffman codes of length l+1 
    }

    length = 19;
    for (l = 0; l < 16; l++)
    {
        // check EOB: 0|0 
        if ((htable[160] >> 8) == l)
        {
            *pbuf++= 0;         // HUFFVAL with huffman codes of length l+1 
            length++;
        }

        // check HUFFVAL: 0|1 to E|A 
        for (i = 0; i < 150; i++)
        {
            if ((htable[i] >> 8) == l)
            {
                a = i/10;
                b = i%10;
                // HUFFVAL with huffman codes of length l+1
                *pbuf++= (a<<4)|(b+1);   
                length++;
            }
        }

        // check ZRL: F|0 
        if ((htable[161] >> 8) == l)
        {
        // HUFFVAL with huffman codes of length l+1 
            *pbuf++= 0xF0;              
            length++;
        }

        // check HUFFVAL: F|1 to F|A 
        for (i = 150; i < 160; i++)
        {
            if ((htable[i] >> 8) == l)
            {
                a = i/10;
                b = i%10;
                 // HUFFVAL with huffman codes of length l+1 
                *pbuf++= (a<<4)|(b+1); 
                length++;
            }
        }
    }

    *plength++= length>>8;              // length field 
    *plength = length&0xFF;
    return (length + 2);
}


//*****************************************************************************
//
//!     DefineRestartIntervalMarker
//!    
//!    \param1                      pointer to Marker buffer  
//!    \param2                      return interval
//!
//!     \return                      Length or error code                                
//
//*****************************************************************************
static int DefineRestartIntervalMarker(char *pbuf, int ri)
{

    *pbuf++= 0xFF;                  // define restart interval marker 
    *pbuf++= 0xDD;
    *pbuf++= 0x00;                  // length 
    *pbuf++= 0x04;
    *pbuf++= ri >> 8;               // restart interval 
    *pbuf++= ri & 0xFF;
    return 6;
}
static int ScanHeaderMarker(char *pbuf, int format)
{
    int length;
    
    if (format == FORMAT_MONOCHROME)
        length = 8;
    else
        length = 12;

    *pbuf++= 0xFF;                  // start of scan 
    *pbuf++= 0xDA;
    *pbuf++= length>>8;             // length field 
    *pbuf++= length&0xFF;
    if (format == FORMAT_MONOCHROME)// monochrome 
    {
        *pbuf++= 0x01;              // number of image components in scan 
        *pbuf++= 0x00;              // scan component selector: Y 
        *pbuf++= 0x00;              // DC | AC huffman table selector: Y 
    }
    else                            // YCbCr
    {
        *pbuf++= 0x03;              // number of image components in scan 
        *pbuf++= 0x00;              // scan component selector: Y 
        *pbuf++= 0x00;              // DC | AC huffman table selector: Y 
        *pbuf++= 0x01;              // scan component selector: Cb 
        *pbuf++= 0x11;              // DC | AC huffman table selector: Cb 
        *pbuf++= 0x02;              // scan component selector: Cr 
        *pbuf++= 0x11;              // DC | AC huffman table selector: Cr 
    }

    *pbuf++= 0x00;         // Ss: start of predictor selector 
    *pbuf++= 0x3F;         // Se: end of spectral selector 
    *pbuf++= 0x00;         // Ah | Al: successive approximation bit position 

    return (length+2);
}

static int CreateJpegHeader(char *header, int width, int height,
                            int format, int restart_int, int qscale)
{
    char *pbuf = header;
    int length;

    // SOI 
    *pbuf++= 0xFF;
    *pbuf++= 0xD8;
    length = 2;

    // JFIF APP0 
    length += JfifApp0Marker(pbuf);

    // Quantization Tables 
    pbuf = header + length;
    length += DefineQuantizationTableMarker((unsigned char *)pbuf, qscale, format);

    // Frame Header 
    pbuf = header + length;
    length += FrameHeaderMarker(pbuf, width, height, format);

    // Huffman Table DC 0 for Luma 
    pbuf = header + length;
    length += DefineHuffmanTableMarkerDC(pbuf, &JPEG_StdHuffmanTbl[352], 0x00);

    // Huffman Table AC 0 for Luma 
    pbuf = header + length;
    length += DefineHuffmanTableMarkerAC(pbuf, &JPEG_StdHuffmanTbl[0], 0x10);

    if (format != FORMAT_MONOCHROME)// YCbCr
    {
        // Huffman Table DC 1 for Chroma 
        pbuf = header + length;
        length += DefineHuffmanTableMarkerDC
                                        (pbuf, &JPEG_StdHuffmanTbl[368], 0x01);

        // Huffman Table AC 1 for Chroma 
        pbuf = header + length;
        length += DefineHuffmanTableMarkerAC
                                        (pbuf, &JPEG_StdHuffmanTbl[176], 0x11);
    }

    if (restart_int > 0)
    {
        pbuf = header + length;
        length += DefineRestartIntervalMarker(pbuf, restart_int);
    }

    pbuf = header + length;
    length += ScanHeaderMarker(pbuf, format);

    return length;
}

u16 MT9D111_Greate_Header(void)
{
	STMFLASH_Read(FLASH_SAVE_ADDR,(u32*)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
   memset(g_header, '\0', sizeof(g_header));
	g_header_length = CreateJpegHeader((char *)&g_header[0], SaveParaList.size[0],SaveParaList.size[1], 0, 0x0020, 9);
	return g_header_length;
}
//初始化
u8 MT9D111_Init(void)
{ 

  	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_GPIOD_CLK_ENABLE();			//开启GPIOD时钟  D6 RST
	
	GPIO_Initure.Pin=GPIO_PIN_6;           //PD6
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    HAL_GPIO_Init(GPIOD,&GPIO_Initure);     //初始化
 
	PCF8574_WriteBit(DCMI_PWDN_IO,0);		//POWER ON
	delay_ms(10);  
	CAMERA_RST=0;			//必须先拉低OV5640的RST脚,再上电
	delay_ms(10);
	CAMERA_RST=1;			//结束复位      
	SCCB_Init();			//初始化SCCB 的IO口 

	Page_16bit_WR_Reg(0,0X04,0X0607);
	read_reg=Page_16bit_RD_Reg(0,0X04);
	printf("reg:%d\r\n",read_reg);
	
	
  	return 0x00; 	//ok
} 
  



//操作部分位
uint8_t wrMT9D111RegBit(u8 Page, u8 regID, u16 C_Dat,u16 W_Dat)
{
	u16 temp;
	temp=Page_16bit_RD_Reg(Page,regID);
	temp &=~C_Dat;
	temp |= W_Dat; 
	Page_16bit_WR_Reg(Page,regID,temp);
	return 0;
}

void MT9D111_RGB_Config(void)
{
	u16 i;
	u16 reg;
	reg = sizeof(mt9d111_QVGA_RGB)/sizeof(int)/3;
	printf("refg=%d",reg);
	for(i=0;i<154;i++)//CHANGE_REG_NUM//(sizeof(mt9d111_QVGA))/3	154
	{
		if(mt9d111_QVGA_RGB[i][0]>2)
			{
				delay_ms(5);
			}
		else
		{
			 Page_16bit_WR_Reg(mt9d111_QVGA_RGB[i][0],mt9d111_QVGA_RGB[i][1],mt9d111_QVGA_RGB[i][2]);
		}
	}
	Page_16bit_WR_Reg(0,0x66,(58<<8)|(1));
	wrMT9D111RegBit(0,0x67,0x7f,2);
	wrMT9D111RegBit(0,0x65,1<<14,0<<14);
}

void MT9D111_Jpeg_Config(void)
{
	u16 i;
//	u16 reg;
	
	//读取上一次参数
	STMFLASH_Read(FLASH_SAVE_ADDR,(u32*)&SaveParaList,sizeof(SavePara_TypeDef)/4+((sizeof(SavePara_TypeDef)%4)?1:0));
	
	read_reg=Page_16bit_RD_Reg(1,0XC8);
	printf("reg:%d\r\n",read_reg);
	for(i=0;i<(sizeof(mt9d111_init_cmds_list))/sizeof(int)/3;i++)
	{
		if(mt9d111_init_cmds_list[i][0]>2)
			{
				delay_ms(500);
			}
		else
		{
			
			Page_16bit_WR_Reg(mt9d111_init_cmds_list[i][0],mt9d111_init_cmds_list[i][1],mt9d111_init_cmds_list[i][2]);
		}
	}
	read_reg=Page_16bit_RD_Reg(1,0XC8);
	printf("reg:%d\r\n",read_reg);
	for(i=0;i<(sizeof(mt9d111_capture_jpeg))/sizeof(int)/3;i++)
	{
		if(mt9d111_capture_jpeg[i][0]>2)
			{
				delay_ms(500);
			}
		else
		{
			if(i==28)//设置宽度，
			{Page_16bit_WR_Reg(mt9d111_capture_jpeg[i][0],mt9d111_capture_jpeg[i][1],SaveParaList.size[0]);}
			else if(i==30)//设置高度
			{Page_16bit_WR_Reg(mt9d111_capture_jpeg[i][0],mt9d111_capture_jpeg[i][1],SaveParaList.size[1]);}
			else 
			 Page_16bit_WR_Reg(mt9d111_capture_jpeg[i][0],mt9d111_capture_jpeg[i][1],mt9d111_capture_jpeg[i][2]);
		}
	}
	delay_ms(500);
	read_reg=Page_16bit_RD_Reg(1,0XC8);
	printf("reg:%d\r\n",read_reg);
	

	
	
	Page_16bit_WR_Reg(0x00,0x09,SaveParaList.exp*13);//设置曝光时间很短，正常为0x4e0
	
	Page_16bit_WR_Reg(0x01,0xC6,0XA743);//设置A对比度  只有 0（100%） 1（125%） 2（150%） 3（175%） 4（noise-reduction） 
	Page_16bit_WR_Reg(0x01,0xC8,0X0003);//
	
	Page_16bit_WR_Reg(0x01,0xC6,0XA744);//设置B对比度
	Page_16bit_WR_Reg(0x01,0xC8,0X0003);//
	
	
	delay_ms(500);

}



















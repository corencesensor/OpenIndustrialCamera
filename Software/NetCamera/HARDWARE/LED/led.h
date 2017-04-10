#ifndef _LED_H
#define _LED_H
#include "sys.h"

#define LED1 PAout(8)//图片传输指示灯
#define OUTPUT PEout(3)//IO输出
#define INPUT PEin(2)//IO输入

void LED_Init(void);
#endif


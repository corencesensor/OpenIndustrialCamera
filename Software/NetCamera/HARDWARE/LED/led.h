#ifndef _LED_H
#define _LED_H
#include "sys.h"

#define LED1 PAout(8)//ͼƬ����ָʾ��
#define OUTPUT PEout(3)//IO���
#define INPUT PEin(2)//IO����

void LED_Init(void);
#endif


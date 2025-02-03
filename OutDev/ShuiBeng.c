#include "stm32f10x.h" 

int Shift=0;

void ShuiBengInit(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	
	GPIO_InitTypeDef B;
	B.GPIO_Mode=GPIO_Mode_Out_PP;
	B.GPIO_Pin=GPIO_Pin_14;
	B.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&B);
	GPIO_ResetBits(GPIOB,GPIO_Pin_14);
};

void ShuiBengSwitch(void)
{
	if (Shift==0)
	{
		GPIO_SetBits(GPIOB,GPIO_Pin_14);
		Shift=1;
	}
	else
	{
		GPIO_ResetBits(GPIOB,GPIO_Pin_14);
		Shift=0;
	}
}

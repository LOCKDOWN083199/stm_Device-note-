/*
备注:一次通信信息长度最长控制在150字节,不能超255;
*/

#include "stm32f10x.h"
#include "OLED.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//定义字符串池长度
#ifndef StringPoolLen
#define StringPoolLen 200
#endif

#ifndef RevcDataLen
#define RevcDataLen 80
#endif

uint8_t RevcFlag=0;						//有x条数据待处理 
char TempData;
char IPAddr[13];
char RevcString[StringPoolLen];			//字符串池
char RevcData[RevcDataLen];				//解析的一个字符串
//uint16_t RevcStringLen=StringPoolLen;	//字符串长度(mod%)
uint16_t RevcStringIndex=0;				//记录当前字符串池中写入位置的游标
uint16_t RevcStringRead=0;				//记录当前字符串池中读出位置的游标
char AllowWork='0';						//允许工作
uint16_t RevcDataIndex=0;				//取出一条字符串的游标

//TCP用的变量
char RevcTCPFlag='0';					//标志正在接收tcp通信
char RecordTCPPrior=0x00;				//上一次接收的字节,用于识别是否进入tcp
char Record2TCPPrior=0x00;
uint16_t RevcTCPIPD=0;					//记录tcp传输数据量
uint8_t RevcTCPDeIPD;					//控制串口返回的信息头过滤量
char RevcTCPQueue[5];
uint8_t RevcTCPQueueIndex=0;			//控制RevcTCPQueue位 [0,3]
char *SubStrIndex=NULL;
char *SubStrHeadIndex=NULL;

void TCPString2Num(char T);

void UsartInit(void)
{
	RevcFlag=0;
	RevcStringIndex=0;
	RevcStringRead=0;
	RevcDataIndex=0;
	RevcTCPDeIPD=4;
	SubStrIndex=RevcString;
	SubStrHeadIndex=RevcString;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//*不能漏
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitTypeDef A;
	A.GPIO_Mode=GPIO_Mode_AF_PP;
	A.GPIO_Pin=GPIO_Pin_9 ;//| GPIO_Pin_10
	A.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&A);
	
	A.GPIO_Mode=GPIO_Mode_IPU;
	A.GPIO_Pin=GPIO_Pin_10 ;
	A.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&A);
	
	USART_InitTypeDef U;
	U.USART_BaudRate=115200;
	U.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
	U.USART_Mode=USART_Mode_Rx | USART_Mode_Tx;
	U.USART_Parity=USART_Parity_No;
	U.USART_StopBits=USART_StopBits_1;
	U.USART_WordLength=USART_WordLength_8b;
	USART_Init(USART1,&U);
	
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef N;
	N.NVIC_IRQChannel=USART1_IRQn;
	N.NVIC_IRQChannelCmd=ENABLE;
	N.NVIC_IRQChannelPreemptionPriority=1;
	N.NVIC_IRQChannelSubPriority=1;
	NVIC_Init(&N);
	
	USART_Cmd(USART1,ENABLE);//*不能漏
	
}

void Serial_Send(uint8_t B)
{
	USART_SendData(USART1,B);
	while (USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}

void Serial_SendString(char *string)
{
	for(int i=0;string[i]!='\0';i++)
	{
		Serial_Send(string[i]);
	}
}

void USART1_IRQHandler(void)
{
	//OLED_ShowString(2,1,"IT");
	if (USART_GetITStatus(USART1,USART_FLAG_RXNE)==RESET)
	{
		TempData=USART_ReceiveData(USART1);

		if (TempData!='\n' && TempData!='+')
		{
			if (RevcTCPFlag=='1')//正在接收tcp通信
			{
				if (RevcTCPDeIPD>0)//处理"+IPD,"
				{
					RevcTCPDeIPD--;
					RevcString[RevcStringIndex%StringPoolLen]=TempData;
				}
				else 
				{
					RevcString[RevcStringIndex%StringPoolLen]=TempData;
					TCPString2Num(TempData);
					
					if (RevcTCPIPD>0)RevcTCPIPD--;
					else if (RevcTCPIPD==0)
					{
						RevcTCPDeIPD=4;
						RevcTCPFlag='0';
						RevcString[++RevcStringIndex%StringPoolLen]='\r';
						TempData='\r';
						RevcFlag++;
					}
				}
			}
			else
			{
				RevcString[RevcStringIndex%StringPoolLen]=TempData;
			}

		}
		else if(TempData=='\n')
		{
			RevcFlag++;
			RevcStringIndex--;
		}
		else if(TempData=='+' && Record2TCPPrior=='\r' && RecordTCPPrior=='\n')
		{
			RevcTCPFlag='1';
			RevcString[RevcStringIndex%StringPoolLen]=TempData;
			RevcStringIndex--;
			RevcStringIndex--;
		}
		else
		{

		}
		
		RevcStringIndex++;
		Record2TCPPrior=RecordTCPPrior;
		RecordTCPPrior=TempData;
		
		if (RevcStringIndex/StringPoolLen>1)	//减小index的绝对值防溢出
		{
			RevcStringIndex=RevcStringIndex%StringPoolLen;
		}
		//OLED_ShowNum(1,3,RevcStringIndex,3);
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
	}
	
};

char* GetRevcData(void)	//从池中返回一条数据
{
	while(RevcString[RevcStringRead]!='\r')
	{
		RevcData[RevcDataIndex++]=RevcString[RevcStringRead];
		RevcStringRead++;
		if (RevcStringRead>=StringPoolLen)RevcStringRead=RevcStringRead%StringPoolLen;
	}
	RevcData[RevcDataIndex]='\0';
	
	OLED_ShowNum(1,3,RevcStringIndex,3);
	
	RevcStringRead++;
	
	//防绝对值溢出
	if (RevcStringRead/StringPoolLen>1)RevcStringRead=RevcStringRead%StringPoolLen;
	RevcDataIndex=0;
	
	return RevcData;
}

void CleanRevcData(void)
{
	int i=0;
	for(i=0;RevcData[i]!='\0';i++)
	{
		RevcData[i]=0x00;
	}
	RevcData[i]=0x00;
}

void TCPString2Num(char T)// 截取tcp反馈\,和\:中间的数字字符成字符串( ,123: )
{
	if (T!=':' && T>=0x30 && T<=0x39)
	{
		RevcTCPQueue[RevcTCPQueueIndex++]=T;
	}
	else if(T==':')
	{
		RevcTCPQueue[RevcTCPQueueIndex]='\0';
		RevcTCPIPD=atoi(RevcTCPQueue);
		RevcTCPQueueIndex=0;
	}
}

uint8_t GetRevcFlag(void)//返回待处理消息数
{
	return RevcFlag;
}

void P_RevcFlag(void)
{
	RevcFlag--;
}

void CleanRevcFlag(void)
{
	RevcFlag=0;
}

char* GetIPAddr(void)
{
	return IPAddr;
}

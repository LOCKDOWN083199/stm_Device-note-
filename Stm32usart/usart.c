/*
备注:一次通信信息长度最长控制在150字节,不能超255;
*/

#include "stm32f10x.h"
#include "OLED.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "Delay.h"
#include "usart.h"

char TempData;				//接收字节
Task Tasks[TaskQLen];		//接收消息组
TaskQueue Queue;			//接收消息队列
Task SendTasks[20];	//发送消息组
STaskQueue SendQueue;		//发送队列

char IpAddr[20];

void UsartInit(void)
{	
	for(int i=0;i<TaskQLen;i++)
	{
		Tasks[i].Done='1';
		Tasks[i].Len=0;
		Tasks[i].String[0]='\0';
		Tasks[i].IsTCP='0';
		Queue.Wait[i]=Tasks[i];
	}
	for(int i=0;i<TaskQLen;i++)
	{
		SendTasks[i].Done='1';
		SendTasks[i].Len=0;
		SendTasks[i].String[0]='\0';
		SendTasks[i].IsTCP='0';
		SendQueue.ATFlag[i]=0;
		SendQueue.Wait[i]=Tasks[i];
	}
	
	Queue.Pop_Index=Queue.Push_Index=0;
	
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
	if (USART_GetITStatus(USART1,USART_FLAG_RXNE)==RESET)
	{
		TempData=USART_ReceiveData(USART1);
		
		TaskPush(TempData);

		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
	}
	
};

//==================================


void TaskQueuePush(void)
{
	Queue.Wait[TaskQPushMod()].Done='0';//可执行
	Queue.Push_Index++;
}
void TaskPush(char T)//字符输入任务
{
	if ((T==0x0d || T==0x0a) && T!='>')
	{
		if (Queue.Wait[TaskQPushMod()].Len==0)
		{
			return;
		}
		else 
		{
			Queue.Wait[TaskQPushMod()].String[Queue.Wait[TaskQPushMod()].Len]='\0';
			if (strstr(Queue.Wait[TaskQPushMod()].String,"+IPD,")!=NULL)
			{
				Queue.Wait[TaskQPushMod()].IsTCP='1';
			}
			TaskQueuePush();
			return;
		}
	}
	else if (T=='>')
	{
		Queue.Wait[TaskQPushMod()].String[Queue.Wait[TaskQPushMod()].Len++]=T;
		Queue.Wait[TaskQPushMod()].String[Queue.Wait[TaskQPushMod()].Len]='\0';
		TaskQueuePush();
	}
	else
	{
		Queue.Wait[TaskQPushMod()].String[Queue.Wait[TaskQPushMod()].Len]=T;
		Queue.Wait[TaskQPushMod()].Len++;
	}
	
}
char* TaskQueuePop(void)
{
	if(Queue.Pop_Index>Queue.Push_Index)return"";
	/*
	if(Queue.Wait[Queue.Pop_Index].Done=='1')
	{
		return "";
	}*/
	char *Data=Queue.Wait[Queue.Pop_Index].String;
	if (Queue.Wait[Queue.Pop_Index].IsTCP=='1')
	{
		Data=strstr(Queue.Wait[Queue.Pop_Index].String,":");
		Data++;
		Queue.Wait[Queue.Pop_Index].IsTCP='0';
	}
	else
	{
		
	}
	Queue.Wait[Queue.Pop_Index].Done='1';//执行完成
	Queue.Wait[Queue.Pop_Index].Len=0;
	Queue.Pop_Index++;
	
	DeIndexVar();
	
	return Data;
}
void DeIndexVar(void)
{
	if (Queue.Pop_Index>=TaskQLen)
	{
		Queue.Pop_Index=Queue.Pop_Index%TaskQLen;
		Queue.Push_Index=Queue.Push_Index%TaskQLen;
	}
}
uint8_t GetTaskStrLen(void)
{
	if(Queue.Wait[Queue.Pop_Index].Done=='1')return 0;
	else return Queue.Wait[Queue.Pop_Index].Len;
}
uint8_t TaskQPushMod(void)
{
	return Queue.Push_Index%TaskQLen;
}
uint8_t TaskQPushDePop(void)
{
	return TaskQPushMod()-Queue.Pop_Index;
}
uint8_t GetQIndex(void)
{
	return Queue.Pop_Index;
}

//=====================

uint8_t SendQueueMod(void);

void SendTcpString(void)
{
	if(SendQueue.Pop_Index>SendQueue.Push_Index)return;
	if(SendQueue.ATFlag[SendQueue.Pop_Index]==1)return;
	
	Serial_SendString(SendQueue.Wait[SendQueue.Pop_Index].String);
	//Serial_Send(0x0d);Serial_Send(0x0a);
	
	SendQueue.Wait[SendQueue.Pop_Index].Done='1';
	SendQueue.Wait[SendQueue.Pop_Index].Len=0;
	SendQueue.Pop_Index++;
	
	if (SendQueue.Pop_Index>=TaskQLen)
	{
		SendQueue.Push_Index=SendQueue.Push_Index%TaskQLen;
		SendQueue.Pop_Index=SendQueue.Pop_Index%TaskQLen;
	}
}

uint8_t SendQueueMod(void)
{
	return SendQueue.Push_Index%TaskQLen;
}

void InsertSendByte(char B)
{
	SendQueue.Wait[SendQueueMod()].String[SendQueue.Wait[SendQueueMod()].Len++]=B;
}
void MakeSendTask(char *S)//(加入队列)串口通信，无关网络，可用于其他AT指令
{
	sprintf(SendQueue.Wait[SendQueueMod()].String,"%s%c%c",S,0x0d,0x0a);
	SendQueue.Wait[SendQueueMod()].Len=strlen(SendQueue.Wait[SendQueueMod()].String);
	SendQueue.Wait[SendQueueMod()].Done='0';
	SendQueue.Push_Index++;
}
void MakeSendATSendTask(char *S)//(加入队列)AT网络发送通信
{
	sprintf(SendQueue.Wait[SendQueueMod()].String,"AT+CIPSEND=%d%c%c",strlen(S),0x0d,0x0a);
	SendQueue.Wait[SendQueueMod()].Len=strlen(SendQueue.Wait[SendQueueMod()].String);
	SendQueue.Wait[SendQueueMod()].Done='0';
	SendQueue.Push_Index++;

	sprintf(SendQueue.Wait[SendQueueMod()].String,"%s%c%c", S, 0x0d,0x0a);
	SendQueue.Wait[SendQueueMod()].Len=strlen(SendQueue.Wait[SendQueueMod()].String);
	SendQueue.Wait[SendQueueMod()].Done='0';
	SendQueue.ATFlag[SendQueueMod()]=1;
	SendQueue.Push_Index++;
}
uint8_t SendQPushDePop(void)
{
	return SendQueue.Push_Index-SendQueue.Pop_Index;
}
void ResetATFlag(void)
{
	if (SendQueue.ATFlag[SendQueue.Pop_Index]==1)SendQueue.ATFlag[SendQueue.Pop_Index]=0;
}

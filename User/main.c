#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "ShuiBeng.h"
#include "OLED.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

void Work(const char* Data);			//声明业务函数

char ATRST[7]="AT+RST";
char ATMODE1[20]="AT+CWMODE_CUR=1";
char ATSEND[50]="AT+CIPSEND=%d%c%c";
char ATTCPCLOSE[20]="AT+CIPCLOSE";
char ATEnterWifi[40]="AT+CWJAP_DEF=\"16A1124\",\"Zyh20001124\"";
char ATTCP[40]="AT+CIPSTART=\"TCP\",\"192.168.0.105\",3456";

char Back[30];							
char arr[254];//公用

uint8_t Send_Msg_Len=0;
char Msg[30]="NUll!";

char CS[10]="JS000010";
uint8_t CSSend=0;
char IP[15]="0.0.0.0";
char WorkingStatus[4]="free";

int main(void)
{
	//uint32_t LivingCount=0;
	//uint64_t WorkTime=0;
	
	ShuiBengInit();
	//ShuiBengSwitch();
	OLED_Init();
	//OLED_ShowString(1,1,"Start");
	
	/**/
	UsartInit();
	
	MakeSendTask(ATTCPCLOSE);
	MakeSendTask(ATRST);
	MakeSendTask(ATEnterWifi);
	
	
	while(1)
	{
		if (TaskQPushDePop()>0)
		{
			sprintf(Back,"%s",TaskQueuePop());
			Work(Back);
			//OLED_ShowString(1,1,"                  ");
			OLED_ShowString(1,1,Back);
			
		}
		if (SendQPushDePop()>0)
		{
			SendTcpString();
		}
		/*
		if (LivingCount>=10000000)
		{
			MakeSendATSendTask("living");
			LivingCount=0;
		}
		LivingCount++;
		*/
	}
	
}

void Work(const char* Data)
{
	if(strstr(Data,"WIFI GOT IP")!=NULL)
	{
		MakeSendTask(ATTCP);
		CSSend=1;
	}
	else if(strstr(Data,"OK")!=NULL && strstr(Data,"SEND")==NULL && CSSend==1)
	{
		CSSend=0;
		sprintf(arr,"%s%s","CS:",CS);
		MakeSendATSendTask(arr);
	}
	else if(strstr(Data,"AAA")!=NULL)
	{
		sprintf(arr,"%s%s","status:",WorkingStatus);
		MakeSendATSendTask(arr);
	}
	else if(strstr(Data,"CS")!=NULL)
	{
		sprintf(arr,"%s%s","CS:",CS);
		MakeSendATSendTask(arr);
	}
	else if(strstr(Data,"CCC")!=NULL)
	{
		ShuiBengSwitch();
		if(strstr(WorkingStatus,"free"))strcpy(WorkingStatus,"busy");
		else strcpy(WorkingStatus,"free");
		sprintf(arr,"%s%s","switch:",WorkingStatus);
		MakeSendATSendTask(arr);
	}
	else if(strstr(Data,">")!=NULL)
	{
		ResetATFlag();
	}
}	

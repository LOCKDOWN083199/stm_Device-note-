#ifndef __USART_
#define __USART_

#define TaskStringLen 100
#define TaskQLen 20
typedef struct {
    char String[TaskStringLen];
    uint8_t Len; 
	uint8_t Done;
	uint8_t IsTCP;
} Task;

typedef struct {
    Task Wait[TaskQLen];
    uint8_t Push_Index;
    uint8_t Pop_Index;
} TaskQueue;

typedef struct {
    Task Wait[TaskQLen];
    uint8_t Push_Index;
    uint8_t Pop_Index;
	uint8_t ATFlag[TaskQLen];//标记哪些Wait被ATSend二阶段回复占用了(与发送队列序号同步)
} STaskQueue;//用于发送

void UsartInit(void);
void Serial_Send(uint8_t B);
void Serial_SendString(char *string);

uint8_t TaskQPushMod(void);
void DeIndexVar(void);
void TaskQueuePush(void);
void TaskPush(char T);
char* TaskQueuePop(void);
uint8_t GetTaskStrLen(void);
uint8_t TaskQPushDePop(void);
uint8_t GetQIndex(void);

void SendTcpString(void);
void MakeSendTask(char *S);//串口通信，无关网络
void MakeSendATSendTask(char *S);//AT网络发送通信
uint8_t SendQPushDePop(void);
void ResetATFlag(void);

#endif



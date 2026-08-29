#include "FreeRTOS_Demo.h" 

extern Motor_Status motor;      // 电机状态结构体（外部变量）
extern RS485_Status RS_Status;  // RS485通信状态结构体（外部变量）
extern uint16_t RXData;         // RS485接收到的原始数据（外部变量）


int16_t Speed=1500;
extern uint8_t buffer[256];
extern uint8_t RX_Status;

uint16_t Distance;
uint16_t Temperature;

// 任务函数声明
void Motor_Task(void *pvParam);    // 电机控制任务
void Serial_Task(void *pvParam);   // 串口打印任务
void OLED_Task(void *pvParam);     // OLED显示任务
void RS485_Task(void *pvParam);    // RS485通信任务


// 任务栈大小（单位：字，即4字节）
#define Motor_Task_SIZE   256
#define Serial_Task_SIZE  256
#define OLED_Task_SIZE    256
#define RS485_Task_SIZE   256


// 任务优先级（数值越大优先级越高）
#define RS485_Task_PRIORITY     2  
#define OLED_Task_PRIORITY      2 
#define Motor_Task_PRIORITY     3 
#define Serial_Task_PRIORITY    1 

// 任务句柄（用于任务控制和管理）
TaskHandle_t xMotor_TaskHandle;     // 电机任务句柄
TaskHandle_t xSerial_TaskHandle;    // 串口任务句柄
TaskHandle_t xOLED_TaskHandle;      // OLED任务句柄
TaskHandle_t xRS485_TaskHandle;     // RS485任务句柄
QueueHandle_t queue1;               // 队列句柄
QueueHandle_t queue2;

/**
 * @brief FreeRTOS任务创建函数
 * @param 无
 * @retval 无
 * @note 
 */
void FreeRTOS_TaskCreate(void)
{
    
    queue1 = xQueueCreate(10, sizeof(uint16_t));
		queue2 = xQueueCreate(10, sizeof(uint16_t));
        
    // 创建电机控制任务
    xTaskCreate(Motor_Task,
                "Motor",                    // 任务名称
                Motor_Task_SIZE,            // 栈大小
                NULL,                       // 任务参数（无）
                Motor_Task_PRIORITY,        // 优先级
                &xMotor_TaskHandle);        // 任务句柄

    // 创建RS485通信任务
     xTaskCreate(RS485_Task,
                 "RS485",
                 RS485_Task_SIZE,
                 NULL,
                 RS485_Task_PRIORITY,
                 &xRS485_TaskHandle);

     // 创建OLED显示任务
     xTaskCreate(OLED_Task,
                 "OLED",
                 OLED_Task_SIZE,
                 NULL,
                 OLED_Task_PRIORITY,
                 &xOLED_TaskHandle);

     // 创建串口打印任务
     xTaskCreate(Serial_Task,
                 "Serial",
                 Serial_Task_SIZE,
                 NULL,
                 Serial_Task_PRIORITY,
                 &xSerial_TaskHandle);
    
    vTaskStartScheduler();
}


/**
 * @brief 电机控制任务
 * @param pvParam 任务参数（未使用）
 * @retval 无
 * @note 调用电机延时处理函数，实现限位检测和方向切换
 * 每10ms执行一次
 */
void Motor_Task(void *pvParam)
{
    while(1)
    {
        Motor_Delay(&motor);        //电机延时处理（检测限位开关并切换方向）
        vTaskDelay(100);            //延时100ms，让出CPU
    }
}

/**
 * @brief RS485通信任务
 * @param pvParam 任务参数（未使用）
 * @retval 无
 * @note 循环读取距离寄存器和温度寄存器
 * 读取命令：从机地址0x12，功能码0x03（读保持寄存器）
 * 距离寄存器地址：0x0101，温度寄存器地址：0x0102
 */
 void RS485_Task(void *pvParam)
 {
     while(1)
     {
        RS485_ReadOneValue(0x0101,&Distance);
        RS485_ReadOneValue(0x0102,&Temperature);
        xQueueSend(queue1,&Distance, portMAX_DELAY); 
				xQueueSend(queue2,&Temperature, portMAX_DELAY); 
    }
 }
 /**
  * @brief OLED显示任务
  * @param pvParam 任务参数（未使用）
  * @retval 无
  * @note 显示格式：
  * 第一行：D:XXXmm （距离值，单位毫米）
  * 第三行：XX.X   （温度值，保留一位小数）
  */
 void OLED_Task(void *pvParam)
 {
	uint16_t distance;
	uint16_t temperature;
     while(1)
     {
            xQueueReceive(queue1,&distance,portMAX_DELAY);
						xQueueReceive(queue2,&temperature,portMAX_DELAY);
            OLED_ShowString(1,1,"Dis:");
            OLED_ShowNum(1,5,distance,3);
            OLED_ShowString(1,9,"mm");
						OLED_ShowString(3,1,"Temp:");
            OLED_ShowNum(3,6,temperature / 10,2);
            OLED_ShowChar(3,8,'.');
            OLED_ShowNum(3,9,temperature % 10,1);
            printf("DIS=%d\r\n TMP=%d\r\n",distance,temperature);
            vTaskDelay(100);
     }
}
void Serial_Task(void *pvParam)
{
    while(1)
    {
        if(RX_Status==1)
			{
				if(strcmp("up",(const char*)buffer)==0)
				{
					Speed+=100;
					Motor_SetSpeed(&motor,Speed);
				}
					
				else if(strcmp("down",(const char*)buffer)==0)
				{
					Speed-=100;
					Motor_SetSpeed(&motor,Speed);
				}
				
				else if(strcmp("stop",(const char*)buffer)==0)
				{
					Motor_Stop(&motor);																														
				}
					
				else if(strcmp("resume",(const char*)buffer)==0)
				{
					Motor_Resume(&motor);
				}
					
				else if(strcmp("forward",(const char*)buffer)==0)
				{
					motor.dir=Forward;
					Motor_SetDir(&motor);
				}
					
				else if(strcmp("backward",(const char*)buffer)==0)
				{
					motor.dir=Backward;
					Motor_SetDir(&motor);
				}
					
				memset(buffer,0,256);
				RX_Status=0;
				DMA_SetCurrDataCounter(DMA1_Channel3,256);
				DMA_Cmd(DMA1_Channel3,ENABLE);
			}
		}
         vTaskDelay(100);
}

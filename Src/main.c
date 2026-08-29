#include "stm32f10x.h"                  // Device header
#include "bsp_PWM.h"
#include "bsp_Motor.h"
#include "bsp_485.h"
#include "OLED.h"
#include "bsp_Serial.h"
#include "FreeRTOS_Demo.h"   
#include "stdio.h"
#include "string.h"

Motor_Status motor;

extern uint16_t RXData;
extern uint16_t Distance;
extern uint16_t Temperature;
extern RS485_Status RS_Status;



int main(void)
{
   		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

			Motor_Init(&motor,1500);
			Serial_Init();
			Limit_GPIO_Configuration();

			RS485_Init();

			OLED_Init();				
			FreeRTOS_TaskCreate();     // ¡û µ÷ÓÃ

   
}




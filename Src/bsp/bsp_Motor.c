#include "bsp_Motor.h"
#include "bsp_PWM.h"
#include "FreeRTOS.h"
extern volatile uint8_t Move_End;


/**
 * @brief 电机控制相关GPIO引脚初始化
 * @param 无
 * @retval 无
 * @note 初始化PB11（PWM控制）和PE12（方向控制）为推挽输出
 */
void Motor_GPIO_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	// 初始化PE12为推挽输出（电机方向控制）
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOE,&GPIO_InitStructure);
	
	// 初始化PB11为推挽输出（电机使能/控制）
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

/**
 * @brief 启动电机
 * @param motor 电机状态结构体指针
 * @retval 无
 * @note 若电机为禁用状态则先使能，然后清零PB11并启动TIM5
 */
void Motor_Start(Motor_Status*motor)
{
	if(motor->choice==Motor_DISABLE)
	{
		motor->choice=Motor_ENABLE;
	}

	GPIO_ResetBits(GPIOB,GPIO_Pin_11);
	TIM_Cmd(TIM5,ENABLE);
}

/**
 * @brief 停止电机
 * @param motor 电机状态结构体指针
 * @retval 无
 * @note 关闭TIM5并将电机状态设为禁用
 */
void Motor_Stop(Motor_Status*motor)
{
	TIM_Cmd(TIM5,DISABLE);
	motor->choice=Motor_DISABLE;
}

/**
 * @brief 恢复电机运行
 * @param motor 电机状态结构体指针
 * @retval 无
 * @note 重新开启TIM5并将电机状态设为使能
 */
void Motor_Resume(Motor_Status*motor)
{
	TIM_Cmd(TIM5,ENABLE);
	motor->choice=Motor_ENABLE;
}

/**
 * @brief 设置定时器自动重装载值（ARR）
 * @param Speed 目标转速，用于计算ARR = 7200000/Speed - 1
 * @retval 无
 * @note 通过修改ARR来改变PWM周期，从而改变电机转速
 */
void Motor_SetARR(uint16_t Speed)
{
	TIM_SetAutoreload(TIM5,7200000/Speed	-1);
	TIM_SetCompare1(TIM5,3600000/Speed);
}


/**
 * @brief 设置电机转速
 * @param motor 电机状态结构体指针
 * @param Speed 目标转速（需在Min_Speed和Max_Speed之间）
 * @retval 无
 * @note 若Speed为0则关闭定时器；否则限幅后更新ARR和CCR
 */
void Motor_SetSpeed(Motor_Status*motor,uint16_t Speed)
{
	if(Speed == 0)
  {
    TIM_Cmd(TIM5, DISABLE);
	motor->choice=Motor_DISABLE;
    return;
  }
	if(Speed<Min_Speed)
	{
		Speed=Min_Speed;
	}
	else if(Speed>Max_Speed)
	{
		Speed=Max_Speed;
	}
	motor->Motor_Speed = Speed;
	Motor_SetARR(Speed);
}

/**
 * @brief 设置电机方向
 * @param motor 电机状态结构体指针
 * @retval 无
 * @note 根据motor->dir设置PE12：Forward时清零，Backward时置1
 */
void Motor_SetDir(Motor_Status*motor)
{
	  TIM_Cmd(TIM5,DISABLE);
    if(motor->dir == Forward)
    {
        GPIO_ResetBits(GPIOE,GPIO_Pin_12);
    }
    else
    {
        GPIO_SetBits(GPIOE,GPIO_Pin_12);
    }
		TIM_Cmd(TIM5,ENABLE);
}

/**
 * @brief 电机初始化函数
 * @param motor 电机状态结构体指针
 * @param Speed 初始转速
 * @retval 无
 * @note 初始化PWM（ARR=7199，PSC=9）、GPIO、方向、速度，并启动电机
 */
void Motor_Init(Motor_Status*motor,uint16_t Speed)
{
	PWM_Init(7199,9);
	Motor_GPIO_Init();
	motor->dir = Forward;
	motor->choice=Motor_ENABLE;
	Motor_SetSpeed(motor,Speed);
	Motor_SetDir(motor);
	Motor_Start(motor);
}

/**
 * @brief 电机延时处理与方向切换
 * @param motor 电机状态结构体指针
 * @retval 无
 * @note 当Move_End标志为1时，延时20ms后检测PA13/PA14
 *       若任一引脚为低电平，则切换电机方向并清除Move_End标志
 */
void Motor_Delay(Motor_Status *motor)
{
	if(Move_End==1)
	{
		vTaskDelay(20);
		//Delay_ms(20);
		if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_13)==0)
		{
			motor->dir=Forward;
			Motor_SetDir(motor);
			Move_End=0;
		}

		if(GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_14)==0)
		{
			motor->dir=Backward;
			Motor_SetDir(motor);
			Move_End=0;
		}
		TIM_Cmd(TIM5,ENABLE);
	}
}


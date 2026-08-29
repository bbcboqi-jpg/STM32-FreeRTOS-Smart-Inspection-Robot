#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include "stm32f10x.h"
#include "bsp_Delay.h"
#include "bsp_Limit.h"

/**
 * @brief 电机使能状态枚举
 */
typedef enum
{
	Motor_ENABLE = 0,   /*!< 电机使能 */
	Motor_DISABLE       /*!< 电机禁用 */
} Motor_EN;

/**
 * @brief 电机旋转方向枚举
 */
typedef enum {
    Forward = 0,        /*!< 正转（前进） */
    Backward = 1        /*!< 反转（后退） */
} Motor_Direction;

/**
 * @brief 电机状态结构体
 */
typedef struct {
    Motor_EN choice;           /*!< 电机使能/禁用状态 */
    Motor_Direction dir;       /*!< 电机旋转方向 */
    uint16_t Motor_Speed;      /*!< 电机当前转速 */
} Motor_Status;

/* 转速限幅宏定义 */
#define Min_Speed 100    /*!< 最小转速（对应最大周期） */
#define Max_Speed 5000   /*!< 最大转速（对应最小周期） */

/* 函数声明及注释 */

/**
 * @brief 电机控制相关GPIO引脚初始化
 * @param 无
 * @retval 无
 */
void Motor_GPIO_Init(void);

/**
 * @brief 启动电机（使能定时器输出PWM）
 * @param motor 电机状态结构体指针
 * @retval 无
 */
void Motor_Start(Motor_Status *motor);

/**
 * @brief 停止电机（关闭定时器输出）
 * @param motor 电机状态结构体指针
 * @retval 无
 */
void Motor_Stop(Motor_Status *motor);

/**
 * @brief 恢复电机运行（重新开启定时器）
 * @param motor 电机状态结构体指针
 * @retval 无
 */
void Motor_Resume(Motor_Status *motor);

/**
 * @brief 设置定时器自动重装载值（ARR），改变PWM周期
 * @param Speed 目标转速，用于计算ARR = 7200000/Speed - 1
 * @retval 无
 */
void Motor_SetARR(uint16_t Speed);

/**
 * @brief 设置电机转速（进行限幅处理并更新ARR和CCR）
 * @param motor 电机状态结构体指针
 * @param Speed 目标转速
 * @retval 无
 */
void Motor_SetSpeed(Motor_Status *motor, uint16_t Speed);

/**
 * @brief 设置电机方向（根据结构体中的dir成员控制PE12电平）
 * @param motor 电机状态结构体指针
 * @retval 无
 */
void Motor_SetDir(Motor_Status *motor);

/**
 * @brief 电机初始化（包括PWM、GPIO、默认方向、默认转速并启动）
 * @param motor 电机状态结构体指针
 * @param Speed 初始转速
 * @retval 无
 */
void Motor_Init(Motor_Status *motor, uint16_t Speed);

/**
 * @brief 电机延时处理与限位检测后方向切换
 * @param motor 电机状态结构体指针
 * @retval 无
 * @note 当Move_End标志为1时，延时20ms，若检测到限位开关信号则切换电机方向
 */
void Motor_Delay(Motor_Status *motor);

#endif /* __BSP_MOTOR_H */
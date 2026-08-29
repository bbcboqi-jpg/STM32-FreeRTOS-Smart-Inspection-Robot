#include "stm32f10x.h"
#include "bsp_PWM.h"

/**
 * @brief 初始化TIM5的PWM输出功能
 * @param ARR 自动重装载值（决定PWM周期）
 * @param PSC 预分频器值（决定计数时钟频率）
 * @retval 无
 * @note 使用PA0作为PWM输出通道（TIM5_CH1）
 *       PWM模式1，占空比固定为50%（脉冲值 = ARR/2）
 *       注意：函数最后禁用了TIM5，需要外部调用TIM_Cmd(TIM5,ENABLE)来启动PWM输出
 */
void PWM_Init(uint16_t ARR, uint16_t PSC)
{
    // 使能GPIOA和TIM5时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

    // 配置PA0为复用推挽输出（TIM5_CH1）
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置定时器时基单元（计数器、预分频器、周期）
    TIM_TimeBaseInitTypeDef TIM_TimeInitStructure;
    TIM_TimeInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;      // 时钟分割：无
    TIM_TimeInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeInitStructure.TIM_Period = ARR;                      // 自动重装载值
    TIM_TimeInitStructure.TIM_Prescaler = PSC;                   // 预分频器
    TIM_TimeInitStructure.TIM_RepetitionCounter = 0;             // 重复计数器（高级定时器用）
    TIM_TimeBaseInit(TIM5, &TIM_TimeInitStructure);

    // 使能影子寄存器（ARR和CCR的预装载功能）
    TIM_ARRPreloadConfig(TIM5, ENABLE);   // ARR预装载使能
    TIM_OC1PreloadConfig(TIM5, ENABLE);   // CCR1预装载使能

    // 配置PWM输出通道1（OC1）
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);                        // 先初始化结构体为默认值
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;              // PWM模式1
    TIM_OCInitStructure.TIM_Pulse = ARR / 2;                       // 比较值（决定占空比=50%）
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // 输出使能
    TIM_OC1Init(TIM5, &TIM_OCInitStructure);

    // 注意：此处禁用了定时器，需要外部调用TIM_Cmd(TIM5, ENABLE)来启动PWM输出
    TIM_Cmd(TIM5, DISABLE);
}
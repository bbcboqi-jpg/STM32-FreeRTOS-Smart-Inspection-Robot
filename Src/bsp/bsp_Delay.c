#include "bsp_Delay.h"

/**
 * @brief 毫秒级延时函数（软件循环实现）
 * @param time 要延时的毫秒数（单位：毫秒）
 * @retval 无
 * @note 该延时基于软件循环，精度受系统时钟频率影响
 *       当前循环参数 i=12000 是在特定主频下调试得到的近似1ms延时
 *       若更换系统时钟或优化等级，需重新校准循环次数
 */
void Delay_ms(uint16_t time)
{
    uint16_t i = 0;
    while (time--)
    {
        i = 12000;          // 每毫秒的循环计数值（需根据实际主频调整）
        while (i--);        // 空循环消耗时间
    }
}
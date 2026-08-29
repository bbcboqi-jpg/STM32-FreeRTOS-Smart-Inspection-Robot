#ifndef __BSP_SERIAL_H
#define __BSP_SERIAL_H

#include <stdint.h>  // 提供uint8_t、uint16_t等类型定义

/**
 * @brief 初始化USART3串口（部分重映射：PC10-TX, PC11-RX）
 * @param 无
 * @retval 无
 */
void Serial_Init(void);

/**
 * @brief USART3接收中断服务函数
 * @param 无
 * @retval 无
 */
void USART3_IRQHandler(void);

/**
 * @brief 获取串口接收完成状态（并自动清零标志）
 * @param 无
 * @retval uint8_t 1表示有新数据，0表示无新数据
 */
uint8_t RCV_Status(void);

/**
 * @brief 重定向printf函数到USART3
 * @param ch 要发送的字符
 * @retval int 返回发送的字符（通常为ch）
 */
int fputc(int ch);

/**
 * @brief 获取当前接收缓冲区中的数据（不改变读取指针）
 * @param 无
 * @retval uint16_t 当前索引处的接收数据
 */
uint16_t RCV_DATA(void);

#endif
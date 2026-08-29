#ifndef __BSP_485_H
#define __BSP_485_H

#include "stm32f10x.h"                  // Device header
#include "bsp_Delay.h"
#include "FreeRTOS_Demo.h"

//Modbus协议
#define RS485_SLAVE_ADDR          0X12			//从机地址配置宏
#define Modbus_CMD				  0x03			//读寄存器命令码

//寄存器地址
#define Distance_ADDR			  0X0101		//距离数据寄存器地址
#define Temperature_ADDR			  0x0102		//温度数据寄存器地址

//通信帧结构宏
#define RS485_RX_FRAME_LEN		  7				//接收帧长度
#define RS485_TX_FRAME_LEN		  8				//发送帧长度
#define RS485_WRITE_FRAME_LEN	  8				//写操作发送的固定帧长
#define RS485_READ_FRAME_LEN	  8				//读操作发送的固定帧长

//硬件引脚映射
#define RS485_TX_PIN			  GPIO_Pin_9	//USART1接收引脚
#define RS485_RX_PIN			  GPIO_Pin_9	//USART1发送引脚
#define RS485_GPIO_PORT			  GPIOA			

//读取数量
#define MODBUS_READ_COUNT       0x0001  // 每次读取寄存器的个数







/**
 * @brief RS485通信状态结构体
 */
typedef struct
{
	uint8_t RS485_Busy;       /*!< 忙标志（发送/接收状态） */
	uint8_t RS485_RXFlag;     /*!< 接收完成标志 */
	uint8_t RS485_Count;      /*!< 已接收字节计数 */
	uint8_t RS485_Buffer[10]; /*!< 接收数据缓冲区 */
} RS485_Status;

/**
 * @brief 计算Modbus协议的CRC16校验值
 * @param _puBuf 待校验的数据缓冲区指针
 * @param _usLen 数据长度（字节数）
 * @retval uint16_t 计算得到的CRC校验值（高字节在前）
 */
uint16_t CRC16_Modbus(uint8_t *_puBuf, uint16_t _usLen);

/**
 * @brief 初始化RS485通信所需的GPIO引脚
 * @param 无
 * @retval 无
 */
void RS485_GPIO_Configuration(void);

/**
 * @brief 初始化RS485通信（USART1配置及中断）
 * @param 无
 * @retval 无
 */
void RS485_Init(void);

/**
 * @brief 通过RS485发送Modbus读写命令（带CRC校验）
 * @param uAddr 从机地址
 * @param uCmd 功能码（如0x03读保持寄存器，0x06写单个寄存器）
 * @param uReg 寄存器地址
 * @param uData 要写入的数据（读命令时通常为0）
 * @param len 待发送的字节数（固定为8，因为命令帧为8字节）
 * @retval 无
 */
void USART1_RS485_RW_Opr(uint8_t uAddr, uint8_t uCmd, uint16_t uReg, uint16_t uData, uint8_t len);

/**
 * @brief 解析RS485接收到的数据（提取有效数据并校验CRC）
 * @param 无
 * @retval 无
 * @note 从RS485_Buffer中取前7字节，校验前5字节的CRC，通过后更新RXData
 */
void RS485_RXData(void);

//将读取温度和距离的逻辑封装为一个函数，避免轮询增加代码量
void RS485_ReadOneValue(uint16_t regAddr, uint16_t *pValue);

#endif /* __BSP_485_H */
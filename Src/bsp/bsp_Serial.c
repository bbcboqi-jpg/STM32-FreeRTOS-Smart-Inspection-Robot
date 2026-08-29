#include "stm32f10x.h"
#include <stdint.h>
#include "bsp_Serial.h"
#include <stdio.h>


uint8_t RX_Status=0;
uint16_t buffer[256];
void Serial_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);

    GPIO_PinRemapConfig(GPIO_PartialRemap_USART3,ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;

    // TX (PC10) 复用推挽输出
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOC,&GPIO_InitStructure);
    
    // RX (PC11) 浮空输入
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOC,&GPIO_InitStructure);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate=115200;
    USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;
    USART_InitStructure.USART_Parity=USART_Parity_No;
    USART_InitStructure.USART_StopBits=USART_StopBits_1;
    USART_InitStructure.USART_WordLength=USART_WordLength_8b;
    USART_Init(USART3,&USART_InitStructure); 

    USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel=USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
    NVIC_Init(&NVIC_InitStructure);

    USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE);

    DMA_InitTypeDef dma;
    dma.DMA_BufferSize=256;
    dma.DMA_DIR=DMA_DIR_PeripheralSRC;
    dma.DMA_M2M=DMA_M2M_Disable;
    dma.DMA_MemoryBaseAddr=(uint32_t)buffer;
    dma.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
    dma.DMA_MemoryInc=DMA_MemoryInc_Enable;
    dma.DMA_Mode=DMA_Mode_Normal;
    dma.DMA_PeripheralBaseAddr=(uint32_t)&(USART3->DR);
    dma.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
    dma.DMA_PeripheralInc=DMA_PeripheralInc_Disable;
    dma.DMA_Priority=DMA_Priority_Medium;
    DMA_Init(DMA1_Channel3,&dma);
    DMA_Cmd(DMA1_Channel3,ENABLE);

    USART_Cmd(USART3, ENABLE);
}

void USART3_IRQHandler(void)
{
    if(USART_GetITStatus(USART3,USART_IT_IDLE)!=RESET)
    {
				USART_ReceiveData(USART3);
        USART_ClearITPendingBit(USART3,USART_IT_IDLE);
        DMA_Cmd(DMA1_Channel3,DISABLE);
        uint8_t len=256-DMA_GetCurrDataCounter(DMA1_Channel3);
        buffer[len]='\0';
				RX_Status=1;
    }
}


/**
 * @brief 重定向printf函数到USART3（实现fputc）
 * @param ch 要发送的字符
 * @param f 文件指针（未使用）
 * @retval int 返回发送的字符
 * @note 使用轮询方式发送，等待发送缓冲区空
 */
int fputc(int ch, FILE *f)
{
    USART_SendData(USART3, (uint8_t)ch);
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    return ch;
}
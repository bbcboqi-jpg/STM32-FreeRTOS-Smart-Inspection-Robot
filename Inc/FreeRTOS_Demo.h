#ifndef __FREERTOS_DEMO_H
#define __FREERTOS_DEMO_H

#include "stm32f10x.h"   
#include "FreeRTOS.h"
#include "task.h"
#include "bsp_PWM.h"
#include "bsp_Motor.h"
#include "bsp_485.h"
#include "OLED.h"
#include "bsp_Serial.h"  
#include <stdio.h>
#include <string.h>
#include "queue.h"

void FreeRTOS_TaskCreate(void);

#endif
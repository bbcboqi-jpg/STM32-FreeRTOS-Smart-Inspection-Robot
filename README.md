# STM32F107VCT6 + FreeRTOS 电机控制系统

基于 **STM32F107VCT6（ARM Cortex-M3，72MHz）** 与 **FreeRTOS** 构建的多任务嵌入式电机控制系统。

系统通过 **RS485（Modbus RTU）** 周期性读取距离、温度传感器数据，并通过 OLED 实时显示；同时支持通过 USART3 串口命令对电机进行**调速、换向、启停控制**。系统利用外部中断检测限位开关，在达到行程端点后自动切换电机方向，实现基本的行程保护。

---

## 项目功能

- 基于 FreeRTOS 的多任务系统设计
- RS485 + Modbus RTU 周期性读取传感器数据
- OLED 实时显示距离与温度
- USART3 + DMA 接收串口控制命令
- 电机速度调节
- 电机正反转控制
- 电机启停控制
- PC13 / PC14 外部中断实现限位检测
- 限位触发后自动切换电机方向
- 基于 BSP 层实现外设驱动与应用层解耦
- FreeRTOS Queue 实现任务间数据通信

---

## 硬件平台


| 项目     | 配置              |
| -------- | ----------------- |
| MCU      | STM32F107VCT6     |
| 内核     | ARM Cortex-M3     |
| 主频     | 72MHz             |
| Flash    | 256KB             |
| SRAM     | 64KB              |
| 仿真器   | ST-LINK V2        |
| 电机驱动 | TIM5_CH1 脉冲输出 |
| 电机脉冲 | PA0 / TIM5_CH1    |
| 电机方向 | PE12              |
| 电机使能 | PB11              |
| 限位开关 | PC13 / PC14       |
| RS485    | USART1            |
| OLED     | 软件模拟 I2C      |
| 调试串口 | USART3 + DMA      |

---

## 外设引脚


| 外设      | 引脚        | 功能               |
| --------- | ----------- | ------------------ |
| TIM5_CH1  | PA0         | 电机脉冲输出       |
| Motor DIR | PE12        | 电机方向控制       |
| Motor EN  | PB11        | 电机使能           |
| Limit 1   | PC13        | 限位检测           |
| Limit 2   | PC14        | 限位检测           |
| USART1    | PA9 / PA10  | RS485 TX / RX      |
| RS485 EN  | PA12        | RS485 收发使能     |
| OLED SCL  | PB8         | 软件 I2C 时钟      |
| OLED SDA  | PB9         | 软件 I2C 数据      |
| USART3    | PC10 / PC11 | 串口调试与命令接收 |

---

# 软件架构

系统基于 FreeRTOS 划分为 4 个任务：

```text
                         FreeRTOS Scheduler
                                │
          ┌─────────────────────┼─────────────────────┐
          │                     │                     │
          ▼                     ▼                     ▼
   ┌─────────────┐       ┌─────────────┐       ┌─────────────┐
   │ RS485_Task  │       │ Serial_Task │       │ Motor_Task  │
   │   优先级 2   │       │   优先级 1   │       │   优先级 3   │
   │ 采集距离/温度 │       │ 解析控制命令 │       │ 限位检测换向 │
   └──────┬──────┘       └──────┬──────┘       └──────┬──────┘
          │                     │                     │
          │ Queue               │ BSP接口             │ BSP接口
          ▼                     ▼                     ▼
   ┌─────────────┐       ┌─────────────┐       ┌─────────────┐
   │ Queue × 2   │       │ bsp_Motor   │       │ bsp_Motor   │
   │ 距离 / 温度  │       │ 调速/换向/启停│       │  + bsp_PWM   │
   └──────┬──────┘       └─────────────┘       └──────┬──────┘
          │                                           │
          │                                           ▼
          ▼                                    ┌─────────────┐
   ┌─────────────┐                            │ 电机 / 限位 │
   │  OLED_Task  │                            │    硬件执行 │
   │   优先级 2   │                            └─────────────┘
   │ OLED + printf│
   └──────┬──────┘
          │
          ▼
   ┌─────────────┐
   │ OLED 显示    │
   └─────────────┘
```

---

## FreeRTOS 任务


| 任务          | 优先级 | 栈大小 | 主要职责                                                      |
| ------------- | -----: | -----: | ------------------------------------------------------------- |
| `Motor_Task`  |      3 | 256 字 | 周期性调用`Motor_Delay()`，检测限位触发标志并自动切换电机方向 |
| `RS485_Task`  |      2 | 256 字 | 通过 Modbus RTU 循环读取距离、温度数据并发送到队列            |
| `OLED_Task`   |      2 | 256 字 | 从队列接收数据并显示距离、温度                                |
| `Serial_Task` |      1 | 256 字 | 解析 USART3 DMA 接收的 ASCII 命令并控制电机                   |

---

# 任务通信

RS485 任务负责采集传感器数据，并通过 FreeRTOS Queue 将数据发送给 OLED 任务。

```text
                    RS485_Task
                       │
             ┌─────────┴─────────┐
             │                   │
             ▼                   ▼
        距离数据               温度数据
             │                   │
             ▼                   ▼
         Queue 1              Queue 2
             │                   │
             └─────────┬─────────┘
                       ▼
                   OLED_Task
                       │
                       ▼
                  OLED 显示
```

系统使用两个队列分别传输距离和温度数据：

* 队列长度：10
* 数据类型：`uint16_t`
* 通信方式：`xQueueSend()` / `xQueueReceive()`

通过消息队列实现传感器数据采集与 OLED 显示之间的生产者-消费者解耦。

---

# RS485 / Modbus RTU

RS485 使用 USART1：

```text
TX        PA9
RX        PA10
收发使能   PA12
波特率     9600
数据格式   8-N-1
```

Modbus RTU 配置：


| 参数       | 配置         |
| ---------- | ------------ |
| 从机地址   | `0x12`       |
| 功能码     | `0x03`       |
| 距离寄存器 | `0x0101`     |
| 温度寄存器 | `0x0102`     |
| 波特率     | 9600         |
| 数据格式   | 8-N-1        |
| CRC        | CRC16 查表法 |

系统周期性读取：

```text
0x0101 → 距离数据
0x0102 → 温度数据
```

温度数据采用 **10 倍定点数**表示：`253 → 25.3 ℃`

---

# OLED 显示

OLED 使用软件模拟 I2C：

```text
SCL → PB8
SDA → PB9
```

OLED 任务从 FreeRTOS Queue 中读取传感器数据并进行显示。

运行时主要显示：

```text
Dis:XXXmm


Temp:XX.X
```

同时 OLED 任务中使用 `printf` 输出相关数据，方便调试。

---

# 电机控制

电机脉冲由：`TIM5_CH1 → PA0`输出。

方向控制：`PE12`

使能控制：`PB11`

---

## 电机调速

TIM5 使用预分频：`PSC = 9`

计数时钟为：`7.2 MHz`

通过修改 `ARR` 和 `CCR` 实现电机速度调节：

```text
ARR = 7200000 / Speed - 1
CCR = 3600000 / Speed
```

PWM 占空比固定约为 50%。

输出脉冲频率对应设置的 `Speed`。

默认速度：`1500 Hz`

---

# 电机方向控制

电机方向通过 PE12 输出高低电平进行控制。

为了避免电机换向过程中产生异常脉冲，换向过程为：

```text
关闭 TIM5
    ↓
切换方向电平
    ↓
重新使能 TIM5
```

这种方式可以避免在换向瞬间继续输出原方向的脉冲。

---

# 限位保护

系统使用PC13、PC14两个限位开关：

配置为上拉输入，并通过 EXTI 下降沿触发外部中断。

限位触发流程：

```text
限位开关触发
      ↓
 EXTI 外部中断
      ↓
设置 Move_End 标志
      ↓
 Motor_Task 检测
      ↓
切换电机方向
      ↓
清除 Move_End 标志
```

当电机运行到行程端点时，通过限位开关检测当前位置，并自动切换电机方向。

---

# 串口控制

USART3 用于调试以及电机控制命令接收：

```text
TX → PC10
RX → PC11
```

串口采用 DMA 接收 ASCII 命令。

## 支持的控制命令


| 命令       | 功能                |
| ---------- | ------------------- |
| `up`       | 电机速度 +100       |
| `down`     | 电机速度 -100       |
| `stop`     | 电机停止，关闭 TIM5 |
| `resume`   | 电机恢复运行        |
| `forward`  | 电机正转            |
| `backward` | 电机反转            |

例如：`up` —— 发送后电机速度增加 100
例如：`down` —— 发送后电机速度减少 100
例如：`stop` —— 关闭 TIM5，停止电机运行
例如：`resume` —— 恢复电机运行

---

# USART3 DMA 接收流程

USART3 使用 DMA 接收串口命令。

命令处理流程：

```text
USART3
   ↓
DMA 接收
   ↓
ASCII 命令
   ↓
Serial_Task
   ↓
命令解析
   ↓
调用 BSP 电机控制接口
   ↓
执行调速 / 换向 / 启停
```

命令处理完成后：

```text
清空接收缓冲区
        ↓
重置 DMA 计数器
        ↓
重新使能 DMA 接收
```

---

# BSP 驱动层

项目采用 BSP（Board Support Package）方式组织底层外设驱动。

主要驱动模块：


| 文件           | 功能                                   |
| -------------- | -------------------------------------- |
| `bsp_Motor.c`  | 电机初始化、调速、换向、启停、限位处理 |
| `bsp_PWM.c`    | TIM5 PWM 输出                          |
| `bsp_485.c`    | RS485 与 Modbus RTU                    |
| `bsp_Serial.c` | USART3 串口与 DMA                      |
| `Limit.c`      | 限位 GPIO 与 EXTI                      |
| `OLED.c`       | OLED 软件 I2C 与显示                   |
| `bsp_Delay.c`  | 软件毫秒延时                           |

通过 BSP 层将硬件驱动与 FreeRTOS 应用任务进行解耦，提高代码的模块化程度。

---

# 项目目录

```text
.
├── Drivers/
│   ├── Start/
│   │   └── STM32 启动文件及 Cortex-M3 内核文件
│   └── Library/
│       └── STM32 标准外设库
│
├── FreeRTOS/
│   └── FreeRTOS 内核源码及 portable 移植层
│
├── Inc/
│   ├── bsp/
│   │   └── BSP 驱动头文件
│   └── FreeRTOS_Demo.h
│
├── Src/
│   ├── bsp/
│   │   ├── bsp_Motor.c
│   │   ├── bsp_PWM.c
│   │   ├── bsp_485.c
│   │   ├── bsp_Serial.c
│   │   ├── Limit.c
│   │   ├── OLED.c
│   │   └── bsp_Delay.c
│   │
│   ├── FreeRTOS_Demo.c
│   ├── FreeRTOSConfig.h
│   ├── main.c
│   └── stm32f10x_it.c
│
├── Readme/
│   └── 工程相关说明文档
│
├── docs/
│   └── images/
│       └── 实物照片与演示素材
│
├── F107VCT6.uvprojx
├── F107VCT6.code-workspace
├── .gitignore
└── README.md
```

# 开发环境

## 编译环境

* Keil MDK
* ARMCC / AC5
* STM32 Standard Peripheral Library
* FreeRTOS

## 备用开发环境

* VSCode
* TRAE
* EIDE 插件

## 预定义宏

```text
USE_STDPERIPH_DRIVER
STM32F10X_CL
```

其中：`USE_STDPERIPH_DRIVER`

用于启用 STM32 标准外设库。

```text
STM32F10X_CL
```

用于指定 STM32F10x Connectivity Line 器件。

## 调试下载

使用：`ST-LINK V2`

通过 SWD 接口完成程序下载与调试。

---

# 编译与烧录

## 使用 Keil MDK

使用 Keil MDK 打开：`F107VCT6.uvprojx`

编译工程。

编译完成后生成：

```text
F107VCT6.hex
F107VCT6.axf
```

具体输出位置由工程构建配置决定。

---

## 使用 VSCode / TRAE + EIDE

打开：`F107VCT6.code-workspace`

安装 EIDE 插件后进行工程编译。

---

## 下载程序

使用：`ST-LINK V2`

连接 STM32 的 SWD 接口进行程序下载。

---

# 关键实现说明

## 1. Modbus RTU

`bsp_485.c` 使用 CRC16 查表法进行数据校验。

主要配置：

```text
从机地址：0x12
功能码：0x03
距离寄存器：0x0101
温度寄存器：0x0102
波特率：9600
数据格式：8-N-1
```

接收帧长度：`7 字节`

发送帧长度：`8 字节`

温度采用 10 倍定点数表示。

例如：`253 → 25.3℃`

---

## 2. 电机调速

TIM5 计数时钟为：`7.2 MHz`

通过修改：

```text
ARR
CCR
```

改变 PWM 输出频率。

计算公式：

```text
ARR = 7200000 / Speed - 1
CCR = 3600000 / Speed
```

输出占空比保持约 50%。

---

## 3. 电机换向

换向前关闭 TIM5：`TIM5 OFF`

然后改变 PE12 电平：`PE12 → 切换方向`

最后重新使能 TIM5：`TIM5 ON`

避免换向瞬间产生异常脉冲。

---

## 4. 限位保护

PC13 / PC14 配置为：

```text
上拉输入
EXTI
下降沿触发
```

中断发生后设置：`Move_End`

由 `Motor_Task` 在任务上下文中完成后续的方向切换。

---

## 5. 串口 DMA

USART3 采用 DMA 接收命令。

命令处理完成后重新初始化 DMA 接收计数并重新使能 DMA，从而实现连续的串口命令接收。

---

## 6. FreeRTOS Queue

RS485 任务采集到数据后：

```text
RS485_Task
     ↓
xQueueSend()
     ↓
Queue
     ↓
xQueueReceive()
     ↓
OLED_Task
```

通过 Queue 实现不同任务之间的数据传递。

---

# 注意事项

## 软件延时

`bsp_Delay.c` 中的 `Delay_ms()` 使用软件循环实现。

当前循环参数：`12000`

基于：`72MHz`

主频进行校准。

如果修改系统时钟频率或编译优化等级，需要重新校准延时参数。

---

## FreeRTOS 任务栈

当前任务栈大小：`256 word`

FreeRTOS 中任务栈单位为 word。

当前 Cortex-M3 环境下：`1 word = 4 bytes`

因此：`256 word = 1024 bytes`

如果任务中增加较大的局部变量或大量使用 `printf()`，需要注意任务栈溢出风险。

---

## FreeRTOS 调度器

执行：`vTaskStartScheduler();`

后，FreeRTOS 调度器启动。

正常情况下调度器不会返回，因此 `main()` 中位于该函数之后的代码不会继续执行。

---

## 中断上下文

`Motor_Delay()` 内部使用：`vTaskDelay();`

因此：`Motor_Delay()`

只能在任务上下文中调用。

**不能在中断服务函数中直接调用 `Motor_Delay()`。**

# License

本项目主要用于嵌入式学习、课程设计及个人项目实践。

项目中使用的第三方软件、库及组件请遵循其对应的许可证要求。

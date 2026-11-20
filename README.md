# 基于 STM32 的便携式多模态健康监测系统

以 **STM32F103RCT6**（Cortex-M3，72 MHz）为主控，基于 **FreeRTOS（CMSIS-RTOS v2）** 构建五任务并发的
健康监测原型：MAX30102 采集光电容积脉搏波（PPG）并解算心率 / 血氧，ADXL345 做动态阈值计步，
0.96 寸 OLED 本地显示，USART1 蓝牙模块周期上报，心率或血氧越限时声光报警。

## 系统结构

```text
MAX30102 (I2C1, PB6/PB7, 400 kHz) ─┐
ADXL345  (I2C2, PB10/PB11, 100 kHz)─┤
                                    ├─► FreeRTOS 五任务 ─┬─► OLED_Task   200 ms 刷新心率/血氧/步数
       共享数据 HealthData_t g_health│                     ├─► BLE_Task     1 s 组帧上报 USART1
       { heart, spo2, step_cnt }    ┘                     ├─► Alarm_Task  500 ms 复核阈值 → PC1/PC2/PC3
                                                          ├─► MAX30102_Task 20 ms 读 FIFO
                                                          └─► ADXL345_Task  20 ms 读三轴计步
```

## 任务划分

| 任务 | 周期 | 职责 |
|---|---|---|
| `MAX30102_Task` | 20 ms | 轮询 FIFO 数据就绪标志，读红光/红外原始值，攒满 1024 点触发 FFT 解算 |
| `ADXL345_Task` | 20 ms | 读三轴加速度，动态阈值 + 下降沿计步 |
| `OLED_Task` | 200 ms | 刷新心率、血氧、步数 |
| `BLE_Task` | 1 s | 组 ASCII 帧经 USART1 发蓝牙 |
| `Alarm_Task` | 500 ms | 复核心率与血氧，越限时驱动 LED 与蜂鸣器 |

任务在 `Core/Src/freertos.c` 中由 `osThreadNew()` 创建；共享数据为 `Core/Src/main.c` 里的
`HealthData_t g_health`。

## PPG 信号链（心率 / 血氧）

1. MAX30102 配置为 **SpO2 双 LED 模式**：`REG_MODE_CONFIG=0x03`，
   `REG_SPO2_CONFIG=0x27`（量程 4096 nA、LED 脉宽 411 µs），`REG_FIFO_CONFIG=0x0F`（不平均、不翻转）；
   LED 驱动电流 `0x32`（约 10 mA）。
2. 任务读 FIFO 6 字节（1 路红光 + 1 路红外），分别存入 `s1`（红光）与 `s2`（红外）的实部。
3. **去直流**：各减去 1024 点均值，去掉 0 Hz 分量。
4. **3 点加权滑动平均**：`(x[i-1] + 2x[i] + x[i+1]) / 4`，再做 **8 点平均** 压制高频噪声。
5. 双通道 FFT 取模，在 0–59 bin 内找最大模位置 `k`：`心率 = 60 × 100 × k / 1024`
   （采样率 100 Hz，分辨率约 5.86 bpm/bin）。
6. **血氧**：`R = (AC_ir × DC_red) / (AC_red × DC_ir)`，再代二次多项式
   `SpO2 = -45.060 R² + 30.354 R + 94.845`。
7. **无效信号判据**：低频段能量不足（`s1[i].real <= 10`）时把心率 / 血氧置 0，避免手指未放稳时输出乱值。

## 动态阈值计步

1. 每次读 4 组三轴，取均值作为 1 个样本，降低瞬时噪声。
2. 每 50 个样本统计三轴 max / min，动态阈值 = `(max + min) / 2`。
3. 选峰峰值最大且 ≥ 40 LSB 的轴作为"最活跃轴"，适应佩戴方向变化。
4. `old` / `new` 移位寄存器：`old` 无条件更新为 `new`，`new` 仅在 `|cur - new| > 30` 时更新。
5. 最活跃轴上出现 `old > 阈值` 且 `new < 阈值` 的下降沿，计 1 步。

## 外设配置

| 外设 | 配置 | 用途 |
|---|---|---|
| I2C1 | 400 kHz，PB6/PB7 | MAX30102、OLED |
| I2C2 | 100 kHz，PB10/PB11 | ADXL345（±2g，100 Hz 输出速率） |
| USART1 | 9600 8N1，PA9/PA10 | 蓝牙透传，1 Hz 上报 `HR/SpO2/Step` |
| GPIO PC1 / PC2 / PC3 | 推挽输出 | 蜂鸣器 / 双 LED 声光报警 |

报警阈值定义在 `Core/Inc/alarm.h`：心率 < 50 或 > 120、SpO2 < 93% 触发。

## 目录

| 路径 | 说明 |
|---|---|
| `Core/Src/main.c` | 初始化、共享数据 `g_health`、主循环 |
| `Core/Src/freertos.c` | 五个任务的创建与属性配置 |
| `Core/Src/max30102.c` / `max30102_Task.c` | 传感器驱动与 FIFO 采集任务 |
| `Core/Src/blood.c` | 去直流、滑动平均、FFT、心率与血氧解算 |
| `Core/Src/adxl345.c` / `adxl345_Task.c` | 加速度计驱动与计步 |
| `Core/Src/oled.c` / `oled_font.c` / `oled_Task.c` | OLED 驱动与刷新任务 |
| `Core/Src/ble_Task.c` / `usart.c` | 蓝牙上报帧与串口驱动 |
| `Core/Src/alarm.c` / `alarm_Task.c` | 阈值判断与声光输出 |
| `Core/Src/i2c.c` / `tim.c` / `gpio.c` | 外设初始化 |
| `Core/Inc/` | 各模块头文件与 `FreeRTOSConfig.h` |
| `Drivers/` | STM32F1xx HAL 驱动与 CMSIS |
| `Middlewares/Third_Party/FreeRTOS/` | FreeRTOS 内核与 CMSIS-RTOS v2 适配层 |
| `MDK-ARM/healthy_project.uvprojx` | Keil MDK 工程文件 |
| `healthy_project.ioc` | STM32CubeMX 工程配置 |

## 编译

用 Keil MDK（ARM Compiler 5）打开 `MDK-ARM/healthy_project.uvprojx`，F7 编译、F8 下载。
仓库不含编译产物（`MDK-ARM/healthy_project/` 已被忽略），首次编译会重新生成。
CubeMX 工程配置见根目录 `healthy_project.ioc`。

## 已知问题

以下问题在代码中真实存在，尚未修改（面试或二次开发前请先处理）：

1. **互斥锁未创建**：`main.c` 只有 `osMutexId_t healthMutexHandle;` 声明，全工程没有 `osMutexNew()`；
   五个任务调用的 `osMutexAcquire(healthMutexHandle, ...)` 实际传入 `NULL`，加锁并不生效。
   修复：在 `MX_FREERTOS_Init()` 创建任务前执行 `healthMutexHandle = osMutexNew(NULL);`。
2. **采样节奏与算法假设不一致**：`max30102_Task.c` 用 `osDelay(20)`，约 50 Hz 取点，而 `blood.c` 的心率换算
   按 100 Hz 计算，实测心率会偏大约一倍。修复：改 `osDelay(10)`，或每次读空 FIFO。
3. **红光 FFT 模被覆盖**：`blood.c` 第 129–130 行两行都写入 `s1[i].real`，第二行用 `s2`（红外）的结果覆盖了
   红光的结果，导致 `ac_red` 与 `ac_ir` 实际同源。修复：第二行改为 `s2[i].real = sqrtf(...)`。
4. **R 值方向与常用公式相反**：代码为 `R = (AC_ir × DC_red) / (AC_red × DC_ir)`，与常见定义
   `(AC_red × DC_ir) / (AC_ir × DC_red)` 相反，配套系数需一并确认。
5. **ADXL345 分辨率**：`DATA_FORMAT=0x00` 实际为 `FULL_RES=0`（10 位），代码注释写的"13 位"与配置不符；
   如需 13 位应改为 `0x08` 并重新验证。
6. 工程中没有温度传感器、跌倒检测、运动伪影去除；蓝牙为透传（手机端使用通用 BLE 调试助手，非自研 App）。
7. 未做与指夹血氧仪 / 心电设备的精度对比，因此代码与文档中不给出精度指标。

## 编码说明

源码注释为 **GBK** 编码（Keil 历史工程）。`.gitattributes` 已设置 `* -text`，禁止 Git 做换行与编码转换，
保证文件字节原样。若编辑器显示乱码，请切换为 GBK / ANSI 编码查看。

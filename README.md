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

## 代码修改记录（2026-11）

第一版代码完成于 2026 年，存在若干实现缺陷，已在 2026-11 统一修正。**以下修改尚未用 Keil 编译、未上板实测**，
验证状态以实际烧录结果为准。

1. **互斥量未创建** → 已在 `MX_FREERTOS_Init()` 的 `RTOS_MUTEX` 块内用 `osMutexNew()` 创建，失败则调用 `Error_Handler()`。
2. **采样节奏与算法假设不一致** → `max30102_Task.c` 改为 `osDelay(10)`（100 Hz），并在一个周期内用
   `for (drain = 0; drain < 3; drain++)` 把 FIFO 中积压的样本取空，避免调度抖动造成采样相位漂移。
3. **红光 FFT 模被红外覆盖** → `blood.c` 中第二行改为 `s2[i].real = sqrtf(...)`，`ac_red` 与 `ac_ir` 的数据源分离。
4. **R 值方向** → 改为 `R = (AC_red * DC_ir) / (DC_red * AC_ir)`，与 `-45.060R² + 30.354R + 94.845` 这套经验系数配套。
5. **ADXL345 分辨率** → `DATA_FORMAT` 由 `0x00` 改为 `0x08`（`FULL_RES=1`、±2g、13 位，3.9 mg/LSB）。
6. **计步任务周期失真** → `adxl345.c` 改为环形缓冲：每个任务周期只做一次 `FILTER_CNT` 次 I2C 读，
   峰值与滑动判断都在缓冲上完成（原实现每拍 50×4 = 200 次 I2C 读，100 kHz 总线上约 160 ms）。
7. **未初始化就使用** → `adxl345_Task.c` 中 `peak`、`cur_sample`、`slid` 全部零初始化。
8. **`ABS` 宏优先级错误** → 改为 `#define ABS(a) (((a) < 0) ? -(a) : (a))`；
   `detect_step` 的步数类型 `int8_t` → `uint16_t`（原静态计数到 127 会溢出）。
9. **无手指时误报警** → `alarm.c` 在 `heart <= 0 && spo2 <= 0` 时直接关灯、关蜂鸣器；
   `blood.c` 增加除零与有效谱线检查，心率超出 30–220 判为无效数据。
10. **OLED 任务占着互斥量写屏** → 锁内只取数据快照，`osMutexRelease()` 之后再调用 `OLED_ShowNum` / `OLED_Showdecimal`。

仍需注意 / 未实现：

1. 工程中没有温度传感器、跌倒检测、运动伪影去除；蓝牙为透传（手机端使用通用 BLE 调试助手，非自研 App）。
2. 未做与指夹血氧仪 / 心电设备的精度对比，因此代码与文档中不给出精度指标。
3. 心率与血氧仍采用固定系数的经验算法，个体差异（肤色、佩戴松紧、环境光）影响明显，不能作为医疗器械使用。

## 编码说明

源码注释为 **GBK** 编码（Keil 历史工程）。`.gitattributes` 已设置 `* -text`，禁止 Git 做换行与编码转换，
保证文件字节原样。若编辑器显示乱码，请切换为 GBK / ANSI 编码查看。

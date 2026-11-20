#ifndef _ALARM_H_
#define _ALARM_H_

#define HEART_LOW_TH      50     // 心率过低
#define HEART_HIGH_TH     120    // 心率过高
#define SPO2_LOW_TH       93.0f  // 血氧过低

extern void Health_Alarm_Check(int heart, float spo2);

#endif

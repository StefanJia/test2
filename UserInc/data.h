#ifndef _DATA_H
#define _DATA_H
#include "userinc.h"

typedef struct 
{
  int32_t raw;
  float ang_v;
  float line_v;
}DECODER_STRUCT;

typedef struct 
{
  MPU6050_DATA_STRUCT mpu6050;
  DECODER_STRUCT decoder1;
  DECODER_STRUCT decoder2;
}DATA_IN_STRUCT;

typedef struct 
{
  uint32_t channel1;
  uint32_t channel2;
  uint32_t channel3;
  uint32_t channel4;
}CHANNEL_STRUCT;

typedef struct 
{
  CHANNEL_STRUCT tim2;
  CHANNEL_STRUCT tim3;
  
}DATA_OUT_STRUCT;


extern DATA_IN_STRUCT indata;
extern DATA_OUT_STRUCT outdata;
void DataInput();
void DataSave();
void DataOutput();
void DataNoPut();
void DataProcess();



#endif


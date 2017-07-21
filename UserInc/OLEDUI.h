#ifndef _OLED_H
#define _OLED_H
#include "userinc.h"
#define MAX_PARA_SIZE	100 
#define MAX_SD_SIZE	50 

#define DOWN_IN         PAin(10)
#define UP_IN           PAin(10)
#define PUSH_IN         PAin(10)
#define PRESS_IN        PAin(10)

typedef struct 
{
  int32_t run_counts;
  int32_t set_time;
  struct
  {
    int32_t mid;
    int32_t max;
  }steer;
}PARA_LIST_STRUCT;

typedef enum 
{
  READY=0,
  RUNNING,
  BLOCKED,
  TIMEOUT
}SYS_STATUS_STRUCT;

typedef struct 
{
  SYS_STATUS_STRUCT status;
  uint32_t T_RUN;
  uint8_t osc_suspend;
  uint8_t sd_write;
  uint8_t force_stop;
}SYS_STRUCT; 

typedef enum
{
  NONE=0,
  PRESS,                
  CW,
  CCW,
  UP,
  DOWN,
  PUSH
}STATUS_BUTTON_STRUCT; 

typedef struct 
{
  int* para;
  char label[13];
  uint8_t precision;
}PARA_SHOW_STRUCT;

typedef struct 
{
  int* para;
  char label[13];
  uint8_t times;
}DATA_SAVE_STRUCT;

typedef struct 
{
  uint8_t changepara;
  uint16_t para_select;
  uint16_t para_num;
  int8_t showpage;
  int8_t showpage_max;
  int8_t showpage_min;
  uint16_t precision;
}OLED_STRUCT;

extern PARA_LIST_STRUCT setpara;
extern SYS_STRUCT sys;

void SysCheck();
void OledShow();
void CheckKey();
void DataWriteFatfs();
void DataNameWriteFatfs();
void UIInit();
void Para2Flash();
void SendOscilloscope();

#endif
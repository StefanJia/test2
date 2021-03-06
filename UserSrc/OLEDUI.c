#include "OLEDUI.h"
#include "oled.h"
#include "SDFatfs.h"
#include "flash.h"
#include "data.h"
#include "interrupt.h"

#define BUFF_TIME_MS 2000
#define LED_SYS_RUN     PEout(6)=0
#define LED_SYS_STOP    PEout(6)=1

PARA_LIST_STRUCT setpara = {0};
OLED_STRUCT oled = {0};
STATUS_BUTTON_STRUCT button;
SYS_STATUS_STRUCT sys_status;
SYS_STRUCT sys;

static void ChangePara(char event);
static void ShowUnder();
static void SysStop();

//parameters to be saved in flash should be listed here in order
int32_t* para_table[MAX_PARA_SIZE]={
  &setpara.run_counts,
  &setpara.set_time,
  &setpara.steer.mid,
  &setpara.steer.max,
  &setpara.test,
  
  {0}
};
//parameters to be shown on the screen should be listed here in order
PARA_SHOW_STRUCT para_show_table[MAX_PARA_SIZE]=      
{
  {&setpara.set_time,"SetTime",1},
  {&setpara.steer.mid,"SteerMid",1},
  {&setpara.steer.max,"SteerMax",1},
  {&setpara.run_counts,"Counts",1},
  {&setpara.test,"Test",1},
  
  {0}
};

//data to be saved in sd card should be listed here in order
#define F_PRINTF_D(data) f_printf(&fil,"%d\t",(data))
#define F_PRINTF_S(data) f_printf(&fil,#data"\t")
void DataNameWriteFatfs()
{
  F_PRINTF_S(sys.T_RUN);
           
  F_PRINTF_S(indata.mpu6050.acc_x);
  F_PRINTF_S(indata.mpu6050.acc_y);
  F_PRINTF_S(indata.mpu6050.acc_z);
  F_PRINTF_S(indata.mpu6050.gyr_x);
  F_PRINTF_S(indata.mpu6050.gyr_y);
  F_PRINTF_S(indata.mpu6050.gyr_z);
           
  F_PRINTF_S(outdata.euler.pitch);
  F_PRINTF_S(outdata.euler.roll);
  F_PRINTF_S(outdata.euler.yaw);
  
  f_printf(&fil,"\r\n");
}

void DataWriteFatfs()
{
  F_PRINTF_D(sys.T_RUN);
  
  F_PRINTF_D(indata.mpu6050.acc_x);
  F_PRINTF_D(indata.mpu6050.acc_y);
  F_PRINTF_D(indata.mpu6050.acc_z);
  F_PRINTF_D(indata.mpu6050.gyr_x);
  F_PRINTF_D(indata.mpu6050.gyr_y);
  F_PRINTF_D(indata.mpu6050.gyr_z);
  
  F_PRINTF_D((int)(100*outdata.euler.pitch));
  F_PRINTF_D((int)(100*outdata.euler.roll));
  F_PRINTF_D((int)(100*outdata.euler.yaw));
  
  f_printf(&fil,"\r\n");
}
//data to be sent through uart oscilloscope should be listed here in order
void SendOscilloscope()
{
  printf("%d,",indata.mpu6050.acc_x);
  printf("%d,",indata.mpu6050.acc_y);
  printf("%d,",indata.mpu6050.acc_z);
  printf("%d,",indata.mpu6050.gyr_x);
  printf("%d,",indata.mpu6050.gyr_y);
  printf("%d,",indata.mpu6050.gyr_z);
  
  printf("%d,",(int)(outdata.euler.roll *100));
  printf("%d,",(int)(outdata.euler.pitch*100));
  printf("%d,",(int)(outdata.euler.yaw  *100));  
  printf("\r\n");
}

void ShowUpper(int8 page)
{
  static int lastpage2;
  if(lastpage2!=page)
  {
    for(int i=0;i<5;LCD_ClearLine(i++));        
    lastpage2=page;
  }
  
  switch(page)
  {
  case 0:       
    oledprintf(0,0,"A X%4d,Y%4d,Z%4d",indata.mpu6050.acc_x>>8,indata.mpu6050.acc_y>>8,indata.mpu6050.acc_z>>8);
    oledprintf(1,0,"G X%4d,Y%4d,Z%4d",indata.mpu6050.gyr_x>>8,indata.mpu6050.gyr_y>>8,indata.mpu6050.gyr_z>>8);
    oledprintf(2,0,"E R%4d,P%4d,Y%4d",(int)outdata.euler.roll,(int)outdata.euler.pitch,(int)outdata.euler.yaw);
    oledprintf(3,0,"c1:%6d,c2:%6d",indata.decoder1.raw,indata.decoder2.raw);
    oledprintf(4,0,"AD:%5d,T:%4.1f",indata.adc10,T/1000.0f);
    break;
    
  case 1:
    
    break;
    
  case 2:
    
    break;
    
  default:
    break;
  }
}

void ForceParaChange()
{
  
}

void SysCheck()
{
  switch(sys.status)
  {
  case READY:break;
  case RUNNING:
    sys.T_RUN += T_PERIOD_MS;
    if(sys.T_RUN >= setpara.set_time*100 || 
                                              sys.force_stop == 1)
      sys.status = BLOCKED;

    break;
  case BLOCKED:
    sys.T_RUN += T_PERIOD_MS;
    if(sys.T_RUN >= setpara.set_time*100 + BUFF_TIME_MS ||
                                              sys.force_stop == 1)
    {
        SysStop();
    }
    break;
  case TIMEOUT:break;
  default:break;
  }
}
  
void SysRun()
{
  char filename[5] = {0};
  uint32_t t_last = T;
  
  if(sys.status == READY)
  {
    memset(&sys,0,sizeof(SYS_STRUCT)); 
    setpara.run_counts++;
    
    Para2Flash();
    
    sprintf(filename,"%d",setpara.run_counts);
    SDFatFSOpen(strcat(filename,".txt"));       //用到HAL_Delay() 不能关中断
    DataNameWriteFatfs();
    
    while(T - t_last < BUFF_TIME_MS);
    LCD_CLS();
    sys.sd_write = 1;
    LED_SYS_RUN;
    sys.status = RUNNING;
    DataOutput();
  }
  else
  {
    printf("Not Ready!\r\n");
  }
}

void SysStop()
{
  sys.status = READY;
  sys.sd_write = 0;
  SDFatFsClose();
  LED_SYS_STOP;
  char filename[5];
  
  sys.osc_suspend = 1;
  sprintf(filename,"%d",setpara.run_counts);
  SDFatFSRead(strcat(filename,".txt"));
}

/*************short*************long****************pro_long***/
/*press********确认*************运行****************停止运行**/
/*push*********改变精度****************************************/
/*up***********翻页*************保存参数************使用自定义参数*/
/*down*********翻页*************发送SD卡*************************/
/****************************************************************/
void CheckKey()
{
  uint32_t pushtime = T;
  
  if(button==PRESS||button==PUSH)
    OLED_Init();        
  
  switch(button)
  {
  case PRESS:                
    while(!PRESS_IN);
    if(T-pushtime<500)       
    {
      oled.changepara ^= 1;  
    }
    else if(T-pushtime<5000)
    {
      SysRun();
    }
    else
    {
      sys.force_stop = 1;
    }
    break;
    
  case PUSH:
    while(!PUSH_IN);
    if(T-pushtime<500)                              
    {
      oled.precision *= 10;
      if(oled.precision == 1000)
        oled.precision = 1;
    }
    else if(T-pushtime<5000)  
    {
      InitOffset6050(&indata.mpu6050,&mpu6050_offset);
    }
    else
    {
      
    }
    break;
    
  case UP:
    while(!UP_IN);
    if(T-pushtime<500)
    {
      if(oled.changepara)   
      {
        if(oled.para_select >0)
          oled.para_select --;
        else
          oled.para_select = oled.para_num-1;
      }
      else                 
      {
        if(oled.showpage > oled.showpage_min)
          oled.showpage --;
        else
          oled.showpage = 0;
      }
    }
    else if(T-pushtime<2000)
    {
      Para2Flash();
    }
    else
    {
      ForceParaChange();
    }
    break; 
    
  case DOWN:
    while(!DOWN_IN);
    if(T-pushtime<500)
    {
      if(oled.changepara)   
      {
        if(oled.para_select <oled.para_num-1)
          oled.para_select ++;
        else
          oled.para_select = 0;
      }
      else                 
      {
        if(oled.showpage < oled.showpage_max)
          oled.showpage ++;
        else
          oled.showpage = 0;
      }
    }
    else if(T-pushtime<2000)
    {
      if(sys.status == READY)
      {
        char filename[5] = {0};
        sys.osc_suspend = 1;
        sprintf(filename,"%d",setpara.run_counts);
        SDFatFSRead(strcat(filename,".txt"));
      }
    }
    else
    {
      
    }
    break;
    
  case CW:
    if(oled.showpage >= 0)
      {
        if(oled.changepara)
          ChangePara(1);
        else 
        {
          if(oled.para_select <oled.para_num-1)
            oled.para_select ++;
          else
            oled.para_select = 0;
        }
      }
    break;

  case CCW:
    if(oled.showpage >= 0)
      {
        if(oled.changepara)
          ChangePara(2);
        else
        {
          if(oled.para_select >0)
            oled.para_select --;
          else
            oled.para_select = oled.para_num-1;
        }
      }
    break;
  default:
    break;
  }
  button = NONE;
}

void OledShow()
{
  static int lastpage1;
  if(lastpage1!=oled.showpage)
  {
    LCD_CLS();
    lastpage1=oled.showpage;
    oled.para_select = 0;
  }   
  if(1) 
  {
    if(oled.showpage >= 0 && oled.showpage <= oled.showpage_max)
    {
      ShowUpper(oled.showpage);
      ShowUnder();
    }
    if(oled.showpage == -1)
    {
      
    } 
    if(oled.showpage == -2)
    { 
      
    }
  }
}

static void ShowUnder()
{
  int temp_para_select = oled.para_select;      
  if(temp_para_select>0)
  {
    oledprintf(5,0,"%02d.%-13s",temp_para_select-1,para_show_table[temp_para_select-1].label);
    oledprintf(5,96,"%5d",*para_show_table[temp_para_select-1].para);
  }
  else
  {
    LCD_ClearLine(5);
  }
  
  if(oled.changepara)
  {
    oledprintf(6,0,"%02d.%-13s",temp_para_select,para_show_table[temp_para_select].label);
    oledprintfw(6,96,"%5d",*para_show_table[temp_para_select].para);
  }
  else
  {  
    oledprintfw(6,0,"%02d.%-13s",temp_para_select,para_show_table[temp_para_select].label);
    oledprintf(6,96,"%5d",*para_show_table[temp_para_select].para);
  }
  
  if(temp_para_select<oled.para_num-1)
  {
    oledprintf(7,0,"%02d.%-13s",temp_para_select+1,para_show_table[temp_para_select+1].label);
    oledprintf(7,96,"%5d",*para_show_table[temp_para_select+1].para);
  }  
  else
  {
    LCD_ClearLine(7);
  }
}

static void ChangePara(char event) 
{    
  if(oled.showpage >= 0)
  {
    switch(event)
    {                
    case 1:
      *para_show_table[oled.para_select].para += para_show_table[oled.para_select].precision*oled.precision;
      break;
    case 2:
      *para_show_table[oled.para_select].para -= para_show_table[oled.para_select].precision*oled.precision;
      break;
    default:
      break;
    }
  }
} 

void Para2Flash()
{
  int32_t para_buff[MAX_PARA_SIZE];
  
  LCD_CLS();
  oledprintf(3,3,"Saving Flash");
  printf("flash save begin:\r\n");
  
  for(int i=0;i<MAX_PARA_SIZE;i++)
  {
    para_buff[i] = *para_table[i];
  }
  
  WriteFlash(para_buff, FLASH_USER_START_ADDR_1, MAX_PARA_SIZE);
  printf("flash save finish!\r\n");
  delay_ms(100);
  LCD_CLS();
}

void UIInit()
{
  int32_t para_buff[MAX_PARA_SIZE];
  
  OLED_Init();
  while(para_show_table[oled.para_num].precision)
    oled.para_num++;
  
  ReadFlash(para_buff, FLASH_USER_START_ADDR_1, MAX_PARA_SIZE);
  for(int i=0;i<MAX_PARA_SIZE;i++)
  {
    *para_table[i] = para_buff[i];
  }
  
  oled.precision = 1;
  oled.showpage_max = 3;
  oled.showpage_min = -2;
}
#include "data.h"
#include "OLEDUI.h"
#include "SDFatfs.h"
#include "usmart.h"
#include "mpu6050.h"
#include "interrupt.h"
#include "mpu6050_process.h"

DATA_IN_STRUCT indata;
MPU6050_PHYSICAL_STRUCT mpu6050_filted;
MPU6050_PHYSICAL_STRUCT mpu6050_offset;
MPU6050_EULER_STRUCT eulerRad;

#include "init.h"

DATA_IN_STRUCT indata;
DATA_OUT_STRUCT outdata;


void DataInput()
{
  
  //read raw
  MPU6050_GetData(&indata.mpu6050);
}

void DataProcess()
{
  //��̬�ں�
  MPU6050_Process(&indata.mpu6050, &mpu6050_filted, &mpu6050_offset, &eulerRad, &outdata.euler);

  
  
 outdata.tim2.channel2 = T/10%100;

}

void DataOutput()
{
  PWMStart();
}

void DataNoPut()
{
  PWMStop();
}

void DataSave()
{
  if(!sys.osc_suspend)
    SendOscilloscope();
  
  if(sys.sd_write)
    DataWriteFatfs();

}

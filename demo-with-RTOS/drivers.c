#include "usrinc.h"
#include "icoll.h"

////////////////////////////////////////////////////////////////////////////////
// UserEarlyConfig()����������������ִ�У���ʱ��û��RTOS���ڡ�
////////////////////////////////////////////////////////////////////////////////
bool_t UserEarlyConfig(void)
{
  // �����ʼ���жϿ���ģ��ʹ֮���á�
  if(!ICOLL_Init()){
    return FALSE;
  }
  // ������USART��ʱ��Դ����ΪPLL (BUG��UARTֻ����12MHZ������ʱ��Դ)��
  outl(1, REG_SET(HW_UARTCLKSEL));
  outl(1, REG_CLR(HW_UARTCLKUEN));
  outl(1, REG_SET(HW_UARTCLKUEN));
  
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// UserDriversInit()��������ȼ��߳���ִ�У�����ʹ��RTOS����
////////////////////////////////////////////////////////////////////////////////
bool_t UserDriversInit(void)
{
  if(!HW_PinMuxInit()){
    return FALSE;
  }
  if(!HW_GpioInit()){
    return FALSE;
  }
  if(!LED_Init()){
    return FALSE;
  }
  
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
void ARM_UndefinedException(u32_t regs[], int num)
{
  
}

void ARM_DataAbortException(u32_t regs[], int num)
{
  
}

void ARM_PrefAbortException(u32_t regs[], int num)
{
  
}

void ARM_IRQException(void)
{
  ICOLL_ProcessIRQ();
}

void ARM_FIQException(void)
{
  
}

u32_t ARM_SupervisorException(u32_t r0, u32_t r1, u32_t r2, u32_t r3)
{
  return r0;
}


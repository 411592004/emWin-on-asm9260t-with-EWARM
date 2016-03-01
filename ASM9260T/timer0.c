#include "usrinc.h"
#include "ASM9260T.h"

////////////////////////////////////////////////////////////////////////////////
static void TM0_IRQHandler(void);

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: TM0_Init
//| �������� |: ��ʼ��Timer0��ͨ����Ϊ1���붨ʱ����
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
bool_t TM0_Init(void)
{
  u32_t pclk;
  
  // ����Timer0ģ���ʱ�ӡ�
  outl((1<<4), REG_SET(HW_AHBCLKCTRL1));
  outl((1<<4), REG_CLR(HW_PRESETCTRL1));
  outl((1<<4), REG_SET(HW_PRESETCTRL1));
  // �ر�Timer0��ͨ��0��
  outl((1<<0), REG_CLR(HW_TIMER0_TCR));
  // Timer0��TC0��ÿ��PCLK�����ص�����
  outl((3<<0), REG_CLR(HW_TIMER0_CTCR));
  // Timer0��TC0����������
  outl((3<<0), REG_CLR(HW_TIMER0_DIR));
  // Timer0��TC0�����ƵΪ1��
  outl(0, REG_CLR(HW_TIMER0_PR));
  outl(0, REG_CLR(HW_TIMER0_PC));
  // MR0�ж�ʹ�ܣ�ƥ��ʱ��λTC0��
  outl((7<<0), REG_CLR(HW_TIMER0_MCR));
  outl((3<<0), REG_SET(HW_TIMER0_MCR));
  // ��MR0ƥ������Ϊ1���롣
  pclk = (inl(HW_SYSPLLCTRL)&0x1FF)*1000000u/4u;
  outl(pclk/1000u, HW_TIMER0_MR0);
  // ʹ��Timer0��ͨ��0��
  outl((1<<4), REG_SET(HW_TIMER0_TCR));
  outl((1<<4), REG_CLR(HW_TIMER0_TCR));
  outl((1<<0), REG_SET(HW_TIMER0_TCR));
  // ע���жϷ�������
  ICOLL_SetHandler(INT_TIMER0, TM0_IRQHandler);
  ICOLL_EnableIRQ(INT_TIMER0);
  
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: TM0_IRQHandler
//| �������� |:
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
void TM0_IRQHandler(void)
{
  u32_t ir = inl(HW_TIMER0_IR);
  if(ir & (1<<0)){
    // ϵͳʱ��������
    sys_tick++;
    OSTimeTick();
  }
  outl(ir, REG_SET(HW_TIMER0_IR));
}

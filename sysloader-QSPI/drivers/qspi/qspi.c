#include "usrinc.h"

////////////////////////////////////////////////////////////////////////////////
#define QUAD_MAX_BUSY_WAIT (0x00040000)
static volatile int quad_spi_cs_flag = 0;

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: QSPI_Init
//| �������� |:
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
bool_t QSPI_Init(void)
{
  // ʹ��QSPIʱ�ӡ�
  outl((1<<1), REG_SET(HW_AHBCLKCTRL1));
  // ��λQSPIģ�顣
  outl((1<<1), REG_CLR(HW_PRESETCTRL1));
  outl((1<<1), REG_SET(HW_PRESETCTRL1));
  // QSPIģ��ʱ��Ƶ��ΪPLL/4=120MHZ
  outl(4, HW_QUADSPI0CLKDIV);
  // ����DMA1ģ�顣
  outl((1<<10), REG_SET(HW_AHBCLKCTRL0));
  outl((1<<10), REG_CLR(HW_PRESETCTRL0));
  outl((1<<10), REG_SET(HW_PRESETCTRL0));
  outl(((1<<8)|(0<<0)), HW_DMA1_CHENREG);
  
  // ����QSPI��6���ܽŹ��ܡ�
  HW_SetPinFunc(11, 0, PIN_FUNC4);
  HW_SetPinFunc(11, 1, PIN_FUNC4);
  HW_SetPinFunc(11, 2, PIN_FUNC4);
  HW_SetPinFunc(11, 3, PIN_FUNC4);
  // ��QSPI_DAT2&QSPI_DAT3����Ϊ��ͨIO����Ϊʹ������SPIģʽ��
  HW_SetPinFunc(11, 4, PIN_FUNC0);
  HW_SetPinFunc(11, 5, PIN_FUNC0);
  HW_SetPinMode(11, 4, PIN_MODE_ENABLE);
  HW_SetPinMode(11, 5, PIN_MODE_ENABLE);
  HW_GpioSetDir(11, 4, FALSE);
  HW_GpioSetDir(11, 5, FALSE);
  
  // ����QSPI��SPIģʽ��ʹ��DMA��8λ����֡��10Mbps��
  outl(0x00002038, REG_VAL(HW_QSPI0_CTRL1));
  outl(0x00000205, REG_VAL(HW_QSPI0_TIMING));
  
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __quad_spi_pinmux_switch
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __quad_spi_pinmux_switch(int state)
{
  if(state){
    HW_SetPinFunc(11, 4, PIN_FUNC4);
    HW_SetPinFunc(11, 5, PIN_FUNC4);
  }else{
    HW_SetPinFunc(11, 4, PIN_FUNC0);
    HW_SetPinFunc(11, 5, PIN_FUNC0);
    HW_SetPinMode(11, 4, PIN_MODE_ENABLE);
    HW_SetPinMode(11, 5, PIN_MODE_ENABLE);
    HW_GpioSetDir(11, 4, FALSE);
    HW_GpioSetDir(11, 5, FALSE);
  }
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __quad_spi_lock_cs
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __quad_spi_lock_cs(void)
{
  quad_spi_cs_flag = 1;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __quad_spi_unlock_cs
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __quad_spi_unlock_cs(void)
{
  quad_spi_cs_flag = 0;
  outl((1<<27), REG_CLR(HW_QSPI0_CTRL0));
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_start_dma_tx
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __qspi_start_dma_tx(void *buf, u32_t bytes)
{
	u32_t temreg;
  
	temreg = ((0<<0)    //INT_EN, ch0 irq disable
    |(0<<1)      // DST_TR_WIDTH, des transfer width, should set to HSIZE, here is 000, means 8bit
      |(0<<4)      // SRC_TR_WIDTH, sor transfer width, should set to HSIZE, here is 000, means 8bit
        |(2<<7)      // DINC, des addr increment, des is SPI, so should set to 1x, means no change
          |(0<<9)      // SINC, sor addr increment, src is sram, so should set to 00, means to increase 
            |(1<<11)     // DEST_MSIZE, des burst length, set to 001 means 4 DST_TR_WIDTH per burst transcation
              |(1<<14)     // SRC_MSIZE, sor burst length, set to 001 means 4 SOR_TR_WIDTH per burst transcation
                |(1<<20)     // TT_FC,transfer type and flow control,001 means memory to peripheral,dma is flow controller
                  |(0<<23)     // DMS, des master select, 0 means ahb master 0
                    |(0<<25)     // SMS, sor master select, 1 means ahb master 1
                      |(0<<27)     // LLP_DST_EN, des block chaining enable, set to 0 disable it
                        |(0<<28));   // LLP_SOR_EN, sor block chaining enable, set to 0 disable it	
  outl((u32_t)buf, HW_DMA1_SAR0);	    // source address
  outl(HW_QSPI0_DATA, HW_DMA1_DAR0);  // destination adress
  
  outl(temreg, HW_DMA1_CTL0);
  outl(bytes, HW_DMA1_CTL0+4);
  
  outl(0, HW_DMA1_CFG0);
  outl(((2<<7)|(2<<11)), HW_DMA1_CFG0+4);
	
  outl(0x01, HW_DMA1_DMACFGREG);
  outl(((1<<8)|(1<<0)), HW_DMA1_CHENREG);
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_start_dma_rx
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __qspi_start_dma_rx(void *buf, u32_t bytes)
{
  u32_t temreg;
  
  temreg = ((0<<0)    //INT_EN, ch0 irq disable
    |(2<<1)      // DST_TR_WIDTH, des transfer width, should set to HSIZE, here is 010, means 32bit
      |(2<<4)      // SRC_TR_WIDTH, sor transfer width, should set to HSIZE, here is 010, means 32bit
        |(0<<7)      // DINC, des addr increment, des is sram, so should set to 00, means to increase
          |(2<<9)      // SINC, sor addr increment, src is SPI, so should set to 1x, means no change 
            |(0<<11)     // DEST_MSIZE, des burst length, set to 000 means 1 DST_TR_WIDTH per burst transcation
					    |(0<<14)     // SRC_MSIZE, sor burst length, set to 000 means 1 SOR_TR_WIDTH per burst transcation
                |(2<<20)     // TT_FC,transfer type and flow control,010 means peripheral to memory,dma is flow controller
                  |(0<<23)     // DMS, des master select, 0 means ahb master 0
                    |(0<<25)     // SMS, sor master select, 1 means ahb master 1
                      |(0<<27)     // LLP_DST_EN, des block chaining enable, set to 0 disable it
                        |(0<<28));    // LLP_SOR_EN, sor block chaining enable, set to 0 disable it
  outl(HW_QSPI0_DATA, HW_DMA1_SAR0);  // source address
  outl((u32_t)buf, HW_DMA1_DAR0);			// destination adress
  
  outl(temreg, HW_DMA1_CTL0);                                
  outl(((bytes+3)/4), HW_DMA1_CTL0+4);
  
  outl(0, HW_DMA1_CFG0);
  outl(((3<<7)|(3<<11)), HW_DMA1_CFG0+4);
  
  outl(0x01, HW_DMA1_DMACFGREG);
  outl(((1<<8)|(1<<0)), HW_DMA1_CHENREG);
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_write_buf
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static int __qspi_write_buf(void *buf, u32_t bytes)
{
  int wait_nops = QUAD_MAX_BUSY_WAIT;
  outl(bytes, HW_QSPI0_XFER);
  outl(((3u<<28)|(quad_spi_cs_flag<<27)), HW_QSPI0_CTRL0);
  __qspi_start_dma_tx(buf, bytes);
  while(((inl(HW_QSPI0_STATUS)&1)||(inl(HW_DMA1_CHENREG)&1))&&(wait_nops-->0));
  if(wait_nops <= 0){
    outl(((1<<8)|(0<<0)), HW_DMA1_CHENREG);
    return -1;
  }
  return bytes;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_read_buf
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static int __qspi_read_buf(void *buf, u32_t bytes)
{
  int wait_nops = QUAD_MAX_BUSY_WAIT;
  outl(bytes, HW_QSPI0_XFER);
  outl(((3u<<28)|(1u<<26)|(quad_spi_cs_flag<<27)), HW_QSPI0_CTRL0);
  __qspi_start_dma_rx(buf, bytes);
  while(((inl(HW_QSPI0_STATUS)&1)||(inl(HW_DMA1_CHENREG)&1))&&(wait_nops-->0));
  if(wait_nops <= 0){
    outl(((1<<8)|(0<<0)), HW_DMA1_CHENREG);
    return -1;
  }
  return bytes;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_w25q_write_enable
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __qspi_w25q_write_enable(int enable)
{
  u8_t qspi_cmd[4];
  qspi_cmd[0] = (enable ? 0x06 : 0x04);
  __quad_spi_lock_cs();
  __qspi_write_buf(qspi_cmd, 1);
  __quad_spi_unlock_cs();
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_w25q_read_status1
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static u8_t __qspi_w25q_read_status1(void)
{
  u8_t qspi_cmd[4];
  qspi_cmd[0] = 0x05;
  __quad_spi_lock_cs();
  __qspi_write_buf(qspi_cmd, 1);
  __qspi_read_buf(qspi_cmd, 4);
  __quad_spi_unlock_cs();
  return qspi_cmd[0];
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_w25q_read_status2
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static u8_t __qspi_w25q_read_status2(void)
{
  u8_t qspi_cmd[4];
  qspi_cmd[0] = 0x35;
  __quad_spi_lock_cs();
  __qspi_write_buf(qspi_cmd, 1);
  __qspi_read_buf(qspi_cmd, 4);
  __quad_spi_unlock_cs();
  return qspi_cmd[0];
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_w25q_quad_enable
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static void __qspi_w25q_quad_enable(int enable)
{
  u8_t qspi_cmd[4];
  
  qspi_cmd[0] = 0x01;
  qspi_cmd[1] = __qspi_w25q_read_status1();
  qspi_cmd[2] = __qspi_w25q_read_status2();
  if(enable){
    qspi_cmd[2] |= 0x02;    // ����QUAD_ENABLEλ
  }else{
    qspi_cmd[2] &= ~0x02;   // ���QUAD_ENABLEλ
  }
  __quad_spi_lock_cs();
  __qspi_write_buf(qspi_cmd, 3);
  __quad_spi_unlock_cs();
  while(__qspi_w25q_read_status1()&1){
    util_fastloop(1000);
  }
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: __qspi_read_flash_quad
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
static int __qspi_read_flash_quad(void *buf, u32_t faddr, u32_t bytes)
{
  u8_t qspi_cmd[8];
  
  // ��SPIģʽ�ͳ�6B���flash��ַ��������dumy�ֽڡ�
  outl(0x07, REG_CLR(HW_QSPI0_CTRL1));
  qspi_cmd[0] = 0x6B;
  qspi_cmd[1] = (faddr >> 16);
  qspi_cmd[2] = (faddr >> 8);
  qspi_cmd[3] = (faddr >> 0);
  qspi_cmd[4] = 0;
  __quad_spi_lock_cs();
  if(5 != __qspi_write_buf(qspi_cmd, 5)){
    __quad_spi_unlock_cs();
    return -1;
  }
  // ��QUADģʽ����W25QXX��������ݡ�
  outl((inl(HW_QSPI0_CTRL1)|2), HW_QSPI0_CTRL1);
  if(bytes != __qspi_read_buf(buf, bytes)){
    __quad_spi_unlock_cs();
    return -1;
  }
  __quad_spi_unlock_cs();
  return bytes;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: QSPI_ReadFlash
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |: 
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
int QSPI_ReadFlash(void *buf, u32_t faddr, u32_t bytes)
{
  u32_t n_read, n_left;
  u8_t *ptr;
  
  if(((u32_t)buf%4) || ((u32_t)bytes%4)){
    // �����ַ���ֽ���������4�ֽڶ��롣
    return -1;
  }
  ptr = buf;
  n_left = bytes;
  // ʹ��W25QXX��QUADģʽ����QUAD_ENABLE��
  __qspi_w25q_write_enable(TRUE);
  __qspi_w25q_quad_enable(TRUE);
  // ��QSPIģ��Ĺܽ��л�ΪQUADģʽ��only after QUAD_ENABLE!
  __quad_spi_pinmux_switch(TRUE);
  while(n_left){
    n_read = ((n_left > 2048) ? 2048 : n_left);
    if(__qspi_read_flash_quad(ptr, faddr, n_read) != n_read){
      break;
    }
    n_left -= n_read;
    faddr += n_read;
    ptr += n_read;
  }
  outl(0x07, REG_CLR(HW_QSPI0_CTRL1));
  // ��QSPIģ��Ĺܽ��л�ΪSPIģʽ��just before QUAD_DISABLE!
  __quad_spi_pinmux_switch(FALSE);
  // �ָ�W25QXXΪ��ͨģʽ����QUAD_DISABLE��
  __qspi_w25q_write_enable(TRUE);
  __qspi_w25q_quad_enable(FALSE);
  return (int)(bytes-n_left);
}


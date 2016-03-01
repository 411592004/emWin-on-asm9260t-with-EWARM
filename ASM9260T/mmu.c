#include "usrinc.h"
#include "ASM9260T.h"
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: MMU_Init
//| �������� |: 
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |:
//|          |:
////////////////////////////////////////////////////////////////////////////////
void MMU_Init(void)
{
  // ������������DCache
  MMU_TestCleanDCache();
  // ��ֹDCache
  MMU_DisableDCache();
  // ��ֹICache
  MMU_DisableICache();
  // ��ֹMMU
  MMU_DisableMMU();
  // ���DCache
  MMU_InvalidateDCache();
  // ���ICache
  MMU_InvalidateICache();
  // ���������TLB
  MMU_InvalidateTLB();
  
  // 16KB��ҳ�����㡣
  memset((void*)MMU_TTB, 0, TTB_SIZE);
  memcpy((void*)VECT_ADDR, (void*)EXEC_ADDR, VECT_SIZE);
  
  /* ��д1��ҳ����,��0����ǵ�ַ0��ӳ��,ͬ����Ҫ��֤ICache */
  // �����ַ0x200000000-0x20100000(SDRAM)ӳ�䵽0x0-0x100000
  pTTB[0] = (u32_t)((VECT_ADDR & 0xFFF00000) | MMU_RW_CB);

  // �����ַ0x20100000-0x22000000(SDRAM)ӳ�䵽0x00100000-0x02000000
  for(int i = 0x001; i < 0x020; i++)
  {
    pTTB[i] = (u32_t)(((0x200 + i) << 20) | MMU_RW_CB);
  }

  // �����ַ0x10000000-0x11000000(NOR)ӳ�䵽0x10000000-0x11000000
  for(int i = 0x100; i < 0x110; i++)
  {
    pTTB[i] = (u32_t)((i << 20) | MMU_RW_NCNB);
  }

  // �����ַ0x20000000-0x22000000((SDRAM))ӳ�䵽0x20000000-0x22000000
  for(int i = 0x200; i < 0x220; i++)
  {
    pTTB[i] = (u32_t)((i << 20) | MMU_RW_CB);
  }
  
  // �����ַ0x50000000-0x51000000((AHB����Ĵ���))ӳ�䵽0x50000000-0x51000000
  for(int i = 0x500; i < 0x510; i++)
  {
    pTTB[i] = (u32_t)((i << 20) | MMU_RW_NCNB);
  }

  // �����ַ0x40000000-0x40100000((SRAM))ӳ�䵽0x40000000-0x40100000
  pTTB[0x400] = (u32_t)((0x400 << 20) | MMU_RW_CB);
  
  // �����ַ0x80000000-0x81000000(�Ĵ�����ַ)ӳ�䵽0x80000000-0x81000000
  for(int i = 0x800; i < 0x810; i++)
  {
    pTTB[i] = (u32_t)((i << 20) | MMU_RW_NCNB);
  }
  
  // �����ַ0x90000000-0x90100000(�Ĵ�����ַ)ӳ�䵽0x90000000-0x90100000
  pTTB[0x900] = (u32_t)((0x900 << 20) | MMU_RW_NCNB);
  
  MMU_SetTTBase(MMU_TTB);
  MMU_SetDomain(0x55555550|MMU_DOMAIN1_ATTR|MMU_DOMAIN0_ATTR);
  MMU_SetProcessId(0x00<<25);   /* FCSE PID ����Ϊ0x00000000 */
  MMU_EnableAlignFault();
  MMU_EnableMMU();
  MMU_EnableICache();
  MMU_EnableDCache();
}


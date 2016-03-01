#include "usrinc.h"
#include <string.h>

// CRC32�㷨��CRC-32/ADCCP��
static u32_t crc32_gen(u8_t const data[], int len);

/** @addtogroup Glory_EmbeddedWorks
 * @{
 */
static u32_t rndSeed = 0;
u32_t RandomNumber(void)
{
  rndSeed = (rndSeed * 1664525) + 1013904223;
  return (rndSeed);
}
/**
 * function description
 *
 * @param  none
 * @return none
 *
 * @brief  none
 */
#define PRE_LOAD_NUM  (2048)
void UserEntryInit(void)
{
  volatile tImageSummary *summary = (void*)(EXEC_ADDR+64);
  u32_t imagesize, crcNow, crcBak;
  
__load_image:
  // ��ȡ����ժҪ
  if(PRE_LOAD_NUM != QSPI_ReadFlash((void*)EXEC_ADDR, LOAD_ADDR, PRE_LOAD_NUM)){
    util_fastloop(0x10000);
    LED_Toggle();
    goto __load_image;
  }
  // ��֤magic
  if(memcmp((char*)(summary->magic), "ASM920T", 8)){
    util_fastloop(0x10000);
    LED_Toggle();
    goto __load_image;
  }
  // ��ȡ�����ֽ���
  imagesize = summary->size;
  imagesize = (((imagesize+3)/4)*4);
  if(imagesize > PRE_LOAD_NUM){
    imagesize -= PRE_LOAD_NUM;
    // ��ȡʣ�ྵ������
    if(imagesize != QSPI_ReadFlash((void*)(EXEC_ADDR+PRE_LOAD_NUM), LOAD_ADDR+PRE_LOAD_NUM, imagesize)){
      util_fastloop(0x10000);
      LED_Toggle();
      goto __load_image;
    }
  }
  // ��֤�����CRC32
  crcBak = summary->crc32;
  summary->crc32 = 0; /* ��Ҫ��ժҪ��CRC��Ϊ0�ټ��� */
  crcNow = crc32_gen((u8_t*)EXEC_ADDR, summary->size);
  if(crcNow != crcBak){
    util_fastloop(0x10000);
    LED_Toggle();
    goto __load_image;
  }
  summary->crc32 = crcBak;
}

void UserEntryLoop(void)
{
  __boot_application(EXEC_ADDR);
  
  while(1)
  {
    LED_Toggle();
    util_fastloop(1000000);
  }
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: crc32_gen
//| �������� |: ��׼CRC32ʵ���㷨��
//|          |: CRC32�㷨��CRC-32/ADCCP��
//| �����б� |: 
//|          |: 
//| ��    �� |: 
//|          |: 
//| ��ע��Ϣ |: ��������ֵ����ȡ������������CRC32�롣
//|          |: 
////////////////////////////////////////////////////////////////////////////////
static u32_t const fcstab32[] =
{
  0x4DBDF21C, 0x500AE278, 0x76D3D2D4, 0x6B64C2B0,
  0x3B61B38C, 0x26D6A3E8, 0x000F9344, 0x1DB88320,
  0xA005713C, 0xBDB26158, 0x9B6B51F4, 0x86DC4190,
  0xD6D930AC, 0xCB6E20C8, 0xEDB71064, 0xF0000000
};

// CRC32�㷨��CRC-32/ADCCP��
u32_t crc32_gen(u8_t const data[], int len)
{
  u32_t crc = 0;
  while (len > 0){
    crc = (crc >> 4) ^ fcstab32[(crc ^ (*data >> 0)) & 0x0F];
    crc = (crc >> 4) ^ fcstab32[(crc ^ (*data >> 4)) & 0x0F];
    data++;
    len--;
  }
  return crc;
}

/**
* @}
*/


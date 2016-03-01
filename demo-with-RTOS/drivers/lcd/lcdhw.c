#include "usrinc.h"
#include "ASM9260T.h"
#include "lcdhw.h"

////////////////////////////////////////////////////////////////////////////////
// ��������ˮƽʱ�������������ʱ��Ϊ��λ��
#define LCD_HBP         (40-1)
#define LCD_HFP         (22-1)
#define LCD_HSW         (8-1)
#define LCD_PPL         (HW_LCD_X_SIZE-1)
#define DCLK_PL         (LCD_HBP+LCD_HFP+LCD_HSW+LCD_PPL+4)   // 550 DCLK
// �������ô�ֱʱ�����������ʱ��Ϊ��λ��
#define LCD_VBP         (12-1)
#define LCD_VFP         (10-1)
#define LCD_VSW         (6-1)
#define LCD_LPP         (HW_LCD_Y_SIZE-1)
#define LINE_PF         (LCD_VBP+LCD_VFP+LCD_VSW+LCD_LPP+4)   // 300 LINE

#define LCD_FPS         (60)
#define LCD_DCLK        (DCLK_PL*LINE_PF*LCD_FPS)             // Լ10MHZ DCLK

typedef enum {
  LCD_1BPP = 0,
  LCD_2BPP,
  LCD_4BPP,
  LCD_8BPP,
  LCD_12_16BPP,
  LCD_24BPP,
} BPPEnumDef;

#if HW_LCD_BPP == 32
#define LCD_BPP         (LCD_24BPP)
#else
#define LCD_BPP         (LCD_12_16BPP)
#endif
#define LCD_PKG_NUM     (16)

////////////////////////////////////////////////////////////////////////////////
typedef struct {
  u32_t NEXT;
  u32_t CHCMD;
  u32_t BUFFER;
  u32_t LCDCTRL0;
} LCD_DMAPKGTypeDef;

#pragma data_alignment=4
static u16_t colorpalette[256] @ ".ncnbram";  /* ��ɫ�壬256����Ŀ */
#if HW_LCD_BPP == 32
#pragma data_alignment=4
static u32_t framebuffer[HW_LCD_X_SIZE*HW_LCD_Y_SIZE] @ ".ncnbram";
#else
#pragma data_alignment=4
static u16_t framebuffer[HW_LCD_X_SIZE*HW_LCD_Y_SIZE] @ ".ncnbram";
#endif
static LCD_DMAPKGTypeDef PixelDMAPkg[LCD_PKG_NUM] @ ".ncnbram";
static LCD_DMAPKGTypeDef LUTblDMAPkg @ ".ncnbram";

////////////////////////////////////////////////////////////////////////////////
static void LCD_InitDMAPackage(void)
{
  LCD_DMAPKGTypeDef volatile *pDMAPkg;
  u32_t pkg_len, lcdctrl, address;
  
  // ��ʼ��������ɫ���ұ��DMA����
  memset(colorpalette, 0, sizeof(colorpalette));
  colorpalette[0] = (LCD_BPP << 12);                  // ���ڸ���Ӳ����ɫ���ұ�����͡�
  pDMAPkg = &LUTblDMAPkg;
  pkg_len = ((LCD_BPP == LCD_8BPP) ? 512 : 32);
  pDMAPkg->NEXT     = (u32_t)(&PixelDMAPkg[0]);
  pDMAPkg->CHCMD    = (0x000000c2 | (pkg_len<<16));
  pDMAPkg->BUFFER   = (u32_t)colorpalette;
  pDMAPkg->LCDCTRL0 = (0x30800000 | (pkg_len>>2));
  
  // ��ʼ������֡�������ݵ�DMA����
  address = (u32_t)framebuffer;
  pkg_len = ((sizeof(framebuffer))/LCD_PKG_NUM);
  lcdctrl = (0x2c800000 | (pkg_len>>2));              // RGB mode-32bit, so xfer is len/4.
  for(int i = 0; i < LCD_PKG_NUM; i++){
    pDMAPkg = &PixelDMAPkg[i];
    pDMAPkg->NEXT     = (u32_t)(pDMAPkg + 1);
    pDMAPkg->CHCMD    = (0x00001086 | (pkg_len<<16)); // one chain, one word pio write to lcdctrl.
    pDMAPkg->BUFFER   = address;
    pDMAPkg->LCDCTRL0 = lcdctrl;  // PIO Write Value, ��д��HW_LCD_CTRL_CTRL0��ֵ������
    address += pkg_len;
  }
  pDMAPkg->NEXT   = (u32_t)(&PixelDMAPkg[0]);
  pDMAPkg->CHCMD |= (1 << 6);     // decrement semaphore.
}

////////////////////////////////////////////////////////////////////////////////
static void LCD_DMAStartAPBH(void *pkg, u32_t chn)
{
  chn = (chn * 0x70);
  outl((u32_t)pkg, (HW_APBH_LCD_CH0_NXTCMDAR+chn));
  outl(1/* always 1 */, (HW_APBH_LCD_CH0_SEMA+chn));
}

////////////////////////////////////////////////////////////////////////////////
static void LCD_ResetTimingAndGo(void)
{
  u32_t dma_xfer;
  
  // LCD DMA init
  outl(~0u, REG_CLR(HW_APBH_LCD_CTRL1));
  outl((1u<<30), REG_CLR(HW_APBH_LCD_CTRL0));
  outl((1u<<31), REG_CLR(HW_APBH_LCD_CTRL0));
  
  // ����LCD���ʱ�ӡ�
  outl((1u<<31), REG_CLR(HW_LCD_CTRL_CTRL0));
  outl((1u<<30), REG_SET(HW_LCD_CTRL_CTRL0));
  outl((1u<<30), REG_CLR(HW_LCD_CTRL_CTRL0));
  
  // ����LCD�������ʱ��
  outl(((LCD_HBP<<24)|(LCD_HFP<<16)|(LCD_HSW<<10)|LCD_PPL), HW_LCD_CTRL_TIMING0);
  outl(((LCD_VBP<<24)|(LCD_VFP<<16)|(LCD_VSW<<10)|LCD_LPP), HW_LCD_CTRL_TIMING1);
  outl(((3<<24)|(12)), HW_LCD_CTRL_TIMING2);      // DCLK=120M/12=10MHZ��ƥ��LCD_DCLK��
  outl((LCD_PPL|(LCD_LPP<<16)), HW_LCD_CTRL_TIMING3);
  outl(((1u<<31)|(1<<29)|(LCD_LPP<<16)), HW_LCD_CTRL_SUBPANEL);
  outl(LCD_LPP, HW_LCD_CTRL_DISPLAYSTATUS);
  outl(128, HW_LCD_LOWTHESHOLD);
  outl(384, HW_LCD_UPTHESHOLD);
  
  // ����APBH��DMA��
  outl(0x03000700, REG_CLR(HW_APBH_LCD_CTRL0));   // open DMA CLKGATE
  outl(0x02000000, REG_SET(HW_APBH_LCD_CTRL0));   // burst 16

  // ������ɫ���ұ�
  LCD_DMAStartAPBH(&LUTblDMAPkg, 0);
  outl(0xfc900081, HW_LCD_CTRL_CTRL1);            // load palette, lcd_en=1 tft
  dma_xfer = ((LCD_BPP == LCD_8BPP) ? 256 : 16);
  outl((0x30000000|dma_xfer), HW_LCD_CTRL_CTRL0); // en_receive and startp and receive n*16bits
  while((inl(HW_LCD_CTRL_STAT)&0x00000040) == 0);
#if HW_LCD_BPP == 32
  // ע�⣺���LCD����Ϊ24λɫ����ôbit23����Ϊ0��������ɫ�����λ��
  outl(0xfc200080, HW_LCD_CTRL_CTRL1);            // load pixel data, lcd_en=0 tft
  outl(0xfc200081, HW_LCD_CTRL_CTRL1);            // load pixel data, lcd_en=1 tft
#else
  outl(0xfca00080, HW_LCD_CTRL_CTRL1);            // load pixel data, lcd_en=0 tft
  outl(0xfca00081, HW_LCD_CTRL_CTRL1);            // load pixel data, lcd_en=1 tft
#endif
  // ��ʼ�������ݵ�DMA���䡣
  outl(0x20000000, REG_SET(HW_APBH_LCD_CTRL0));   // DMA cycle enable
  outl(0x1c000000, REG_CLR(HW_APBH_LCD_CTRL0));
  LCD_DMAStartAPBH(&PixelDMAPkg[0], 0);
  outl(0x04000000, REG_SET(HW_APBH_LCD_CTRL0));   // CH0 cycle enable
  outl(0x03000000, REG_CLR(HW_APBH_LCD_CTRL0));
  outl(0x02000000, REG_SET(HW_APBH_LCD_CTRL0));   // burst 16
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: LCDHW_Init
//| �������� |:
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |:
//|          |: 
////////////////////////////////////////////////////////////////////////////////
bool_t LCDHW_Init(void)
{
  // ����LCDģ���ʱ�Ӳ���λ��
  outl((1<<14), REG_SET(HW_AHBCLKCTRL1));
  outl((1<<14), REG_CLR(HW_PRESETCTRL1));
  outl((1<<14), REG_SET(HW_PRESETCTRL1));
  outl(4, HW_LCDCLKDIV);  /* 4��ƵΪ120MHZ */
  // ��ʼ������LCD���Ĺܽš�
  for(int i = 4; i <= 7; i++){
    HW_SetPinFunc(1, i, PIN_FUNC6);
  }
  for(int i = 0; i <= 7; i++){
    HW_SetPinFunc(2, i, PIN_FUNC6);
  }
  for(int i = 0; i <= 7; i++){
    HW_SetPinFunc(3, i, PIN_FUNC6);
  }
  for(int i = 0; i <= 7; i++){
    HW_SetPinFunc(4, i, PIN_FUNC6);
  }
  // LCD��屳������ߡ�
  HW_SetPinFunc(5, 0, PIN_FUNC0);
  HW_SetPinMode(5, 0, PIN_MODE_ENABLE);
  HW_GpioSetDir(5, 0, TRUE);
  HW_GpioSetVal(5, 0);
  
  // ��ʼ��LCDģ�鲢����������
  LCD_InitDMAPackage();
  LCD_ResetTimingAndGo();

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: LCDHW_GetVRam
//| �������� |:
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |:
//|          |: 
////////////////////////////////////////////////////////////////////////////////
u32_t LCDHW_GetVRam(int index)
{
  return (u32_t)framebuffer;
}

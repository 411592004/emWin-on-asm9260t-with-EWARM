#include "usrinc.h"
#include "DIALOG.h"
#include "GUI.h"

// http://www.segger.com/emwin-samples.html
extern void MainTask(void);

////////////////////////////////////////////////////////////////////////////////
//|          |
//| �������� |: task_ui
//| �������� |: GUIӦ���߳�ѭ����
//|          |:
//| �����б� |:
//|          |:
//| ��    �� |:
//|          |:
//| ��ע��Ϣ |: GUI�̶߳�ջ������SDRAM�У���Ϊ����Ҫ�ϴ��ջ�ռ䣡
//|          |:
////////////////////////////////////////////////////////////////////////////////
OS_STK task_gui_stk[TASK_GUI_STK] @ ".noinit";
void task_gui(void *p_arg)
{
  // ��ʼ��LCD������Ӳ����
  if(!LCDHW_Init()){
    sys_suspend();
  }
  
  // emWin��ʱ���롣
  MainTask();
}


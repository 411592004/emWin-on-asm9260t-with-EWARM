@echo off

::ɾ��������Ŀ¼�е�TempFilesĿ¼
for /f "tokens=*" %%a in ('dir /ad/b/s^|find /i "TempFiles"') do rd /q/s "%%a"

::ɾ��������Ŀ¼�е�settingsĿ¼
for /f "tokens=*" %%a in ('dir /ad/b/s^|find /i "settings"') do rd /q/s "%%a"

::�ݹ�ɾ��������Ŀ¼�е�*CustomSfr.sfr�ļ�
del /f /s /q *CustomSfr.sfr

::�ݹ�ɾ��������Ŀ¼�е�*.dep�ļ�
del /f /s /q *.dep

::�ݹ�ɾ��������Ŀ¼�е�*.out�ļ�
del /f /s /q *.out
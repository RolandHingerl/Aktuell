@echo off
REM Batch zur Installation des SE Laserabgleichs (Installation)

taskkill /F /IM SELaserTrimming.exe

taskkill /F /IM LaserDrilling.exe

net share D:\SELasertrimming /Delete

if not exist D:\SELasertrimming goto Next1
rd D:\SELasertrimming /s /q

:Next1

xcopy D:\ContentStation\SELasertrimming\*.* D:\SELasertrimming\ /e /v /y

xcopy "D:\ContentStation\Autostart\*.*" "C:\Documents and Settings\All Users\Start Menu\Programs\Startup\" /e /v /y

net share SELasertrimming=D:\SELasertrimming /UNLIMITED


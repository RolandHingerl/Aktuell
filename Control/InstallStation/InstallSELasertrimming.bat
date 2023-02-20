@echo off
REM Batch zur Installation des SE Laserabgleichs
Echo -----------------------------------
Echo -     I n s t a l l a t i o n     -  
Echo -       SE Lasertrimming          - 
Echo -        BaP/TEF3.49/Baetz        -
Echo -----------------------------------

set TargetIp=192.168.212.33

net use M: \\%TargetIp%\d$ /user:OpconAdmin OpconAdmin

rd M:\ContentStation\ /s /q

xcopy ContentStation\*.* M:\ContentStation\ /e /v /y

psexec \\%TargetIp% -u OpconAdmin -p OpconAdmin -s D:\ContentStation\InstallSELasertrimmingStation.bat

rd M:\ContentStation\ /s /q





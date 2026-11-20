@echo off

rem 链接器输出文件的文件名（不含路径和扩展名）
set linker_output_file_name=%1
echo %linker_output_file_name% & echo.

rem 生成升级文件
set bin_file=".\bin\%linker_output_file_name%.bin"
set ota_file=".\bin\%linker_output_file_name%.ota"
set key_file=".\board\htd_boot_key.h"
set ver_file=".\version.h"
rem 输出 .ota 文件, 对 .bin 文件进行修改
..\..\..\tools\dfu\htd_boot_head_make.exe -i %bin_file% -o %bin_file% -m 1 -k %key_file% -v %ver_file%
copy /Y %bin_file% %ota_file% 

rem 输出 .qlz 文件, 对 .dfu 文件进行 quicklz 压缩, 再加上头部信息
set qlz_file=".\bin\%linker_output_file_name%.qlz_ota"
set qlz_dfu_file=".\bin\%linker_output_file_name%.qlz"
del /f /q %qlz_file%
..\..\..\tools\dfu\quicklz_compress.exe %bin_file% %qlz_file%
..\..\..\tools\dfu\htd_boot_head_make.exe -i %qlz_file% -o %qlz_dfu_file% -m 2 -k %key_file% -v %ver_file% -t 3
del /f /q %qlz_file%

rem 输出 .flz 文件, 对 .dfu 文件进行 fastlz 压缩, 再加上头部信息
set flz_file=".\bin\%linker_output_file_name%.flz_ota"
set flz_dfu_file=".\bin\%linker_output_file_name%.flz"
del /f /q %flz_file%
..\..\..\tools\dfu\fastlz_compress.exe %bin_file% %flz_file%
..\..\..\tools\dfu\htd_boot_head_make.exe -i %flz_file% -o %flz_dfu_file% -m 2 -k %key_file% -v %ver_file% -t 4
del /f /q %flz_file%
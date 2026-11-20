@echo off

rem keil编译器绝对路径
set keil_compiler_include_floder=%1
rem 链接器输出文件的完整路径
set full_name_path=%2
rem 链接器输出文件的文件名（不含路径和扩展名）
set linker_output_file_name=%3
rem 输出文件的目录
for %%i in ("%full_name_path%") do set full_name_dir=%%~dpi

echo %keil_compiler_include_floder% & echo.
echo %full_name_path% & echo.
echo %linker_output_file_name% & echo.
echo %full_name_dir% & echo.

rem 删除 bin 文件夹里面所有文件
del /f /q ".\bin\*.*"

rem 输出 .bin 文件
%keil_compiler_include_floder%\..\bin\fromelf.exe --bin "%full_name_path%" --output ".\bin\%linker_output_file_name%.bin" 
%keil_compiler_include_floder%\..\bin\fromelf.exe -acd --interleave=source -o ".\bin\%linker_output_file_name%.disasm" "%full_name_path%"
rem 拷贝 axf、hex、map 文件
copy "%full_name_dir%\%linker_output_file_name%.axf" ".\bin\%linker_output_file_name%.axf"
copy "%full_name_dir%\%linker_output_file_name%.hex" ".\bin\%linker_output_file_name%.hex"
copy "%full_name_dir%\%linker_output_file_name%.map" ".\bin\%linker_output_file_name%.map"


rem 输出 .uf2 文件
for /f "usebackq delims=" %%i in (`..\..\..\tools\keil_scripts\read_macro.exe .\board\bootuf2_config.h CONFIG_BOOTUF2_FAMILYID`) do set bootuf2_family_id=%%i
for /f "usebackq delims=" %%i in (`..\..\..\tools\keil_scripts\read_macro.exe .\board\bootuf2_config.h CONFIG_BOOTUF2_START_ADDR`) do set bootuf2_start_addr=%%i
..\..\..\tools\dfu\uf2conv.exe ".\bin\%linker_output_file_name%.bin" -f %bootuf2_family_id% -b %bootuf2_start_addr% --output ".\bin\%linker_output_file_name%.uf2" 




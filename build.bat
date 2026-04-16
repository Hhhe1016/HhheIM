@echo off
:: 进入当前脚本所在目录
cd /d "%~dp0"

:: 如果没有 build 文件夹则创建
if not exist build mkdir build
cd build

:: 调用 VS2022 环境
call "E:\VS\VS2022\VC\Auxiliary\Build\vcvarsamd64_x86.bat"

:: 仅针对当前目录的上一级（即当前项目）进行 CMake 配置
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug ..

:: 编译当前生成的独立解决方案
for %%i in (*.sln) do msbuild /m "%%i" /p:Platform=x64 /p:Configuration=Debug

pause
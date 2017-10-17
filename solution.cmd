@echo off

pushd %~dp0
for /f "tokens=2 delims=( " %%i in ('findstr /c:"project(" CMakeLists.txt') do (
  set project=%%i
)

if not exist build (
  mkdir build
)
pushd build

if not exist msvc (
  mkdir msvc
)
pushd msvc

cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_CONFIGURATION_TYPES="Debug;Release" ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG% -DVCPKG_TARGET_TRIPLET=%VCPKG_DEFAULT_TRIPLET% ../..

if %errorlevel% == 0 (
  start %project%.sln
) else (
  pause
)

popd
popd
popd

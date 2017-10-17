cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(gtest_test VERSION 0.1.0 LANGUAGES CXX)

set(PROJECT_TEXT "GTest Test")
set(COMPANY_TEXT "Xnet Systems GmbH")
set(COPYING_TEXT "2017 Xnet Systems GmbH")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_CXX_FLAGS "/permissive- /std:c++17 ${CMAKE_CXX_FLAGS} /utf-8 /wd4530 /wd4577")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifestuac:NO /ignore:4099 /ignore:4098")
  add_definitions(-D_UNICODE -DUNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_ATL_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-unused -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fdiagnostics-absolute-paths -fcolor-diagnostics")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
  link_libraries(c++experimental)
endif()

configure_file(res/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
source_group(res FILES ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB_RECURSE sources src/*.h src/*.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX src FILES ${sources})

file(GLOB_RECURSE resources res/version.manifest res/version.rc)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/res PREFIX res FILES ${resources})
list(APPEND sources ${resources})

add_executable(${PROJECT_NAME} ${sources} ${resources} ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} src res)

find_package(PNG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE PNG::PNG)

find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)

enable_testing()
include(GoogleTest)
find_package(GTest REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::Main)
gtest_add_tests(TARGET ${PROJECT_NAME} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER build)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
  set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
else()
  set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
endif()

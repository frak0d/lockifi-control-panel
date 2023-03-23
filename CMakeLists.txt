cmake_minimum_required(VERSION 3.20)
project(lockifi-control-panel LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID MATCHES "(Clang|GNU)")
  add_compile_options(-Wall -Wextra -pedantic)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  add_compile_options("/W4")
endif()

add_executable(lockifi-control-panel main.cpp)

find_package(CURL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
target_link_libraries(lockifi-control-panel Qt::Core Qt::Widgets curl)

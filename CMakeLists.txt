cmake_minimum_required(VERSION 3.16)

# 项目名称
project(WeatherAnalysis VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Charts)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core)

# 【关键点 1】告诉 CMake 头文件在哪里找
# 如果不加这句，#include "mainwindow.h" 会报错说找不到文件
include_directories(
    src/ui
    src/network
    src/data
    src/utils
)

# 【关键点 2】列出所有源文件的 新路径
# 注意：main.cpp 在根目录，其他的要带上前缀 src/xx/
set(PROJECT_SOURCES
    main.cpp

    # 界面层
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.ui
    src/data/dbmanager.cpp
    src/data/dbmanager.h
    # 你将来要添加的文件（暂时先注释掉，等创建了再解开）
    src/network/weathermanager.cpp
    src/network/weathermanager.h
    src/utils/jsonhelper.cpp
    src/utils/jsonhelper.h

)


# 【修改后】直接调用生成可执行文件的命令，去掉 if 和 endif
qt_add_executable(WeatherAnalysis
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_link_libraries(WeatherAnalysis PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Charts
)
target_link_libraries(WeatherAnalysis PRIVATE Qt6::Core)
target_link_libraries(WeatherAnalysis PRIVATE Qt6::Core)

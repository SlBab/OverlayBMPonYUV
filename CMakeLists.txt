cmake_minimum_required(VERSION 3.10.0)
project(BmpOverlay VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
find_package(Threads REQUIRED)

add_executable(BmpOverlay sources/main.cpp sources/bmp_to_yuv.cpp sources/overlay.cpp sources/read_yuv.cpp)

# Подключаем заголовочные файлы из текущей директории
target_include_directories(BmpOverlay PRIVATE ${PROJECT_SOURCE_DIR}/sources)

# Подключаем библиотеку pthread
target_link_libraries(BmpOverlay PRIVATE Threads::Threads)
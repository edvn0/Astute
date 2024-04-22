cmake_minimum_required(VERSION 3.21)
project(stbimage)

add_library(stbimage STATIC
        include/stb_image.h
        include/stb_image_write.h
        include/stb_image_resize2.h
        src/stb_image.cpp
        src/stb_image_write.cpp
        src/stb_image_resize2.cpp
)
target_include_directories(stbimage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(stbimage::stbimage ALIAS stbimage)

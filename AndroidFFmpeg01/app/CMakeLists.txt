# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#FFMpeg配置
#FFmpeg配置目录
set(distribution_DIR ${CMAKE_SOURCE_DIR}/../../../../src/main/jniLibs)

# 编解码(最重要的库)
add_library(
            avcodec-57
            SHARED
            IMPORTED)
set_target_properties(
            avcodec-57
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libavcodec-57.so)


# 滤镜特效处理库
add_library(
            avfilter-6
            SHARED
            IMPORTED)
set_target_properties(
            avfilter-6
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libavfilter-6.so)

# 封装格式处理库
add_library(
            avformat-57
            SHARED
            IMPORTED)
set_target_properties(
            avformat-57
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libavformat-57.so)

# 工具库(大部分库都需要这个库的支持)
add_library(
            avutil-55
            SHARED
            IMPORTED)
set_target_properties(
            avutil-55
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libavutil-55.so)

# 音频采样数据格式转换库
add_library(
            swresample-2
            SHARED
            IMPORTED)
set_target_properties(
            swresample-2
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libswresample-2.so)

# 视频像素数据格式转换
add_library(
            swscale-4
            SHARED
            IMPORTED)
set_target_properties(
            swscale-4
            PROPERTIES IMPORTED_LOCATION
            ../../../../src/main/jniLibs/armeabi/libswscale-4.so)


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
#判断编译器类型,如果是gcc编译器,则在编译选项中加入c++11支持
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++11")
endif(CMAKE_COMPILER_IS_GNUCXX)


#配置编译的头文件
include_directories(src/main/jniLibs/include)




# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native-lib.cpp )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.


find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native-lib avcodec-57 avfilter-6 avformat-57 avutil-55 swresample-2 swscale-4

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
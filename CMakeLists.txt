cmake_minimum_required(VERSION 3.26)
project(microscopy_image_stitching)

set(CMAKE_CXX_STANDARD 17)

add_executable(mimgstch src/main.cpp
        src/Image_Stitching.cpp
        src/Image.cpp
        src/Panorama.cpp
        src/Correlation.cpp
        src/Correlation.h
        src/Dots.h
        src/FFT.cpp
        src/FFT.h
        src/Homography.h
        src/Process.cpp
        src/Process.h
        src/SVD.cpp
        src/SVD.h
        src/Homography.cpp
)

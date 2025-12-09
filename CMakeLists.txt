cmake_minimum_required(VERSION 3.10)
project(yapt)

option(FUNCTION_PARSING "Function Parsing" ON)

if(FUNCTION_PARSING)
    add_definitions(-DFUNCTION_PARSING)
endif()

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# Add include directory
include_directories(include)

#external dependencies
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

# Enable FetchContent
include(FetchContent)

# Declare OpenEXR repository to fetch
FetchContent_Declare(
        OpenEXR
        GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/openexr.git
        GIT_TAG v3.2.0
)

# Optionally, set build options
set(OPENEXR_BUILD_UTILS OFF)  # Turn off utilities
set(OPENEXR_BUILD_DOCS OFF)   # Turn off documentation

# Fetch and make the library available
FetchContent_MakeAvailable(OpenEXR)

find_package(CGAL REQUIRED COMPONENTS Core)

find_package(Threads REQUIRED)

# Find Intel MKL
set(MKL_ROOT "/opt/intel/oneapi/mkl/2025.2")
set(MKL_INCLUDE_DIR "${MKL_ROOT}/include")
set(MKL_LIBRARIES "${MKL_ROOT}/lib/intel64/libmkl_intel_lp64.so;${MKL_ROOT}/lib/intel64/libmkl_gnu_thread.so;${MKL_ROOT}/lib/intel64/libmkl_core.so")
list(APPEND CMAKE_PREFIX_PATH "${MKL_ROOT}/lib/cmake/mkl")

# Find PyTorch/libtorch
list(APPEND CMAKE_PREFIX_PATH "/opt/libtorch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Workaround: Remove FALSE from torch/c10 targets (libtorch 2.8.0 packaging bug)
foreach(_target torch c10 torch_cpu)
    if(TARGET ${_target})
        get_target_property(_libs ${_target} INTERFACE_LINK_LIBRARIES)
        if(_libs)
            list(REMOVE_ITEM _libs "FALSE")
            set_target_properties(${_target} PROPERTIES INTERFACE_LINK_LIBRARIES "${_libs}")
        endif()
    endif()
endforeach()

# Add executable
add_executable(yapt ${SOURCES}
        src/main.cpp
        src/random.cpp
        src/stb_image.cpp
        include/Vec3.h
        include/ray.h
        include/hittable.h
        include/sphere.h
        src/Vec3.cpp
        src/ray.cpp
        include/hittable_list.h
        src/hittable_list.cpp
        include/yapt.h
        include/constants.h
        src/color.cpp
        include/interval.h
        src/interval.cpp
        include/camera.h
        src/camera.cpp
        include/utils.h
        include/material.h
        src/material.cpp
        include/aabb.h
        include/bvh.h
        include/texture.h
        include/external/stb_image.h
        include/rtw_stb_image.h
        src/aabb.cpp
        include/perlin.h
        include/quad.h
        include/constant_medium.h
        include/onb.h
        include/pdf.h
        include/image_exporter.h
        src/image_exporter.cpp
        include/image_data.h
        include/sampler.h
        include/triangle.h
        include/importer.h
        include/aggregators.h
        src/aggregators.cpp
        include/sceneloader.h
        include/path.h
        src/sceneloader.cpp
        src/path.cpp
        include/parser.h
        include/scene.h
        include/sampling_strategy.h
        src/sampling_strategy.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(yapt
        ${PNG_LIBRARIES}
        Threads::Threads
        assimp
        CGAL::CGAL
        CGAL::CGAL_Core
        OpenEXR::OpenEXR
        ${TORCH_LIBRARIES}
)


add_custom_target(clean-all
        COMMAND ${CMAKE_BUILD_TOOL} clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory CMakeFiles
        COMMAND ${CMAKE_COMMAND} -E remove CMakeCache.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})


find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)

add_executable(qtvor ${SOURCES}
        src/qtvor.cpp
        src/random.cpp
        src/stb_image.cpp
        include/Vec3.h
        include/ray.h
        include/hittable.h
        include/sphere.h
        src/Vec3.cpp
        src/ray.cpp
        include/hittable_list.h
        src/hittable_list.cpp
        include/yapt.h
        include/constants.h
        src/color.cpp
        include/interval.h
        src/interval.cpp
        include/camera.h
        src/camera.cpp
        include/utils.h
        include/material.h
        src/material.cpp
        include/aabb.h
        include/bvh.h
        include/texture.h
        include/external/stb_image.h
        include/rtw_stb_image.h
        src/aabb.cpp
        include/perlin.h
        include/quad.h
        include/constant_medium.h
        include/onb.h
        include/pdf.h
        include/image_exporter.h
        src/image_exporter.cpp
        include/image_data.h
        include/sampler.h
        include/triangle.h
        include/importer.h
        include/aggregators.h
        src/aggregators.cpp
        include/sceneloader.h
        include/path.h
        src/sceneloader.cpp
        src/path.cpp
        src/qtvor.cpp
        include/parser.h
        include/scene.h
        include/qtvor/zoomablegraphicsview.h
        include/qtvor/voronoicellitem.h
        include/qtvor/zoomableimageview.h
        include/qtvor/utils.h
        include/functions.h
        include/sampling_strategy.h
        src/sampling_strategy.cpp
)

target_link_libraries(qtvor
        CGAL::CGAL
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        ${PNG_LIBRARIES}
        Threads::Threads
        assimp
        CGAL::CGAL
        CGAL::CGAL_Core
        OpenEXR::OpenEXR
        ${TORCH_LIBRARIES}
)

add_executable(eval ${SOURCES}
        src/eval.cpp
        src/random.cpp
        src/stb_image.cpp
        include/Vec3.h
        include/ray.h
        include/hittable.h
        include/sphere.h
        src/Vec3.cpp
        src/ray.cpp
        include/hittable_list.h
        src/hittable_list.cpp
        include/yapt.h
        include/constants.h
        src/color.cpp
        include/interval.h
        src/interval.cpp
        include/camera.h
        src/camera.cpp
        include/utils.h
        include/material.h
        src/material.cpp
        include/aabb.h
        include/bvh.h
        include/texture.h
        include/external/stb_image.h
        include/rtw_stb_image.h
        src/aabb.cpp
        include/perlin.h
        include/quad.h
        include/constant_medium.h
        include/onb.h
        include/pdf.h
        include/image_exporter.h
        src/image_exporter.cpp
        include/image_data.h
        include/sampler.h
        include/triangle.h
        include/importer.h
        include/aggregators.h
        src/aggregators.cpp
        include/sceneloader.h
        include/path.h
        src/sceneloader.cpp
        src/path.cpp
        include/parser.h
        include/scene.h
        include/functions.h
        src/eval.cpp
        include/sampling_strategy.h
        src/sampling_strategy.cpp
)

target_link_libraries(eval
        CGAL::CGAL
        ${PNG_LIBRARIES}
        Threads::Threads
        assimp
        CGAL::CGAL
        CGAL::CGAL_Core
        OpenEXR::OpenEXR
        ${TORCH_LIBRARIES}
)

add_executable(test_torch
        src/test_torch.cpp
)

target_link_libraries(test_torch
        ${TORCH_LIBRARIES}
)
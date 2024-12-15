cmake_minimum_required(VERSION 3.10)
project(yapt)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# Add include directory
include_directories(include)

# Add source files
#file(GLOB SOURCES "src/*.cpp")
#file(GLOB HEADERS "include/*.h")

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


# Add executable
add_executable(yapt ${SOURCES}
        src/main.cpp
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
        src/demo.cpp
        include/demo.h
        include/importer.h
        include/aggregators.h
        include/sceneloader.h
        include/path.h
        src/sceneloader.cpp
        src/path.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(yapt
        ${PNG_LIBRARIES}
        Threads::Threads
        assimp
        CGAL::CGAL
        CGAL::CGAL_Core
        OpenEXR::OpenEXR
)


add_custom_target(clean-all
        COMMAND ${CMAKE_BUILD_TOOL} clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory CMakeFiles
        COMMAND ${CMAKE_COMMAND} -E remove CMakeCache.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})


# Tests configuration
# Add include directory for tests
include_directories(tests)

# Add test sources
file(GLOB TEST_SOURCES "tests/*.cpp")

# Add executable for tests
add_executable(tests ${TEST_SOURCES}
        tests/vec3test.cpp
        src/stb_image.cpp
        include/color.h
        include/hittable.h
        include/sphere.h
        include/ray.h
        tests/raytest.cpp
        tests/spheretest.cpp
        src/Vec3.cpp
        src/ray.cpp
        include/hittable_list.h
        src/hittable_list.cpp
        tests/hittabletest.cpp
        include/yapt.h
        include/constants.h
        src/color.cpp
        include/interval.h
        include/interval.h
        src/interval.cpp
        tests/intervaltest.cpp
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
        tests/triangletest.cpp
        include/aggregators.h
        tests/samplertest.cpp
        include/sceneloader.h
        src/sceneloader.cpp
        src/path.cpp
)

# Link external libraries to tests executable if needed
target_link_libraries(tests
        ${PNG_LIBRARIES}
        CGAL::CGAL
        CGAL::CGAL_Core
        Threads::Threads
)

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME doctest COMMAND tests)

find_package(Qt6 COMPONENTS Core Widgets Gui REQUIRED)

add_executable(qtvor ${SOURCES}
        src/qtvor.cpp
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
        src/demo.cpp
        include/demo.h
        include/importer.h
        include/aggregators.h
        include/sceneloader.h
        include/path.h
        src/sceneloader.cpp
        src/path.cpp
        src/qtvor.cpp
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
)
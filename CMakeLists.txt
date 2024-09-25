cmake_minimum_required(VERSION 3.10)

project(yapt)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

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
)

find_package(Threads REQUIRED)
target_link_libraries(yapt ${PNG_LIBRARIES} Threads::Threads assimp CGAL::CGAL CGAL::CGAL_Core)


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
)

# Link external libraries to tests executable if needed
target_link_libraries(tests ${PNG_LIBRARIES} CGAL::CGAL CGAL::CGAL_Core Threads::Threads)

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME doctest COMMAND tests)
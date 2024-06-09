cmake_minimum_required(VERSION 3.10)

project(yapt)

set(CMAKE_CXX_STANDARD 17)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# Add include directory
include_directories(include)

# Add source files
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

#external dependencies
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})

# Add executable
add_executable(yapt ${SOURCES}
        src/main.cpp
        include/vec3.h
        include/ray.h
        include/hittable.h
        include/sphere.h
        src/vec3.cpp
        src/ray.cpp
        include/hittable_list.h
        src/hittable_list.cpp
        include/yapt.h
        include/constants.h
        src/color.cpp
)

target_link_libraries(yapt ${PNG_LIBRARIES})

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
        include/color.h
        include/hittable.h
        include/sphere.h
        include/ray.h
        tests/raytest.cpp
        tests/spheretest.cpp
        src/vec3.cpp
        src/ray.cpp
        include/hittable_list.h
        src/hittable_list.cpp
        tests/hittabletest.cpp
        include/yapt.h
        include/constants.h
        src/color.cpp
)

# Link external libraries to tests executable if needed
target_link_libraries(tests ${PNG_LIBRARIES})

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME doctest COMMAND tests)
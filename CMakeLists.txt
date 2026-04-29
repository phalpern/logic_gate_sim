cmake_minimum_required(VERSION 3.20)

project(gatesim
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# For Windows; harmless elsewhere
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include(GoogleTest)

file(GLOB_RECURSE GATESIM_ALL_SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

set(GATESIM_MAIN_SOURCE
    ${PROJECT_SOURCE_DIR}/src/gatesim_main.cpp
)

list(REMOVE_ITEM GATESIM_ALL_SOURCES ${GATESIM_MAIN_SOURCE})

file(GLOB_RECURSE GATESIM_TEST_SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/test/*.t.cpp
)

add_library(gatesim_lib
    ${GATESIM_ALL_SOURCES}
)

target_include_directories(gatesim_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(gatesim_lib
    PRIVATE -Wall -Wextra -Wpedantic
)

add_executable(gatesim
    ${GATESIM_MAIN_SOURCE}
)

add_custom_target(run
    COMMAND gatesim
    DEPENDS gatesim
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_link_libraries(gatesim
    PRIVATE gatesim_lib
)

target_include_directories(gatesim
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(gatesim
    PRIVATE -Wall -Wextra -Wpedantic
)

enable_testing()

foreach(test_source ${GATESIM_TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)

    add_executable(${test_name}
        ${test_source}
    )

    target_link_libraries(${test_name}
        PRIVATE gatesim_lib GTest::gtest_main
    )

    target_compile_options(${test_name}
        PRIVATE -Wall -Wextra -Wpedantic
    )

    gtest_discover_tests(${test_name})

    # add_test(
    #     NAME ${test_name}
    #     COMMAND ${test_name}
    # )
endforeach()

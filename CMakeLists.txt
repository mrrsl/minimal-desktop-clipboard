cmake_minimum_required(VERSION 3.22)

project(ClipLib VERSION 1.0
                DESCRIPTION "Clipboard Library"
                LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# Adding GTest
if (WIN32)
    set(gmake_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)


add_executable(clipX11demo src/clipboardX11-demo.cc)
add_executable(testmain tests/test.cpp)

add_library(cliplib STATIC src/clipboard.cc)

if (UNIX)
    target_link_libraries(clipX11demo -lX11)
    target_link_libraries(clipX11demo -lpthread)
    
    target_link_libraries(cliplib -lX11)
    target_link_libraries(cliplib -lpthread)
endif()

target_include_directories(testmain PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(cliplib PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(testmain GTest::gtest_main)
target_link_libraries(testmain cliplib)

include(GoogleTest)
gtest_discover_tests(testmain)

target_include_directories(clipX11demo PRIVATE src)
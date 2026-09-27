cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT ANDROID)
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
        message(FATAL_ERROR "Not supported iOS")
    endif()
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(RusDash VERSION 1.0.4)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    third_party/game-objects-factory
)

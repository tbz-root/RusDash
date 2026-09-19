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

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.13.6
)
FetchContent_MakeAvailable(pybind11)

if(ANDROID)
    message(STATUS "ScriptTrigger: Android Python embed")

    if(NOT DEFINED ANDROID_ABI)
        set(ANDROID_ABI "arm64-v8a")
    endif()

    set(PY_ANDROID_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/python-android/${ANDROID_ABI}")
    set(PY_ANDROID_INCLUDE "${PY_ANDROID_ROOT}/include")

    set(PY_ANDROID_LIB "")
    foreach(_cand
        "${PY_ANDROID_ROOT}/lib/libpython3.12.so"
        "${PY_ANDROID_ROOT}/lib/libpython3.11.so"
        "${PY_ANDROID_ROOT}/lib/libpython3.13.so"
        "${PY_ANDROID_ROOT}/lib/libpython3.14.so"
    )
        if(EXISTS "${_cand}")
            set(PY_ANDROID_LIB "${_cand}")
            break()
        endif()
    endforeach()
    if(NOT PY_ANDROID_LIB)
        file(GLOB _py_libs "${PY_ANDROID_ROOT}/lib/libpython3*.so")
        if(_py_libs)
            list(GET _py_libs 0 PY_ANDROID_LIB)
        endif()
    endif()

    if(NOT PY_ANDROID_LIB)
        message(WARNING
            "Android libpython not found under:\n  ${PY_ANDROID_ROOT}/lib/\n"
            "Run scripts/build_python_android.sh (Linux/WSL).\n"
            "See third_party/python-android/README.md."
        )
        set(PY_ANDROID_LIB "${PY_ANDROID_ROOT}/lib/libpython3.11.so")
    else()
        message(STATUS "Android libpython: ${PY_ANDROID_LIB}")
    endif()

    target_include_directories(${PROJECT_NAME} PRIVATE
        "${PY_ANDROID_INCLUDE}"
    )

    target_link_libraries(${PROJECT_NAME}
        pybind11::embed
        "${PY_ANDROID_LIB}"
        log
    )

    get_filename_component(_py_libname "${PY_ANDROID_LIB}" NAME)
    string(REGEX MATCH "libpython3\\.([0-9]+)" _py_mm "${_py_libname}")
    set(_py_minor "${CMAKE_MATCH_1}")
    if(NOT _py_minor)
        set(_py_minor "12")
    endif()

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SCRIPT_TRIGGER_ANDROID_PYTHON=1
        SCRIPT_TRIGGER_PYTHON_VERSION_MAJOR=3
        SCRIPT_TRIGGER_PYTHON_VERSION_MINOR=${_py_minor}
    )
else()
    message(STATUS "ScriptTrigger: Desktop Python embed (Windows-oriented)")

    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS "Python3: ${Python3_VERSION}")
    message(STATUS "Python3 include: ${Python3_INCLUDE_DIRS}")
    message(STATUS "Python3 libs: ${Python3_LIBRARIES}")

    target_link_libraries(${PROJECT_NAME}
        Python3::Python
        pybind11::embed
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        SCRIPT_TRIGGER_DESKTOP_PYTHON=1
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    third_party/game-objects-factory
)
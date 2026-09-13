cmake_minimum_required(VERSION 4.0)

project(CppTemplate
    VERSION 0.0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
include(CMakeDependentOption)

cmake_dependent_option(${PROJECT_NAME}_BUILD_TESTS
    "Enable ${PROJECT_NAME} project tests targets" ON
    "BUILD_TESTING" OFF
)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

if (${PROJECT_NAME}_BUILD_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
endif()

add_subdirectory(src)

file(GLOB_RECURSE TEMPLATE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB_RECURSE TEMPLATE_IMPLEMENT_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE TEMPLATE_TEST_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.test.cpp")

list(FILTER TEMPLATE_IMPLEMENT_SOURCE EXCLUDE REGEX "\\.test\\.cpp$")

add_library(template ${TEMPLATE_IMPLEMENT_SOURCE})
add_library(template::tmp ALIAS template)

target_compile_features(template PUBLIC cxx_std_23)
target_include_directories(template PUBLIC ${TEMPLATE_INCLUDE_DIRS})
target_link_libraries(template
    PUBLIC
        Microsoft.GSL::GSL
        fmt::fmt
        spdlog::spdlog
)

if (${PROJECT_NAME}_BUILD_TESTS AND TEMPLATE_TEST_SOURCE)
    include(Catch)

    add_executable(template.test ${TEMPLATE_TEST_SOURCE})

    target_link_libraries(template.test
        PRIVATE
            Catch2::Catch2WithMain
    )

    catch_discover_tests(template.test)
endif()
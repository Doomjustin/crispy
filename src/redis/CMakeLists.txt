file(GLOB_RECURSE CRISPY_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB_RECURSE CRISPY_IMPLEMENT_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE CRISPY_TEST_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.test.cpp")

list(FILTER CRISPY_IMPLEMENT_SOURCE EXCLUDE REGEX "\\.test\\.cpp$")

add_library(crispy_redis ${CRISPY_IMPLEMENT_SOURCE})
add_library(crispy::redis ALIAS crispy_redis)

target_compile_features(crispy_redis PUBLIC cxx_std_23)

target_compile_definitions(crispy_redis
    PUBLIC
        BOOST_ASIO_HAS_IO_URING
        BOOST_ASIO_DISABLE_EPOLL
)

target_include_directories(crispy_redis PUBLIC ${CRISPY_INCLUDE_DIRS})

target_link_libraries(crispy_redis
    PUBLIC
        Microsoft.GSL::GSL
        fmt::fmt
        spdlog::spdlog
        Boost::asio
        PkgConfig::liburing
        PkgConfig::jemalloc
)

if (${PROJECT_NAME}_BUILD_TESTS AND CRISPY_TEST_SOURCE)
    include(Catch)

    add_executable(crispy.redis.test ${CRISPY_TEST_SOURCE})

    target_link_libraries(crispy.redis.test
        PRIVATE
            Catch2::Catch2WithMain
            crispy::redis
    )

    catch_discover_tests(crispy.redis.test)
endif()
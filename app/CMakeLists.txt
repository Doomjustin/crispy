add_executable(redis_server redis_server.cpp)
target_link_libraries(redis_server PUBLIC crispy::redis)
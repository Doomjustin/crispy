#include <cstdlib>

#include <redis/server.h>
#include <spdlog/spdlog.h>

int main()
{
    SPDLOG_INFO("Starting Redis server...");

    crispy::redis::Server server{ 12345 };
    server.run();

    SPDLOG_INFO("Redis server stopped.");
    return EXIT_SUCCESS;
}
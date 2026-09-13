#include <cstdlib>

#include <redis/server.h>
#include <spdlog/spdlog.h>

int main()
{
    crispy::redis::Server server{ 12345 }; // Example port number
    server.run();

    return EXIT_SUCCESS;
}
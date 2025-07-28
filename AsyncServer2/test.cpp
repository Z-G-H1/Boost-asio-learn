#include "CServer.h"

int main(int argc, char const *argv[])
{
    boost::asio::io_context ioc;
    CServer server(ioc, 10086);
    // server.
    ioc.run();
    return 0;
}

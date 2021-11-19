#include <iostream>
#include <future>
//-----------------------------//
#include "dSocket.h"
//-----------------------------//

//-----------------------------//
int main() {
    dSocket Server(true);
    Server.init(dSocketProtocol::TCP);
    Server.finalize(dSocketType::SERVER, 20000);

    std::future <void> ServerThread = std::async(std::launch::async, [&Server] {
        auto Socket = Server.acceptConnection();
        std::cout << Socket << std::endl;
    });

    dSocket Client(true);
    Client.init(dSocketProtocol::TCP);
    Client.finalize(dSocketType::CLIENT, 20000, "127.0.0.1");

    std::future <void> ClientThread = std::async(std::launch::async, [&Client] {
        auto Result = Client.connectToServer(1000);
    });

    return 0;
}

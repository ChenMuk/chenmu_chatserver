#include "chatserver.h"
#include "chatservice.h"
#include <signal.h>
#include <iostream>
#include <cstdlib>     // for atoi
#include <string>      // for std::string

// 服务器异常退出时的处理
void resetHandler(int) {
    ChatService::getInstance()->reset();
    exit(0);
}

int main(int argc, char* argv[]) {
    // 检查参数数量
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>" << std::endl;
        return 1;
    }

    // 读取参数
    std::string ip = argv[1];
    int port = std::atoi(argv[2]);

    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress listenAddr(ip, port);
    ChatServer server(&loop, listenAddr, "chenme");
    server.start();
    loop.loop();

    return 0;
}
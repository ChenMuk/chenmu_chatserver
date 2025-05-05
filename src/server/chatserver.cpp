#include "chatserver.h"
#include "json.hpp"
#include <chatservice.h>
#include <functional>
#include <string>
#include <iostream>
#include <muduo/base/Logging.h>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(5);
}

void ChatServer::start()
{
    _server.start();
}

// 处理连接断开的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected()) {
        LOG_INFO << conn->peerAddress().toIpPort() << " connected";
    } else {
        LOG_INFO << conn->peerAddress().toIpPort() << " disconnected";
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 处理读写的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf=buffer->retrieveAllAsString();
    //反序列化
    json js=json::parse(buf);
    auto msghandler = ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    msghandler(conn,js,time);
}

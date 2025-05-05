#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include "json.hpp"
#include "usermodel.h"
#include "offlinemsgmodel.h"
#include "friendmodel.h"
#include "groupmodel.h"
#include "redis.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;

//定义回调
using MsgHandler = std::function<void (const TcpConnectionPtr &conn, json &js,Timestamp time)>;

class ChatService{
public:
    //单例模式
    static ChatService* getInstance();
    //登录
    void login(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //注册
    void regis(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //一对一聊天
    void oneChat(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //发送群消息
    void groupChat(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //注销
    void loginOut(const TcpConnectionPtr &conn, json &js,Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //处理server ctrl+c 退出
    void reset();
    //redis的subscribe message的回调函数
    void handleRedisSubscribeMessage(int,string);
private:
    ChatService();
    //存储业务id对应的处理器
    unordered_map<int, MsgHandler> _msgHandlerMap;
    //存储在线用户的连接
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    //数据操作类对象
    UserModel _usermodel;
    OfflineMsgModel _offlinemsgmodel;
    FriendModel _friendmodel;
    GroupModel _groupmodel;
    //互斥锁 保证线程安全
    mutex _connMutex;
    //redis对象
    Redis _redis;
};


#endif
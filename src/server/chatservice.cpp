#include "chatservice.h"
#include "public.h"
#include <iostream>
using namespace std;

ChatService *ChatService::getInstance()
{
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 注册回调
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REGIS_MSG, std::bind(&ChatService::regis, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGIN_OUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 找不到的话 就返回一个空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            cout << "msgid: " << msgid << " not find!" << endl;
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 登录 id password
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];

    User user = _usermodel.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        if (user.getState() == "online")
        {
            // 已经登录了，无需再登
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["error"] = 1;
            response["errmsg"] = "该账号已经登录";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功,记录连接信息
            {
                lock_guard<mutex> lg(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 向redis 订阅 channel
            _redis.subscribe(id);

            // 更新状态
            user.setState("online");
            _usermodel.updateState(user);

            // 响应
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["error"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询是否有离线消息
            vector<string> msgs = _offlinemsgmodel.query(id);
            if (!msgs.empty())
            {
                response["offlinemsg"] = msgs;
                _offlinemsgmodel.remove(id);
            }

            // 查询好友信息
            vector<User> users = _friendmodel.query(id);

            if (!users.empty())
            {
                vector<string> vec;
                for (User &u : users)
                {
                    json js1;
                    js1["id"] = u.getId();
                    js1["name"] = u.getName();
                    js1["state"] = u.getState();
                    vec.push_back(js1.dump());
                }
                response["friends"] = vec;
            }

            // 查询群组信息
            vector<Group> groups = _groupmodel.queryGroups(id);
            if (!groups.empty())
            {
                vector<string> vec;
                for (Group &g : groups)
                {
                    json js1;
                    js1["id"] = g.getId();
                    js1["groupname"] = g.getName();
                    js1["groupdesc"] = g.getDesc();

                    vector<GroupUser> groupusers = g.getUsers();
                    for (auto &p : groupusers)
                    {
                        json js2;
                        js2["id"] = p.getId();
                        js2["name"] = p.getName();
                        js2["state"] = p.getState();
                        js2["role"] = p.getRole();
                        js1["users"].push_back(js2.dump());
                    }
                    vec.push_back(js1.dump());
                }
                response["groups"] = vec;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["error"] = 2;
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}
// 注册 name password
void ChatService::regis(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool res = _usermodel.insert(user);
    if (res)
    {
        // 成功
        json response;
        response["msgid"] = REGIS_MSG_ACK;
        response["error"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 失败
        json response;
        response["msgid"] = REGIS_MSG_ACK;
        response["error"] = 1;
        conn->send(response.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lg(_connMutex);
        // 删除该客户端连接
        for (auto &p : _userConnMap)
        {
            if (p.second == conn)
            {
                user.setId(p.first);
                _userConnMap.erase(p.first);
                break;
            }
        }
    }

    // 向redis取消订阅
    _redis.unsubscribe(user.getId());

    // 更新用户状态为offline
    if (user.getId() != -1)
    {
        user.setState("offline");
        _usermodel.updateState(user);
    }
}

// 一对一聊天  msgid fromid fromname toid msg
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"];
    {
        lock_guard<mutex> lg(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // 存在该连接 直接转发
            it->second->send(js.dump());
            return;
        }
    }
    // 不存在改连接,查询是否在线，如果在线那么就在其他服务器中连接了，去redis中发布该消息
    User user = _usermodel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // 不在线 存储离线信息
    _offlinemsgmodel.insert(toid, js.dump());
}

// 处理server ctrl+c 退出
void ChatService::reset()
{
    _usermodel.resetStatus();
}

// 添加好友  msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"];
    int friendid = js["friendid"];
    _friendmodel.insert(userid, friendid);
}

// 创建群组  id groupname groupdesc
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];

    Group group(-1, groupname, groupdesc);
    if (_groupmodel.createGroup(group))
    {
        _groupmodel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组 id groupid
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupmodel.addGroup(userid, groupid, "normal");
}
// 发送群消息 id groupid msg
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> userids = _groupmodel.queryGroupUsers(userid, groupid);
    for (int id : userids)
    {
        lock_guard<mutex> lg(_connMutex);
        auto it = _userConnMap.find(id);
        // 在线，并且在当前连接中
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        // 可能在别的服务器的连接中
        else
        {
            User user = _usermodel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlinemsgmodel.insert(id, js.dump());
            }
        }
    }
}

// 注销
void ChatService::loginOut(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    ChatService::getInstance()->clientCloseException(conn);
}

// redis的subscribe message的回调函数
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lg(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }
    _offlinemsgmodel.insert(userid, msg);
}
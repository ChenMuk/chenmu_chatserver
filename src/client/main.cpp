#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.h"
#include "user.h"
#include "public.h"

// 读写进程之间的信号量
sem_t rwsem;

// 记录登录状态
bool is_login = false;

// 记录主聊天界面
bool is_MainChat = false;

// 记录当前登录的用户信息
User currentuser;

// 记录好友列表
vector<User> currentFriendLists;

// 记录群组列表
vector<Group> currentGroupLists;

void readTaskHandler(int clientfd);
void mainChat(int clientfd);
void doLoginResponse(json &responsejs);
void doRegisResponse(json &responsejs);
void showCurrentUserData();

// 主界面 main线程用于发送信息 子线程用于接收信息
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command is invalid, please input like this: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析命令
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socker  create error" << endl;
        exit(-1);
    }

    // 连接信息 ip port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // 连接
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "connect server error" << endl;
        exit(-1);
    }

    // 读写线程的信号量
    sem_init(&rwsem, 0, 0);

    // 启动接收的子线程
    thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main线程 用于接收用户输入,并发送数据给服务器
    while (1)
    {
        // 首页菜单 包括登录，注册，退出
        cout << "--------------------" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "--------------------" << endl;
        cout << "choice: ";

        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1: // login
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid: ";
            cin >> id;
            cin.get();
            cout << "password: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            is_login = false;
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error" << endl;
            }

            // 等待子线程接收完登录请求的response
            sem_wait(&rwsem);

            if (is_login)
            {
                // 显示主聊天界面
                is_MainChat = true;
                mainChat(clientfd);
            }
        }
        break;
        case 2: // register
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REGIS_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send register msg error" << endl;
            }
            sem_wait(&rwsem);
        }
        break;
        case 3: // quit
        {
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        }
        break;
        default:
            cerr << "invalid input" << endl;
            break;
        }
    }
}

// 子线程 接收线程
void readTaskHandler(int clientfd)
{
    while (1)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收数据 并序列化成json对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>() << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == GROUP_CHAT_MSG)
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (msgtype == LOGIN_MSG_ACK)
        {
            doLoginResponse(js); // 处理登录成功
            sem_post(&rwsem);
            continue;
        }
        if (msgtype == REGIS_MSG_ACK)
        {
            doRegisResponse(js);
            sem_post(&rwsem); // 处理注册成功
            continue;
        }
    }
}

// 处理登录成功
void doLoginResponse(json &responsejs)
{
    // 登录失败
    if (responsejs["error"].get<int>() != 0)
    {
        cerr << responsejs["errmsg"] << endl;
        is_login = false;
    }
    // 成功
    else
    {
        // 记录登录的用户信息
        currentuser.setId(responsejs["id"].get<int>());
        currentuser.setName(responsejs["name"]);

        // 好友列表记录
        if (responsejs.contains("friends"))
        {
            currentFriendLists.clear();
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"]);
                user.setName(js["name"]);
                user.setState(js["state"]);
                currentFriendLists.push_back(user);
            }
        }

        // 记录群组列表
        if (responsejs.contains("groups"))
        {
            currentGroupLists.clear();
            vector<string> vec = responsejs["groups"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                Group g;
                g.setId(js["id"]);
                g.setName(js["groupname"]);
                g.setDesc(js["groupdesc"]);

                vector<string> vec2 = js["users"];
                for (auto &p : vec2)
                {
                    GroupUser gu;
                    json js1 = json::parse(p);
                    gu.setId(js1["id"].get<int>());
                    gu.setName(js1["name"]);
                    gu.setState(js1["state"]);
                    gu.setRole(js1["role"]);
                    g.getUsers().push_back(gu);
                }
                currentGroupLists.push_back(g);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示离线信息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            { // time+[id]+name+" said: "+msg
                json js = json::parse(str);
                if (js["msgid"].get<int>() == ONE_CHAT_MSG)
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }

        is_login = true;
    }
}

// 处理注册成功
void doRegisResponse(json &responsejs)
{
    if (responsejs["error"].get<int>() != 0)
    {
        cerr << "register failed! the username already exists" << endl;
    }
    else
    {
        cout << "register success, your id is: " << responsejs["id"].get<int>() << endl;
    }
}

void showCurrentUserData()
{
    cout << "----------------login user---------------" << endl;
    cout << "current id: " << currentuser.getId() << " name: " << currentuser.getName() << endl;
    cout << "----------------friend list--------------" << endl;
    if (!currentFriendLists.empty())
    {
        for (auto &user : currentFriendLists)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------group list--------------" << endl;
    if (!currentGroupLists.empty())
    {
        for (auto &group : currentGroupLists)
        {
            cout << group.getId() << " " << group.getName() << " 描述：" << group.getDesc() << endl;
            cout<<"成员："<<endl;
            for (auto &guser : group.getUsers())
            {
                cout << guser.getId() << " " << guser.getName() << " " << guser.getState() << " " << guser.getRole() << endl;
            }
        }
    }
    cout << "----------------------------------------" << endl;
}

// 客户端支持的命令
unordered_map<string, string> commandMap = {
    {"help","显示支持的所有命令"},
    {"chat","一对一聊天，格式为chat:friendid:message"},
    {"addfriend","添加好友，格式为addfriend:friendid"},
    {"creategroup","创建群组，格式为creategroup:groupname:groupdesc"},
    {"addgroup","加入群组,格式为addgroup:groupid"},
    {"groupchat","发送群信息，格式为groupchat:groupid:message"},
    {"loginout","注销，格式为loginout"}
};


void help(int clientfd,string str);  //命令回调函数
void chat(int clientfd,string str);  //chat回调函数
void addfriend(int clientfd,string str); //addfriend回调函数
void creategroup(int clientfd,string str);  //creategroup回调函数
void addgroup(int clientfd,string str);  //addgroup回调函数
void groupchat(int clientfd,string str);  //groupchat回调函数
void loginout(int clientfd,string str);  //loginout回调函数

//命令回调函数注册
unordered_map<string,function<void (int,string)>> commandHandlerMap={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}
};

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// 主聊天界面
void mainChat(int clientfd)
{
    help(0,"");

    char buffer[1024]={0};
    while(is_MainChat){
        cin.getline(buffer,1024);
        string commandbuffer(buffer);
        string command;
        int idx=commandbuffer.find(":");
        if(idx==-1){
            command=commandbuffer;
        }
        else{
            command=commandbuffer.substr(0,idx);
        }
        auto it=commandHandlerMap.find(command);
        if(it==commandHandlerMap.end()){
            cerr<<"invalid command"<<endl;
            continue;
        }
        it->second(clientfd,commandbuffer.substr(idx+1));
    }
}

void help(int clientfd,string str){
    cout<<"你可以使用如下命令: "<<endl;
    for(auto& p:commandMap){
        cout<<p.first<<" "<<p.second<<endl;
    }
    cout<<endl;
}
void chat(int clientfd,string str){
    int idx=str.find(":");
    int friendid=stoi(str.substr(0,idx));
    string msg=str.substr(idx+1);
    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=currentuser.getId();
    js["name"]=currentuser.getName();
    js["toid"]=friendid;
    js["msg"]=msg;
    js["time"]=getCurrentTime();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send message error"<<endl;
    }
}
void addfriend(int clientfd,string str){
    int friendid=stoi(str);
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=currentuser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"add friend error"<<endl;
    }
}
void creategroup(int clientfd,string str){
    int idx=str.find(":");
    string groupname=str.substr(0,idx);
    string groupdesc=str.substr(idx+1);
    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=currentuser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"create group error"<<endl;
    }
}
void addgroup(int clientfd,string str){
    int groupid=stoi(str);

    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=currentuser.getId();
    js["groupid"]=groupid;
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"add group error"<<endl;
    }
}
void groupchat(int clientfd,string str){
    int idx=str.find(":");
    int groupid=stoi(str.substr(0,idx));
    string message=str.substr(idx+1);

    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=currentuser.getId();
    js["name"]=currentuser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send group message error"<<endl;
    }
}
void loginout(int clientfd,string str){
    json js;
    js["msgid"]=LOGIN_OUT_MSG;
    js["id"]=currentuser.getId();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"log out error"<<endl;
    }
    else{
        is_MainChat=false;
    }
}
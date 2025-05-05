#ifndef PUBLIC_H
#define PUBLIC_H

/*
client 和 server的共同部分
*/
enum EnMsgType{
    LOGIN_MSG = 1,  //登录消息
    LOGIN_MSG_ACK,  //登录响应
    REGIS_MSG,   //注册消息
    REGIS_MSG_ACK,  //注册响应
    ONE_CHAT_MSG,    //一对一聊天消息
    ADD_FRIEND_MSG,     //添加好友
    CREATE_GROUP_MSG,   //创建群组
    ADD_GROUP_MSG,      //加入群组
    GROUP_CHAT_MSG,     //群聊天
    LOGIN_OUT_MSG       //注销
};

#endif
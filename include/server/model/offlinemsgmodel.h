#ifndef OFFLINEMSGMODEL_H
#define OFFLINEMSGMODEL_H
#include <string>
#include <vector>
using namespace std;

class OfflineMsgModel{
public:
    //存储离线消息
    void insert(int userid,string message);

    //删除离线消息
    void remove(int userid);

    //查询用户的离线消息
    vector<string> query(int userid);
};


#endif
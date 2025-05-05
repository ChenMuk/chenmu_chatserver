#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.h"
#include <vector>
using namespace std;

//好友列表的数据操作
class FriendModel{
public:
    //添加好友
    void insert(int userid,int friendid);

    //获取好友列表
    vector<User> query(int userid); 
};


#endif
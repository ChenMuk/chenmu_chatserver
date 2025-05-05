#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.h"
#include <string>
#include <vector>
using namespace std;

class GroupModel{
public:
    //创建群组
    bool createGroup(Group &group);
    //用户加入群组
    void addGroup(int userid,int groupid,string role);
    //查询用户所在的所有组
    vector<Group> queryGroups(int userid);
    //查询用户的某个群组 除他以外其他用户的id
    vector<int> queryGroupUsers(int userid,int groupid);
};

#endif
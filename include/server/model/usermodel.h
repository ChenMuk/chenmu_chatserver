#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.h"

class UserModel{
public:
    //新增用户
    bool insert(User &user);

    //根据id查询用户
    User query(int id);

    //更新用户状态信息
    bool updateState(User user);

    //重置状态信息
    void resetStatus();
};

#endif
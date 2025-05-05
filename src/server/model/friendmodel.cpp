#include "friendmodel.h"
#include "db.h"

//添加好友
void FriendModel::insert(int userid,int friendid){
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d,%d)",userid,friendid);
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//获取好友列表
vector<User> FriendModel::query(int userid){
    char sql[1024] = {0};
    sprintf(sql, "select user.id,user.name,user.state from user inner join friend on user.id=friend.friendid where friend.userid=%d",userid);
    MySQL mysql;
    vector<User> users;
    if(mysql.connect()){
        MYSQL_RES* res= mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                users.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return users;
}
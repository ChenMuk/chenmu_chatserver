#include "groupmodel.h"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
// 用户加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(groupid,userid,grouprole) values(%d,%d,'%s')",
            groupid, userid, role.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
// 查询用户所在的所有组
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select allgroup.id,allgroup.groupname,allgroup.groupdesc from allgroup inner join groupuser on allgroup.id=groupuser.groupid where groupuser.userid=%d", userid);
    MySQL mysql;
    vector<Group> groups;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groups.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    for (Group &grp : groups)
    {
        char sql[1024] = {0};
        sprintf(sql, "select user.id,user.name,user.state,groupuser.grouprole from user inner join groupuser on user.id=groupuser.userid where groupuser.groupid=%d", grp.getId());
        MySQL mysql;
        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser gu;
                    gu.setId(atoi(row[0]));
                    gu.setName(row[1]);
                    gu.setState(row[2]);
                    gu.setRole(row[3]);
                    grp.getUsers().push_back(gu);
                }
                mysql_free_result(res);
            }
        }
    }
    return groups;
}
// 查询用户的某个群组 除他以外其他用户的id
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d",groupid,userid);
    MySQL mysql;
    vector<int> userids;
    if(mysql.connect()){
        MYSQL_RES* res= mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                userids.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return userids;
}
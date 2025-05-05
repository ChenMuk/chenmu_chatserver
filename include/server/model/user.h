#ifndef USER_H
#define USER_H
#include <string>
using namespace std;

class User
{
public:
    User(int id = -1, string name = "", string password = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
    }
    int getId()
    {
        return this->id;
    }
    void setId(int id)
    {
        this->id = id;
    }
    string getName()
    {
        return this->name;
    }
    void setName(string name)
    {
        this->name = name;
    }
    string getPassword()
    {
        return this->password;
    }
    void setPassword(string password)
    {
        this->password = password;
    }
    string getState()
    {
        return this->state;
    }
    void setState(string state)
    {
        this->state = state;
    }

protected:
    int id;
    string name;
    string password;
    string state;
};

#endif
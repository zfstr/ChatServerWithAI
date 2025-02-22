#ifndef USER_H
#define USER_H
#include <string>
using namespace std;
// User表的ORM类
class User
{
public:
    User(int id = -1, string name = "", string password = "", string state = "offline", bool response_with_ai = 0)
    {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
        this->response_with_ai = response_with_ai;
    }
    void setId(int id) {this->id = id;}
    void setName(string name){this->name=name;}
    void setPassword(string password){this->password = password;}
    void setState(string state){this->state = state;}
    void setResponse_with_ai(int response_with_ai){this->response_with_ai = response_with_ai;}


    int getId() {return this->id;}
    string getName(){return this->name;}
    string getPassword(){return this->password;}
    string getState(){return this->state;}
    int getResponse_with_ai(){return this->response_with_ai;}

private:
    int id;
    string name;
    string password;
    string state;
    int response_with_ai;
};

#endif
#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"
// User的数据操作类
class UserModel
{
public:
    bool insert(User &user);
    // 根据用户号码查询用户信息
    User query(int id);
    bool updateState(User user);

    bool updateResponse_with_ai(User user);

    void resetState();

};

#endif
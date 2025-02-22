#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <ctime>
// 获取单例对象的接口函数
using namespace muduo;
using namespace std;

string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
//
ChatService::ChatService()
{
    _msgHanderlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHanderlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHanderlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHanderlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    _msgHanderlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHanderlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHanderlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    _msgHanderlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});

    _msgHanderlerMap.insert({CHAT_AI_MSG, std::bind(&ChatService::chatWithAI, this, _1, _2, _3)});
    _msgHanderlerMap.insert({RESPONSE_WITH_AI_MSG, std::bind(&ChatService::responseWithAI, this, _1, _2, _3)});
     // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }

}
// 服务器异常，业务重置
void ChatService::reset()
{
    // 把online用户设置为offline
    _userModel.resetState();
}
// 获取消息对应的处理器
MsgHander ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHanderlerMap.find(msgid);
    if (it == _msgHanderlerMap.end())
    {

        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    else
    {
        return _msgHanderlerMap[msgid];
    }
}

// 处理登录业务和注册业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do login!";
    int id = js["id"].get<int>();
    // string name = js["name"];
    string password = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this id logining, please reg!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功
            // 记录连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id); 

            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json friendjs;
                    friendjs["id"] = user.getId();
                    friendjs["name"] = user.getName();
                    friendjs["state"] = user.getState();
                    vec2.push_back(friendjs.dump());
                    {
                        lock_guard<mutex> lock(_connMutex);
                        auto it = _userConnMap.find(user.getId());
                        json comjs;
                        comjs["msgid"] = COME_ONLINE_MSG;
                        comjs["id"] = id;
                        if(it != _userConnMap.end())
                        {
                            
                            it->second->send(comjs.dump());
                        }
                        // 查询toid是否在线 
                        User user_other_server = _userModel.query(user.getId());
                        cout << user_other_server.getState() << endl;
                        if (user_other_server.getState() == "online")
                        {
                            cout << comjs.dump() << endl;
                            _redis.publish(user_other_server.getId(), comjs.dump());
                        }
                        // 查询toid是否在线 
                        // User user = _userModel.query(user.getId());
                        // cout << user.getState() << endl;
                        // if (user.getState() == "online")
                        // {
                        //     cout << comjs.dump() << endl;
                        //     _redis.publish(user.getId(), comjs.dump());
                        // }

                    }
                }
                response["friends"] = vec2;
            }
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> vec3;
                for (Group & group : groupVec)
                {
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    vector<string> vec4;
                    for( GroupUser & user : group.getUsers())
                    {
                        json groupuserjs;
                        groupuserjs["id"] = user.getId();
                        groupuserjs["name"] = user.getName();
                        groupuserjs["state"] = user.getState();
                        groupuserjs["role"] = user.getRole();
                        vec4.push_back(groupuserjs.dump());
                    }
                    groupjs["users"] = vec4;
                    vec3.push_back(groupjs.dump());
                }
                response["groups"] = vec3;
            }
            conn->send(response.dump());
            // cout << response.dump() << endl;
        }
    }
    else
    {
        // 登录失败,
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "this password or id is error!";
        conn->send(response.dump());
    }
}
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);

    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    int userid = js["id"].get<int>();
    // 查询toid是否在线 
    User user = _userModel.query(toid);
    if (user.getResponse_with_ai() == 1)
    {   
        lock_guard<mutex> lock(_connMutex);
        auto userit = _userConnMap.find(userid);
        string str = js["msg"];
        string responsewithai = ai.chat(str);
        json ai_responsejs;
        ai_responsejs["msgid"] = ONE_CHAT_MSG;
        ai_responsejs["id"] = toid;
        ai_responsejs["name"] = user.getName();
        ai_responsejs["toid"] = userid;
        ai_responsejs["msg"] = responsewithai;
        ai_responsejs["time"] = getCurrentTime();
        userit->second->send(ai_responsejs.dump());
    }

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息  服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }
    
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    
    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}
// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex);

    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线 
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}


// 开启ai自动回复
void ChatService::responseWithAI(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    User user = _userModel.query(userid);
    cout << userid << "状态" << user.getResponse_with_ai() << endl;
    if(user.getResponse_with_ai() == 1 )
    {
        user.setResponse_with_ai(0);
        if(_userModel.updateResponse_with_ai(user))
        {
            {
                lock_guard<mutex> lock(_connMutex);
                auto it = _userConnMap.find(userid);  
                json AIjs;
                AIjs["msgid"] = RESPONSE_WITH_AI_ACK_MSG;
                AIjs["msg"] = "成功关闭ai自动回复功能";
                it->second->send(AIjs.dump());
            }
        }
    }
    else
    {
        user.setResponse_with_ai(1);
        if(_userModel.updateResponse_with_ai(user))
        {
            {
                lock_guard<mutex> lock(_connMutex);
                auto it = _userConnMap.find(userid);  
                json AIjs;
                AIjs["msgid"] = RESPONSE_WITH_AI_ACK_MSG;
                AIjs["msg"] = "成功开启ai自动回复功能";
                it->second->send(AIjs.dump());
            }
        }
    }
    
}


// 与AI聊天业务
void ChatService::chatWithAI(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);   
        string msg = js["msg"].get<string>();
        string sendmsg = ai.chat(msg);
        json AIjs;
        AIjs["msgid"] = CHAT_AI_MSG;
        AIjs["msg"] = sendmsg;
        it->second->send(AIjs.dump());
    }
}

void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }

    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

     // 更新用户的状态信息
    User user(userid,"","","offline");
    _userModel.updateState(user);
    
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        // 定义作用域
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表中删除连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId()); 

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
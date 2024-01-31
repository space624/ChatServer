#include "chatservice.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>
#include <vector>

using namespace muduo;
using namespace std;

ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handler回调操作
ChatService::ChatService() {
    msgHandlerMap_.insert({ LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3) });
    msgHandlerMap_.insert({ LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3) });
    msgHandlerMap_.insert({ REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3) });
    msgHandlerMap_.insert({ ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3) });
    msgHandlerMap_.insert({ ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3) });

    if (redis_.connect()) {
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage , this , _1 , _2));
    }
}

// 登陆业务
void ChatService::login(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = userModel_.query(id);
    if (user.getId() != id || user.getPwd() != pwd) {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "用户或密码错误";
        conn->send(response.dump());
        return;
    }

    if (user.getState() == "online") {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 2;
        response["errmsg"] = "改账号已经登录";
        conn->send(response.dump());
        return;
    }

    // 修改登录状态
    user.setState("online");
    userModel_.updateState(user);

    {
        // 记录用户连接信息
        lock_guard<mutex> lock(connMutex_);
        userConnMap_.insert({ id, conn });
    }
    //登陆成功后订阅channel(id)
    redis_.subscribe(id);

    json response;
    response["msgid"] = LOGIN_MSG_ACK;
    response["errno"] = 0;
    response["id"] = user.getId();
    response["name"] = user.getName();
    // 用户是否有离线消息
    vector<string> vec = offlineMsgModel_.query(id);
    if (!vec.empty()) {
        response["offlinemsg"] = vec;
        offlineMsgModel_.remove(id);
    }

    // 查询该用户的好友信息
    vector<User> userVec = friendModel_.query(id);
    if (!userVec.empty()) {
        vector<string> vec2;
        for (User& user : userVec) {
            json js;
            js["id"] = user.getId();
            js["name"] = user.getName();
            js["state"] = user.getState();
            vec2.push_back(js.dump());
        }
        response["friends"] = vec2;
    }

    //查询用户群组信息
    vector<Group> groupuserVec = groupModel_.queryGroups(id);
    if (!groupuserVec.empty()) {
        vector<string> groupV;
        for (Group& group : groupuserVec) {
            json grpjson;
            grpjson["id"] = group.getId();
            grpjson["groupname"] = group.getName();
            grpjson["groupdesc"] = group.getDesc();
            vector<string> userV;
            for (GroupUser& user : group.getUsers()) {
                json js;
                js["id"] = user.getId();
                js["name"] = user.getName();
                js["state"] = user.getState();
                js["role"] = user.getRole();
                userV.push_back(js.dump());
            }
            grpjson["users"] = userV;
            groupV.push_back(grpjson.dump());
        }
        response["groups"] = groupV;
    }
    conn->send(response.dump());
}

// 注册业务
void ChatService::reg(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    if (!userModel_.insert(user)) {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
        return;
    }
    json response;
    response["msgid"] = REG_MSG_ACK;
    response["errno"] = 0;
    response["id"] = user.getId();
    conn->send(response.dump());
}

MsgHandler ChatService::getHandler(int msgId) {
    auto it = msgHandlerMap_.find(msgId);
    if (it == msgHandlerMap_.end()) {
        return [=](const TcpConnectionPtr& conn , json& js , Timestamp) {
            LOG_ERROR << " msgID : " << msgId << " can not find handler! ";
        };
    } else {
        return msgHandlerMap_[msgId];
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn) {
    User user;
    {
        lock_guard<mutex> lock(connMutex_);
        for (auto it = userConnMap_.begin(); it != userConnMap_.end(); ++it) {
            if (it->second == conn) {
                // map表删除用户的连接信息
                user.setId(it->first);
                userConnMap_.erase(it);
                break;
            }
        }
    }

    //用户注销,redis中取消订阅
    redis_.unsubscribe(user.getId());

    if (user.getId() != -1) {
        // 更新用户的状态信息
        user.setState("offline");
        userModel_.updateState(user);
    }
}

void ChatService::loginout(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(userid);
        if (it != userConnMap_.end()) {
            userConnMap_.erase(it);
        }
    }

    //用户注销,redis中取消订阅
    redis_.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid , "" , "" , "offline");
    userModel_.updateState(user);
}

// 一对一聊天
void ChatService::oneChat(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(connMutex_);
        auto it = userConnMap_.find(toid);
        if (it != userConnMap_.end()) {
            // 在线
            // 转发消息
            it->second->send(js.dump());
            return;
        }
    }

    //toid是否在线
    User user = userModel_.query(toid);
    if (user.getState() == "online") {
        //消息发送至 redis toid通道上
        redis_.publish(toid , js.dump());
        return;
    }
    //存储离线消息
    offlineMsgModel_.insert(toid , js.dump());
}

void ChatService::reset() {
    // online状态的用户,设置为offline
    userModel_.resetState();
}

// 添加好友
void ChatService::addFriend(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userid , friendid);
}

void ChatService::createGroup(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组消息
    Group group(-1 , name , desc);
    if (groupModel_.createGroup(group)) {
        groupModel_.addGroup(userid , group.getId() , "creator");
    }
}

void ChatService::addGroup(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid , groupid , "normal");
}

void ChatService::groupChat(const TcpConnectionPtr& conn , json& js , Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = groupModel_.queryGroupUsers(userid , groupid);
    lock_guard<mutex> lock(connMutex_);

    for (int id : useridVec) {
        auto it = userConnMap_.find(id);
        if (it != userConnMap_.end()) {
            it->second->send(js.dump());
        } else {
            User user = userModel_.query(id);
            if (user.getState() == "online") {
                //消息发送至 redis id通道上
                redis_.publish(id , js.dump());
            } else {
                //存储离线消息
                offlineMsgModel_.insert(id , js.dump());
            }
        }
    }
}

void ChatService::handleRedisSubscribeMessage(int userid , string msg) {
    json js = json::parse(msg.c_str());

    lock_guard<mutex> lock(connMutex_);
    auto it = userConnMap_.find(userid);
    if (it != userConnMap_.end()) {
        it->second->send(js.dump());
        return;
    }

    offlineMsgModel_.insert(userid , msg);
}
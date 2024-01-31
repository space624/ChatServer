#ifndef CHATSERVICE_H
#define CHARSERVICE_H

#include <muduo/net/TcpConnection.h>

#include <functional>
#include <mutex>
#include <unordered_map>

#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "json.hpp"
#include "offlinemessagemodel.hpp"
#include "usermodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;

using json = nlohmann::json;

// 处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr& conn, json& js, Timestamp)>;

// 单列
class ChatService {
   public:
    static ChatService* instance();

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr& conn);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 添加好友
    void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //  登录
    void login(const TcpConnectionPtr& conn, json& js, Timestamp time);

    // 注册
    void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //注销业务
    void loginout(const TcpConnectionPtr& conn, json& js, Timestamp time);

    //从redis消息队列中获取订阅消息
    void handleRedisSubscribeMessage(int userid, string msg);

    // 获取消息对应的处理器
    MsgHandler getHandler(int magId);

    // 服务器异常业务重置
    void reset();

   private:
    ChatService();
    // 消息id,对映事件的处理方法
    unordered_map<int, MsgHandler> msgHandlerMap_;
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> userConnMap_;

    mutex connMutex_;

    // 数据操作类
    UserModel userModel_;
    OfflineMsgModel offlineMsgModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;
    Redis redis_;
};

#endif
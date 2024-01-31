#include "chatserver.hpp"
#include "chatservice.hpp"
#include "json.hpp"

#include <functional>
#include <string>

using namespace std;
using namespace placeholders;

using json = nlohmann::json;

ChatServer::ChatServer(EventLoop* loop, const InetAddress& listAddr, const string& nameArg)
    : server_(loop, listAddr, nameArg), loop_(loop) {
    // 注册连接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    server_.setThreadNum(4);
}

void ChatServer::start() {
    server_.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn) {
    // 断开连接
    if (!conn->connected()) {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time) {
    string buf = buffer->retrieveAllAsString();
    json js = json::parse(buf);
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器来执行相应的业务处理
    msgHandler(conn, js, time);
}
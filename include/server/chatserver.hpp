#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer{
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop * loop , const InetAddress & listAddr , const string & nameArg);
    //启动服务
    void start();
private:
    //连接相关的回调函数
    void onConnection(const TcpConnectionPtr & );
    //读写相关的回调
    void onMessage(const TcpConnectionPtr & , Buffer * ,Timestamp );

    TcpServer   server_;    //组合muduo库,实现服务器功能
    EventLoop * loop_;      //事件循环的指针

};

#endif
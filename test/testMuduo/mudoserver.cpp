#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <string>
#include <functional>

/*
基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象指针
3.明确TcpServer构造函数需要什么参数,输出ChatSercer的构造函数
4.当前服务器类的构造函数中,注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务端线程数量数,muduo库会分配
*/
class ChatServer
{
public:
    ChatServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, const std::string &nameArg) : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_.setThreadNum(4);
    }

    void start()
    {
        server_.start();
    }

private:
    // 用户的连接创建和断开
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:online " << std::endl;
        }
        else
        {
            std::cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state:offline " << std::endl;
            conn->shutdown();   //close(fd)
            //loop_->quit();
        }
    }

    // 用户的读写事件
    void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp time)
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data:" << buf << " time: " << time.toString() << std::endl;
        conn->send(buf);
    }
    muduo::net::TcpServer server_;
    muduo::net::EventLoop *loop_;
};

int main(){
    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");
    server.start();
    loop.loop();
    return 0;
}
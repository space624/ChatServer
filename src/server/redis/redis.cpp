#include "redis.hpp"
#include <iostream>

Redis::Redis() :publish_context_(nullptr) , subcribe_context_(nullptr) {

}

Redis::~Redis() {
    if (publish_context_) {
        redisFree(publish_context_);
    }

    if (subcribe_context_) {
        redisFree(subcribe_context_);
    }
}

bool Redis::connect() {
    publish_context_ = redisConnect("127.0.0.1" , 6379);
    if (!publish_context_) {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    subcribe_context_ = redisConnect("127.0.0.1" , 6379);
    if (!subcribe_context_) {
        cerr << "connect redis-server failed!" << endl;
        return false;
    }

    //线程监听通道上的事件,有消息就给业务层上报
    thread t([&]() {
        observer_channel_message();
    });

    t.detach();
    cout << "connect redis-server success!" << endl;

    return true;
}

bool Redis::publish(int channel , string message) {
    redisReply* reply = (redisReply*) redisCommand(publish_context_ , "PUBLISH %d %s" , channel , message.c_str());
    if (!reply) {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool Redis::subscribe(int channel) {
    //SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息,这里只做订阅通道,不接收通道消息
    //通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    //只负责发送命令,不阻塞接收redis server响应消息,否则和notifyMsg线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->subcribe_context_ , "SUBSCRIBE %d" , channel)) {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区,知道缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while (!done) {
        if (REDIS_ERR == redisBufferWrite(this->subcribe_context_ , &done)) {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

bool Redis::unsubscribe(int channel) {
    if (REDIS_ERR == redisAppendCommand(this->subcribe_context_ , "UNSUBSCRIBE %d" , channel)) {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区,知道缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while (!done) {
        if (REDIS_ERR == redisBufferWrite(this->subcribe_context_ , &done)) {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

void Redis::observer_channel_message() {
    redisReply* reply = nullptr;
    while (REDIS_OK == redisGetReply(this->subcribe_context_ , (void**) &reply)) {
        if (reply && reply->element[2] && reply->element[2]->str) {
            notify_message_handler_(atoi(reply->element[1]->str) , reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>>>>>>>>> objserver_channel_message quit <<<<<<<<<<<<<<<<<<<<" << endl;
}

void Redis::init_notify_handler(function<void(int , string)>fn) {
    this->notify_message_handler_ = fn;
}
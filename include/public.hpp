#ifndef PUBLIC_H
#define PUBLIC_H
/*
server 和 client的公共文件
*/
enum EnMagType{
    LOGIN_MSG = 1,      //登录消息
    LOGIN_MSG_ACK,      //登陆响应消息
    LOGINOUT_MSG,       //注销
    REG_MSG,            //注册消息
    REG_MSG_ACK,        //注册响应消息
    ONE_CHAT_MSG,       //聊天消息
    ADD_FRIEND_MSG,     //添加好友

    CREATE_GROUP_MSG,   //创建群组
    ADD_GROUP_MSG,      //加入群聊
    GROUP_CHAT_MSG,     //群聊天
};


#endif
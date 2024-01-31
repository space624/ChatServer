#include <iostream>
#include <vector>
#include <map>
#include <string>
#include "json.hpp"

void func1(){
    nlohmann::json js;
    js["msg_type"] = 2;
    js["from"]="zhang san";
    js["to"] = "li si";
    js["msg"] = "hello,what are you doing now?";

    std::string sendBuf = js.dump();
    std::cout << sendBuf.c_str() << std::endl;
}


int main(){
    func1();
    return 0;
}

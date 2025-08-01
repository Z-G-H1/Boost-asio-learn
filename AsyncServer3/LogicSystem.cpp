#include "LogicSystem.h"

LogicSystem::LogicSystem(): _b_stop(false){
    RegisterCallbacks();
    _work_thread = std::thread(&LogicSystem::Dealmsg);
}

void LogicSystem::PostToQue(shared_ptr<LogicNode> msg){

}

LogicSystem::~LogicSystem(){

}



void LogicSystem::Dealmsg(){
    while (1)
    {
        std::unique_lock<mutex> unique_lk(_mutex);
        // 空队列且运行状态， 阻塞挂起线程
        while(_msg_que.empty() && !_b_stop){
            _consume.wait(unique_lk);
        }

        // 如果停服， 将队列中剩余数据处理完毕，退出循环
        if(_b_stop){
            while(!_msg_que.empty()){
                auto msg_node = _msg_que.front();
                cout << "recv msg id is " << msg_node->_recv_node->_msg_id << endl;;
                // 根据id查找回调函数
                auto call_back_iter = _fun_callbacks.find(msg_node->_recv_node->_msg_id);
                if(call_back_iter == _fun_callbacks.end()){
                    // 没有对应的回调函数，移除
                    _msg_que.pop();
                    continue;
                }
                //调用回调函数
                call_back_iter->second(msg_node->_session, msg_node->_recv_node->_msg_id,
                    std::string(msg_node->_recv_node->_data, msg_node->_recv_node->_cur_len));
                _msg_que.pop();
            }
            break;
        }

        // 没有停服
        auto msg_node = _msg_que.front();
        // consumer发送的激活信号
         cout << "recv msg id is " << msg_node->_recv_node->_msg_id << endl;;
        auto call_back_iter = _fun_callbacks.find(msg_node->_recv_node->_msg_id);
        if(call_back_iter == _fun_callbacks.end()){
            // 没有对应的回调函数，移除
            _msg_que.pop();
            continue;
        }
        //调用回调函数
        // ???????????????????????????????????? 为什么使用cur——len 不是totallen
        call_back_iter->second(msg_node->_session, msg_node->_recv_node->_msg_id,
            std::string(msg_node->_recv_node->_data, msg_node->_recv_node->_cur_len));
        _msg_que.pop();
        
    }
    
}
void LogicSystem::RegisterCallbacks(){
    _fun_callbacks[MSG_HELLO_WORD] = [this](shared_ptr<CSession> session, short msg_id, string msg_data){
        HelloWorldCallBack(session,msg_id,msg_data);
    };
}
void LogicSystem::HelloWorldCallBack(shared_ptr<CSession> session, short msg_id, string msg_data){
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data, root);
    std:: cout << "recv msg id is " <<  root["id"].asInt() << " msg data is "
        << root["data"].asString() << std::endl;
    root["data"] = "server has received msg, msg data is " + root["data"].asString();
    std::string return_str = root.toStyledString();
    session->Send(return_str,root["id"].asInt());
}
#include "CSession.h"
#include "CServer.h"
#include <iostream>
CSession::CSession(boost::asio::io_context& io_context, CServer* server):
	_socket(io_context), _server(server){
	boost::uuids::uuid  a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
}

tcp::socket& CSession::GetSocket() {
	return _socket;
}

std::string& CSession::GetUuid() {
	return _uuid;
}

void CSession::Start(){
	memset(_data, 0, MAX_LENGTH);
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),[self = shared_from_this()]
        (const boost::system::error_code& error, size_t  bytes_transferred){
        self->HandleRead(error,bytes_transferred);
    });
}

void CSession::Send(char* msg,  int max_length){
    bool pending = false;
    // 队列上锁
    std::lock_guard<mutex> lock(_send_lock);
    if(_send_que.size() > 0)
        pending = true;
    _send_que.push(make_shared<MsgNode>(msg, max_length));
    // 防止同一个socket 发起两个异步写操作
    if(pending)
        return ;
    boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
    [self = shared_from_this()](const boost::system::error_code& error,size_t bytes_transferred){
        self->HandleWrite(error, bytes_transferred);
    });

}


void CSession::HandleWrite(const boost::system::error_code& error,size_t bytes_transferred) {
	if (!error) {
        std::lock_guard<mutex> lock(_send_lock);
        _send_que.pop();
        if(!_send_que.empty()){
            // 队列不为空，继续把队列中的数据发送出去
            auto &msg_node = _send_que.front();
            boost::asio::async_write(_socket,boost::asio::buffer(msg_node->_data, msg_node->_max_len),
            [self = shared_from_this()](const boost::system::error_code& error,size_t bytes_transferred){
                self->HandleWrite(error, bytes_transferred);
            });
        }
	}
	else {
		std::cout << "handle write failed, error is " << error.what() << endl;
		_server->ClearSession(_uuid);
	}
}

void CSession::HandleRead(const boost::system::error_code& error, size_t  bytes_transferred){
	if (!error) {
        cout << "server receive data is" << _data << endl;
        // 把消息发送回去，并再次进行监听
        Send(_data, bytes_transferred);
        memset(_data, 0, MAX_LENGTH);
        _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),[self = shared_from_this()]
        (const boost::system::error_code& error, size_t  bytes_transferred){
            self->HandleRead(error,bytes_transferred);
        });

	}
	else {
		std::cout << "handle read failed, error is " << error.what() << endl;
		_server->ClearSession(_uuid);
	}
}

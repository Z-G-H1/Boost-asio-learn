#include "CSession.h"
#include "CServer.h"
#include <iostream>
CSession::CSession(boost::asio::io_context& io_context, CServer* server):
	_socket(io_context), _server(server), _b_head_parse(false){
	boost::uuids::uuid  a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_TOTAL_LEN);
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

void CSession::Send(char* msg,  int max_length, short msgid){
    // 队列上锁
    std::lock_guard<mutex> lock(_send_lock);
    int send_que_size = _send_que.size();

    if(send_que_size > MAX_SENDQUE){
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return ;
    }
    _send_que.push(make_shared<SendNode>(msg, max_length, msgid));
    // 防止同一个socket 发起两个异步写操作
    if(send_que_size > 0){
        return ;
    }
    auto &msgnode = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
    [self = shared_from_this()](const boost::system::error_code& error,size_t bytes_transferred){
        self->HandleWrite(error, bytes_transferred);
    });
}

void CSession::Send(std::string msg, short msgid){
    std::lock_guard<mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if(send_que_size > MAX_SENDQUE){
        std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
        return ;
    }
    _send_que.push(make_shared<SendNode>(msg.c_str(), msg.size(), msgid));
    if(send_que_size > 0){
        return ;
    }
    auto &msgnode = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
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
            boost::asio::async_write(_socket,boost::asio::buffer(msg_node->_data, msg_node->_total_len),
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

// 更新HandleRead函数，处理粘包问题。
void CSession::HandleRead(const boost::system::error_code& error, size_t  bytes_transferred){
	if (!error) {
        int copy_len = 0;   // 处理的字节数
        while (bytes_transferred > 0){
            if(!_b_head_parse){
                // 需要先处理头部节点
                //如果接受的数据小于头部长度
                if(bytes_transferred + _recv_head_node->_cur_len < HEAD_TOTAL_LEN){
                    // 先将收到的数据保存到recv_head_node中
                    memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data+copy_len, bytes_transferred);
                    _recv_head_node->_cur_len += bytes_transferred;
                    memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data,MAX_LENGTH),[self = shared_from_this()]
                        (const boost::system::error_code& error, size_t  bytes_transferred){
                        self->HandleRead(error,bytes_transferred);
                    });
                    return ;
                }
                // 将头部节点处理好
                int head_remain = HEAD_TOTAL_LEN - _recv_head_node->_cur_len;
                memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data+copy_len ,head_remain);
                copy_len += head_remain;                // 更新处理的字节数
                bytes_transferred -= head_remain;       // 更新未处理的数据
                // 获取消息的msgid
                short msg_id = 0;
                memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
                // 网络字节序转化为本地字节序
                msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                // id 非法
                if(msg_id > MAX_LENGTH){
                    std::cout << "invalid msg_id is " << msg_id << endl;
                    _server->ClearSession(_uuid);
                    return;
                }

                // 头部节点存储的是 消息体的长度
                short msg_len;
                memcpy(&msg_len, _recv_head_node->_data+HEAD_ID_LEN, HEAD_DATA_LEN);
                cout << "msg_len is " << msg_len << endl;

                // 数据长度非法
                if(msg_len > MAX_LENGTH){
                    std::cout << "invalid data length is " << msg_len << endl;
                    _server->ClearSession(_uuid);
                    return;
                }
                // 此时可以创建接受节点了
                _recv_msg_node = make_shared<RecvNode>(msg_len, msg_id);

                /* 下面的部分都是对消息节点的处理，因为接受到了完整的头节点， 即使未处理的数据不完整，
                    下一次也不会在走这个if语句（！_b_head_parse）了*/
                //消息的长度小于头部规定的长度，说明数据未收全，则先将部分消息放到接收节点里
                if(bytes_transferred < msg_len){
                    memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data+copy_len, bytes_transferred);
                    _recv_msg_node->_cur_len += bytes_transferred;
                    memset(_data, 0, MAX_LENGTH);
                    _b_head_parse = true;
                    _socket.async_read_some(boost::asio::buffer(_data,MAX_LENGTH),[self = shared_from_this()]
                        (const boost::system::error_code& error, size_t  bytes_transferred){
                        self->HandleRead(error,bytes_transferred);
                    });
                    // 因为接受的数据都处理完了，要返回
                    return ;
                }

                //走到这里说明消息足够长， 处理完消息后边应该是 下一个消息的消息头， 记得把 parse变量置为false
                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data+copy_len, msg_len);
                _recv_msg_node->_cur_len += msg_len;
                bytes_transferred -= msg_len;
                copy_len += msg_len;
                _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
                // cout << "receive data is " << _recv_msg_node->_data << endl;
                //此处可以调用Send发送测试
                Json::Reader reader;
                Json::Value root;
                reader.parse(string(_recv_msg_node->_data, _recv_msg_node->_total_len), root);
                std::cout << "receive msg id is " << root["id"].asInt() << "msg data is " << root["data"].asString() << std::endl;
				root["data"] = "server has received msg, msg data is " + root["data"].asString();
                std::string return_str = root.toStyledString();
                Send(return_str, root["id"].asInt());
                _b_head_parse = false;
                // 再次轮询，需要清空头节点的数据，因为头节点是复用的
                _recv_head_node->Clear();
                if(bytes_transferred <= 0){
                    memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(boost::asio::buffer(_data,MAX_LENGTH),[self = shared_from_this()]
                        (const boost::system::error_code& error, size_t  bytes_transferred){
                        self->HandleRead(error,bytes_transferred);
                    });
                    return ;
                }
                continue;
            }


            // 剩余部分处理 if(bytes_transferred < data_len){ 这种情况
            int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
            if(bytes_transferred < remain_msg){
                memcpy(_recv_msg_node->_data+_recv_msg_node->_cur_len, _data+copy_len, bytes_transferred);
                _recv_msg_node->_cur_len += bytes_transferred;
                memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data,MAX_LENGTH),[self = shared_from_this()]
                        (const boost::system::error_code& error, size_t  bytes_transferred){
                        self->HandleRead(error,bytes_transferred);
                });
                return;
            }

            memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data+copy_len, remain_msg);
            _recv_msg_node->_cur_len += remain_msg;
            bytes_transferred -= remain_msg;
			copy_len += remain_msg;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
            // cout << "receive data is " << _recv_msg_node->_data << endl;
			//此处可以调用Send发送测试
            Json::Reader reader;
            Json::Value root;
            reader.parse(string(_recv_msg_node->_data, _recv_msg_node->_total_len), root);
            std::cout << "receive msg id is " << root["id"].asInt() << "msg data is " << root["data"].asString() << std::endl;
			root["data"] = "server has received msg, msg data is " + root["data"].asString();
            std::string return_str = root.toStyledString();
			Send(return_str, root["id"].asInt());
			//继续轮询剩余未处理数据
			_b_head_parse = false;
			_recv_head_node->Clear();
			if (bytes_transferred <= 0) {
				memset(_data, 0, MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data,MAX_LENGTH),[self = shared_from_this()]
                        (const boost::system::error_code& error, size_t  bytes_transferred){
                        self->HandleRead(error,bytes_transferred);
                });
				return;
			}
			continue;
		}
	}
	else {
		std::cout << "handle read failed, error is " << error.what() << endl;
		_server->ClearSession(_uuid);
	}
}

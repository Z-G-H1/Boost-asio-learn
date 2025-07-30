#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <iostream>

#include "MsgNode.h"

using boost::asio::ip::tcp;
class CServer;

class CSession:public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& io_context, CServer* server);
	~CSession() {
		std::cout << "Ssession destruct" << endl;
	}
	tcp::socket& GetSocket();
	std::string& GetUuid();
	void Start();
	void Send(char* msg,  int max_length, short msgid);
	void Send(std::string msg, short msgid);


private:
	void HandleRead(const boost::system::error_code& error, size_t bytes_transferred);
	void HandleWrite(const boost::system::error_code& error,size_t bytes_transferred);
	tcp::socket _socket;
	std::string _uuid;
	char _data[MAX_LENGTH];
	CServer* _server;
	std::queue<shared_ptr<SendNode> > _send_que;
	std::mutex _send_lock;
	
	bool _b_head_parse;
	std::shared_ptr<MsgNode> _recv_head_node;
	std::shared_ptr<RecvNode> _recv_msg_node;
};


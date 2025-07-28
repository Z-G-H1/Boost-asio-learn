#pragma once
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <queue>
#include <mutex>
#include <memory>
#include <iostream>

using namespace std;
#define MAX_LENGTH  1024*2
#define HEAD_LENGTH 2
using boost::asio::ip::tcp;
class CServer;

class MsgNode
{
	friend class CSession;
public:
	MsgNode(char * msg, short max_len) :_cur_len(0), _total_len(max_len + HEAD_LENGTH) {
		//末尾括号，初始化为0
		_data = new char[_total_len+1]();
		memcpy(_data, msg, HEAD_LENGTH);
		memcpy(_data, msg+HEAD_LENGTH, max_len);
		_data[_total_len+1] = '\0';
	}

	MsgNode(short max_len): _cur_len(0), _total_len(max_len+HEAD_LENGTH){
		_data = new char[_total_len+1]();
	}

	~MsgNode() {
		delete[] _data;
	}

	void Clear(){
		memset(_data, 0, _total_len);
		_cur_len = 0;
	}

private:
	short _cur_len;
	short _total_len;
	char* _data;
};

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
	void Send(char* msg,  int max_length);

private:
	void HandleRead(const boost::system::error_code& error, size_t bytes_transferred);
	void HandleWrite(const boost::system::error_code& error,size_t bytes_transferred);
	tcp::socket _socket;
	std::string _uuid;
	char _data[MAX_LENGTH];
	CServer* _server;
	std::queue<shared_ptr<MsgNode> > _send_que;
	std::mutex _send_lock;
	
	bool _b_head_parse;
	std::shared_ptr<MsgNode> _recv_head_node;
	std::shared_ptr<MsgNode> _recv_msg_node;
};


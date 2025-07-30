#include "MsgNode.h"

RecvNode::RecvNode(short max_len,short msg_id):MsgNode(max_len),_msg_id(msg_id){

}

SendNode::SendNode(const char * msg, short max_len, short msg_id)
    : MsgNode(max_len+HEAD_TOTAL_LEN), _msg_id(msg_id){
    // 发送数据id。
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data, &msg_id_host, HEAD_ID_LEN);
    // 发送网络字节序。转换
    short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data+HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
    memcpy(_data+HEAD_TOTAL_LEN, msg, max_len);
}


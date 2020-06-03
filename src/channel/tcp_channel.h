/*
 * * file name: tcp_channel.h
 * * description: ...
 * * author: snow
 * * create time:2020  6 03
 * */

#ifndef _TCP_CHANNEL_H_
#define _TCP_CHANNEL_H_

namespace pepper
{
// todo 接口设计似乎不大合理
class TcpChannel
{
public:
    /// 参数分别是数据包和长度，以及数据来源id
    using RecvFunc = std::function<void(const char*, size_t, uint64_t)>;
    /// ip, port
    using Addr = std::pair<std::string, std::string>;

    TcpChannel() : m_io_context(1), m_acceptor(nullptr) {}
    bool init(const Addr& listen_addr_, const std::vector<Addr>& dest_addrs_, const RecvFunc& recv_func_);
    size_t update();
    bool send(uint64_t dest_, const char* buf_, size_t len_);

private:
    void accept();

private:
    asio::io_context m_io_context;
    asio::ip::tcp::acceptor* m_acceptor;
    std::map<uint64_t, asio::ip::tcp::socket> m_sockets;
    RecvFunc m_recv_func;
};
}  // namespace pepper
#endif

/*
 * * file name: tcp_channel.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  6 03
 * */

#include "tcp_channel.h"

namespace pepper
{
bool TcpChannel::init(const Addr& listen_addr_, const std::vector<Addr>& dest_addrs_, const RecvFunc& recv_func_)
{
    // 需要自己监听一个端口
    if (!listen_addr_.first.empty())
    {
        m_acceptor = new asio::ip::tcp::acceptor(
            m_io_context,
            asio::ip::tcp::endpoint(asio::ip::address::from_string(listen_addr_.first), listen_addr_.second));

        if (!m_acceptor)
            return false;
    }

    // 需要连上的远端
    if (!dest_addrs_.empty())
    {
        for (auto&& addr : dest_addrs_)
        {
            if (addr.first.empty())
                continue;

            // todo
        }
    }

    // todo
    return true;
}

size_t TcpChannel::update()
{
    // todo
    return 0;
}

bool TcpChannel::send(uint64_t dest_, const char* buf_, size_t len_)
{
    // todo
    return true;
}

}  // namespace pepper

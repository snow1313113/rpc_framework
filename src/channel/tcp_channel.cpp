/*
 * * file name: tcp_channel.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  6 03
 * */

#include <memory>
#include "tcp_channel.h"

namespace pepper
{
class session : public std::enable_shared_from_this<session>
{
public:
    session(asio::ip::tcp::socket socket_) : m_socket(std::move(socket_)) {}

    void read()
    {
        auto self(shared_from_this());
        asio::async_read(m_socket, asio::buffer(m_read_buf, MAX_READ_BUF_LEN),
                         [&, self](std::error_code ec_, std::size_t len_) {
                             if (ec_)
                                 return;
                             // todo 回调上层的收包接口
                             read();
                         });
    }

    void send()
    {
        // todo
    }

private:
    asio::ip::tcp::socket m_socket;
    static constexpr size_t MAX_READ_BUF_LEN = 1024;
    char m_read_buf[MAX_READ_BUF_LEN];
};

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

        accept();
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
    // todo 有抛异常哦
    return m_io_context.run_for(std::chrono::microseconds(2));
}

bool TcpChannel::send(uint64_t dest_, const char* buf_, size_t len_)
{
    // todo
    return true;
}

void TcpChannel::accept()
{
    m_acceptor->async_accept([&](std::error_code ec_, asio::ip::tcp::socket socket_) {
        if (ec_)
            return;
        std::make_shared<session>(std::move(socket_))->read();
        // 继续监听
        accept();
    });
}

}  // namespace pepper

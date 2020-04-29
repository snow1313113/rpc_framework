/*
 * * file name: channel_interface.h
 * * description: ...
 * * author: snow
 * * create time:2020  4 29
 * */

#ifndef _CHANNEL_INTERFACE_H_
#define _CHANNEL_INTERFACE_H_

#include <functional>

namespace ua
{
/// channel的接口，抽象出多点网格的channel概念，每个点都需要有一个唯一的ID
class IChannel
{
public:
    /// 参数分别是数据头指针，数据长度，已经数据来源id
    using RecvFunc = std::function<void(const char*, size_t, uint64_t)>;

    void set_recv_func(const RecvFunc& recv_fun_) { m_recv_func = recv_fun_; }
    void set_recv_func(RecvFunc&& recv_fun_) { m_recv_func = std::move(recv_fun_); }

    /// 如果*dest != 0，则根据*dest发送，否则根据默认规则发送，并且dest带回最终发送的目标
    virtual uint64_t send(const char* buff_, size_t buff_len_, uint64_t* dest_ = nullptr) = 0;
    /// 广播
    virtual uint64_t broadcast(const char* buff_, size_t buff_len_) = 0;
    /// 执行处理，返回处理了多少次收包
    virtual size_t process() = 0;
    virtual ~IChannel() = default;

protected:
    RecvFunc m_recv_func = nullptr;
};

}  // namespace ua
#endif

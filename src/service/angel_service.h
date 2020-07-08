/*
 * * file name: angel_service.h
 * * description: ...
 * * author: snow
 * * create time:2020  4 25
 * */

#ifndef _ANGEL_SERVICE_H_
#define _ANGEL_SERVICE_H_

#include <memory>
#include <unordered_map>
#include <vector>
#include "angel_context.h"
#include "angel_pkg.h"
#include "base/context_controller.h"
#include "base/rpc_error.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/service.h"

namespace pepper
{
class IAngelChannel
{
public:
    /// 参数分别是数据包，以及数据来源id
    using RecvFunc = std::function<void(const AngelPkg&, uint64_t)>;

    /// 设置收包的回调函数
    void set_recv_func(const RecvFunc& recv_fun_) { m_recv_func = recv_fun_; }
    void set_recv_func(RecvFunc&& recv_fun_) { m_recv_func = std::move(recv_fun_); }
    /// 如果*dest != 0，则根据*dest发送，否则根据默认规则发送，并且dest带回最终发送的目标
    virtual uint64_t send(const AngelPkgHead& head_, const char* body_, uint64_t* dest_ = nullptr) = 0;
    /// 广播
    virtual uint64_t broadcast(const AngelPkgHead& head_, const char* body_) = 0;
    /// 执行处理，返回处理了多少次收包
    virtual size_t process() = 0;

    virtual ~IAngelChannel() = default;

protected:
    RecvFunc m_recv_func = nullptr;
};

class AngelService
{
public:
    using NextFun = Context::NextFun;
    static AngelService& AngelObj()
    {
        static AngelService only;
        return only;
    }

    /// 初始化
    bool init();
    /// 设置conext controller，适配BaseSvr
    void set_context_ctrl(ContextController* context_ctrl_);
    /// 添加通道，返回通道的索引
    uint32_t add_channel(IAngelChannel* channel_);
    /// 注册提供的服务
    bool register_angel_service(google::protobuf::Service* service_);
    /// 循环收发包处理，返回处理了多个请求和超时事件，适配BaseSvr
    size_t loop_once();
    /// 发起rpc，当前需要处于协程中，timeout_是超时时间间隔，单位是ms
    int32_t rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_, google::protobuf::Message* rsp_,
                uint64_t dest_ = 0, bool broadcast_ = false, uint32_t timeout_ = 3000);
    /// 发起异步rpc，当前不能处于协程中，callback是回包或超时的时候的回调，timeout_是超时时间间隔，单位是ms
    int32_t async_rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                      google::protobuf::Message* rsp_ = nullptr, const NextFun& next_fun_ = nullptr,
                      Context* context_ = nullptr, uint64_t dest_ = 0, bool broadcast_ = false,
                      uint32_t timeout_ = 3000);
    /// 设置通道收包开关
    void channel_switch(bool stop_);

private:
    ~AngelService() = default;
    int32_t send(uint32_t channel_index_, AngelPkgHead& head_, const google::protobuf::Message& msg_,
                 bool broadcast_ = false);
    void on_recv(uint32_t channel_index_, const AngelPkg& pkg_, uint64_t src_);
    bool deal_request(uint32_t channel_index_, const AngelPkg& pkg_, uint64_t src_);
    void deal_method(AngelContext* context_, google::protobuf::Service* service_,
                     const google::protobuf::MethodDescriptor* method_desc_);
    void deal_response(const AngelPkg& head_);
    void method_finish(AngelContext* context_);

private:
    /// 已经注册的method
    struct PBMethod
    {
        google::protobuf::Service* service = nullptr;
        const google::protobuf::MethodDescriptor* method = nullptr;
        const google::protobuf::Message* req_type = nullptr;
        const google::protobuf::Message* rsp_type = nullptr;
        bool is_private = false;
    };
    std::unordered_map<uint32_t, PBMethod> m_methods;
    /// 发出远程调用后等待回包的上下信息
    struct AngelRpcCache
    {
        google::protobuf::Message* rsp = nullptr;
        uint64_t gid = 0;
        uint32_t cmd = 0;
    };
    std::unordered_map<uint64_t, AngelRpcCache> m_rpc_cache;
    // 收发包的多个通道
    std::vector<IAngelChannel*> m_channels;
    // 上下文切换管理器
    ContextController* m_context_ctrl = nullptr;
    // 停止从通道收包开关
    bool m_channel_switch = false;
    // 发送用的buf
    static constexpr size_t SEND_BUF_LEN = 4 * 1024;
    char m_send_buf[SEND_BUF_LEN];
};

}  // namespace pepper

#endif

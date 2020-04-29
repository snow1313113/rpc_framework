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
#include "angel_pkg.h"
#include "base/context_controller.h"
#include "base/singleton.h"

namespace pepper
{
class AngelService : public Singleton<AngelService>
{
public:
    using NextFun = Context::NextFun;

    /// 初始化
    bool init();
    /// 设置conext controller，适配BaseSvr
    void set_context_ctrl(ContextController* context_ctrl_);
    /// 添加通道，返回通道的索引
    uint32_t add_channel(IChannel* channel_);
    /// 注册提供的服务
    bool register_pb_service(google::protobuf::Service* service_);
    /// 循环收发包处理，返回处理了多个请求和超时事件，适配BaseSvr
    size_t loop_once();
    /// 发起rpc，当前需要处于协程中，timeout_是超时时间间隔，单位是ms
    int32_t rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_, google::protobuf::Message* rsp_,
                uint64_t dest_ = 0, bool broadcast_ = false, uint32_t timeout_ = 3000);
    /// 发起异步rpc，当前不能处于协程中，callback是回包或超时的时候的回调，timeout_是超时时间间隔，单位是ms
    int32_t async_rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                      google::protobuf::Message* rsp_ = nullptr, NextFun next_fun = nullptr, Context* context = nullptr,
                      uint64_t dest_ = 0, bool broadcast_ = false, uint32_t timeout_ = 3000);
    /// 设置通道收包开关
    void channel_switch(bool stop_);

private:
    void on_recv(uint32_t channel_index, const char* data_, size_t len_, uint64_t src);

private:
    friend Singleton<AngelService>;
    AngelService(const AngelService&) = delete;
    AngelService(AngelService&&) = delete;
    AngelService& operator=(const AngelService&) = delete;
    AngelService() = default;
    ~AngelService() = default;

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
    std::vector<IChannel*> m_channels;
    // 上下文切换管理器
    ContextController* m_context_ctrl = nullptr;
    // 停止从通道收包开关
    bool m_channel_switch = false;
};

}  // namespace pepper

#endif

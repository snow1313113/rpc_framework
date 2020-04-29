/*
 * * file name: angel_service.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  4 25
 * */

#include "angel_service.h"
#include <memory>
#include "base/rpc_error.h"

namespace pepper
{
bool AngelService::init()
{
    // todo
    return false;
}

void AngelService::set_context_ctrl(ContextController* context_ctrl_)
{
    m_context_ctrl = context_ctrl_;
}

uint32_t AngelService::add_channel(IChannel* channel_)
{
    if (channel_)
    {
        uint32_t channel_index = m_channels.size() + 1;
        channel_->set_recv_func(
            [=](const char* data_, size_t len_, src_) { on_recv(channel_index, data_, len_, src_); });
        m_channels.push_back(channel_);
        return channel_index;
    }
    return 0;
}

bool AngelService::register_pb_service(google::protobuf::Service* service_)
{
    auto service_desc = service->GetDescriptor();
    auto service_type = service_desc->options().GetExtension(SERVICE_TYPE);
    if (service_type != ANGEL_SERVICE)
    {
        ERR_LOG(0, 0, 0, "", "register tcp service to pb service");
        return false;
    }

    bool is_service_private = service_desc->options().GetExtension(IS_ALL_PRIVATE);
    for (int i = 0; i < service_desc->method_count(); ++i)
    {
        auto method = service_desc->method(i);
        if (method->options().GetExtension(METHOD_TYPE) != ANGEL_METHOD)
            continue;

        uint32_t cmd = method->options().GetExtension(METHOD_CMD);
        if (cmd == 0)
            return false;

        bool is_private = is_service_private || method->options().GetExtension(IS_PRIVATE);
        auto result = methods_.insert({cmd,
                                       {service, method, &(service->GetRequestPrototype(method)),
                                        &(service->GetResponsePrototype(method)), is_private}});
        if (!(result.second))
            return false;
    }

    return true;
}

size_t AngelService::loop_once()
{
    size_t deal_count = 0;
    if (!m_channel_switch)
    {
        for (auto&& channel : m_channels)
        {
            deal_count += channel->process();
        }
    }
    return deal_count;
}

int32_t AngelService::rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                          google::protobuf::Message* rsp_, uint64_t dest_, bool broadcast_, uint32_t timeout_)
{
    // todo
    return RPC_SUCCESS;
}

int32_t AngelService::async_rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                                google::protobuf::Message* rsp_, NextFun next_fun, Context* context, uint64_t dest_,
                                bool broadcast_, uint32_t timeout_)
{
    // todo
    return RPC_SUCCESS;
}

void AngelService::channel_switch(bool stop_)
{
    m_channel_switch = stop_;
}

void AngelService::on_recv(uint32_t channel_index, const char* data_, size_t len_, uint64_t src)
{
    // todo
}

}  // namespace pepper

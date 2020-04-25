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
        // todo
        m_channels.push_back(channel_);
        return channel_index;
    }
    return 0;
}

bool AngelService::register_pb_service(google::protobuf::Service* service_)
{
    // todo
    return false;
}

size_t AngelService::loop_once()
{
    // todo
    return 0;
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

}  // namespace pepper

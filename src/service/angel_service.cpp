/*
 * * file name: angel_service.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  4 25
 * */

#include "angel_service.h"
#include <memory>
#include "base/coroutine_mgr.h"
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

uint32_t AngelService::add_channel(IAngelChannel* channel_)
{
    if (channel_)
    {
        uint32_t channel_index = m_channels.size() + 1;
        channel_->set_recv_func([=](const AngelPkg& pkg_, uint64_t src_) { on_recv(channel_index, pkg_, src_); });
        m_channels.push_back(channel_);
        return channel_index;
    }
    return 0;
}

bool AngelService::register_pb_service(google::protobuf::Service* service_)
{
    auto service_desc = service_->GetDescriptor();
    auto type = service_desc->options().GetExtension(service_type);
    if (type != ANGEL_SERVICE)
        return false;

    bool is_service_private = service_desc->options().GetExtension(is_private);
    for (int i = 0; i < service_desc->method_count(); ++i)
    {
        auto method = service_desc->method(i);
        if (method->options().GetExtension(service_type) != ANGEL_METHOD)
            continue;

        uint32_t cmd = method->options().GetExtension(cmd);
        if (cmd == 0)
            return false;

        bool is_private = is_service_private || method->options().GetExtension(is_private);
        auto result = m_methods.insert({cmd,
                                        {service_, method, &(service_->GetRequestPrototype(method)),
                                         &(service_->GetResponsePrototype(method)), is_private}});
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
            deal_count += channel->process();
    }
    return deal_count;
}

int32_t AngelService::rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                          google::protobuf::Message* rsp_, uint64_t dest_, bool broadcast_, uint32_t timeout_)
{
    if (!m_context_ctrl->is_use_corotine())
        return RPC_SYS_ERR;

    if (broadcast_ && rsp_)
        return RPC_SYS_ERR;

    uint64_t new_seq_id = m_context_ctrl->generate_seq_id();

    AngelPkgHead head;
    head.gid = gid_;
    head.cmd = cmd_;
    head.ret = RPC_SUCCESS;
    head.msg_type = AngelPkgHead::REQ_MSG;
    if (rsp_)
        head.seq_id = new_seq_id;
    else
        head.pkg_flag |= AngelPkgHead::FLAG_DONT_RSP;

    // 从第一个通道发送
    auto ret = send(0, head, req_, broadcast_);
    if (ret != RPC_SUCCESS)
        return ret;

    if (rsp_)
    {
        auto result = m_rpc_cache.insert({new_seq_id, {rsp_, gid_, cmd_}});
        if (result.second)
        {
            // todo 这是要过期时间，不是超时间隔
            return m_context_ctrl->pending(new_seq_id, timeout_,
                                           [&](uint64_t timeout_seq_id) { m_rpc_cache.erase(timeout_seq_id); });
        }
        else
            return RPC_SYS_ERR;
    }

    return RPC_SUCCESS;
}

int32_t AngelService::async_rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                                google::protobuf::Message* rsp_, const NextFun& next_fun_, Context* context_,
                                uint64_t dest_, bool broadcast_, uint32_t timeout_)
{
    if (m_context_ctrl->is_use_corotine())
        return RPC_SYS_ERR;

    if (broadcast_ && rsp_)
        return RPC_SYS_ERR;

    if ((rsp_ && !next_fun_) || (!rsp_ && next_fun_))
        return RPC_SYS_ERR;

    uint64_t new_seq_id = m_context_ctrl->generate_seq_id();

    AngelPkgHead head;
    head.gid = gid_;
    head.cmd = cmd_;
    head.ret = RPC_SUCCESS;
    head.msg_type = AngelPkgHead::REQ_MSG;
    if (rsp_)
        head.seq_id = new_seq_id;
    else
        head.pkg_flag |= AngelPkgHead::FLAG_DONT_RSP;

    // 都从第一个channel发
    auto ret = send(0, head, req_, broadcast_);
    if (ret != RPC_SUCCESS)
        return ret;

    if (rsp_)
    {
        new_seq_id = m_context_ctrl->async_pending(new_seq_id, next_fun_, context_, timeout_,
                                                   [&](uint64_t timeout_seq_id) { m_rpc_cache.erase(timeout_seq_id); });
        if (new_seq_id == 0)
            return RPC_SYS_ERR;

        // 如果插入失败，等超时
        m_rpc_cache.insert({new_seq_id, {rsp_, gid_, cmd_}});
    }

    return RPC_SUCCESS;
}

void AngelService::channel_switch(bool stop_)
{
    m_channel_switch = stop_;
}

void AngelService::on_recv(uint32_t channel_index_, const AngelPkg& pkg_, uint64_t src_)
{
    if (pkg_.head.msg_type == AngelPkgHead::REQ_MSG)
        deal_request(channel_index_, pkg_, src_);
    else
        deal_response(pkg_);
}

bool AngelService::deal_request(uint32_t channel_index_, const AngelPkg& pkg_, uint64_t src_)
{
    auto iter = m_methods.find(pkg_.head.cmd);
    if (iter == m_methods.end())
        return false;

    if (iter->second.is_private)
        return false;

    auto&& rpc_method = iter->second;
    std::unique_ptr<google::protobuf::Message> new_req(rpc_method.req_type->New());
    if (!new_req->ParsePartialFromArray(pkg_.body, pkg_.head.len))
        return false;

    std::unique_ptr<google::protobuf::Message> new_rsp(rpc_method.rsp_type->New());
    std::unique_ptr<AngelContext> context(new AngelContext(channel_index_, pkg_.head, new_req.get(), new_rsp.get()));

    auto context_ptr = context.get();
    auto proto_service_ptr = rpc_method.service;
    auto method_desc_ptr = rpc_method.method;
    if (m_context_ctrl->is_use_corotine())
    {
        if (!CoroObjMgr::instance().spawn([=]() { deal_method(context_ptr, proto_service_ptr, method_desc_ptr); }))
            return false;
    }
    else
    {
        context_ptr->set_finish_fun([=]() { method_finish(context_ptr); });
        deal_method(context_ptr, proto_service_ptr, method_desc_ptr);
    }

    context.release();
    new_req.release();
    new_rsp.release();
    return true;
}

void AngelService::deal_method(AngelContext* context_, google::protobuf::Service* service_,
                               const google::protobuf::MethodDescriptor* method_desc_)
{
    service_->CallMethod(method_desc_, context_, context_->m_req, context_->m_rsp, nullptr);
    if (m_context_ctrl->is_use_corotine() || context_->is_finish())
        method_finish(context_);
}

void AngelService::method_finish(AngelContext* context_)
{
    if (!(context_->m_head.pkg_flag & AngelPkgHead::FLAG_DONT_RSP))
    {
        context_->m_head.pkg_flag |= AngelPkgHead::FLAG_DONT_RSP;
        context_->m_head.msg_type = AngelPkgHead::RSP_MSG;
        send(context_->m_channel_index, context_->m_head, *(context_->m_rsp));
    }

    // 结束的时候释放资源
    delete context_->m_rsp;
    delete context_->m_req;
    delete context_;
}

int32_t AngelService::send(uint32_t channel_index_, AngelPkgHead& head_, const google::protobuf::Message& msg_,
                           bool broadcast_)
{
    if (channel_index_ == 0 || channel_index_ > m_channels.size())
        return RPC_SYS_ERR;

    auto body_len = msg_.ByteSizeLong();
    if (body_len > 0)
    {
        if (body_len > SEND_BUF_LEN)
            return RPC_SYS_ERR;

        if (!msg_.SerializeToArray(m_send_buf, SEND_BUF_LEN))
            return RPC_SERIALIZE_MSG_ERR;
    }
    head_.len = body_len;

    if (broadcast_)
        return m_channels[channel_index_ - 1]->broadcast(head_, m_send_buf);
    else
        return m_channels[channel_index_ - 1]->send(head_, m_send_buf);
}

void AngelService::deal_response(const AngelPkg& pkg_)
{
    uint64_t seq_id = pkg_.head.seq_id;
    auto iter = m_rpc_cache.find(seq_id);
    if (iter == m_rpc_cache.end())
        return;

    auto cache = iter->second;
    assert(cache.rsp);
    m_rpc_cache.erase(iter);

    int32_t ret_code = RPC_SUCCESS;
    if (pkg_.head.gid != cache.gid || pkg_.head.cmd != cache.cmd)
        ret_code = RPC_SYS_ERR;
    else
    {
        ret_code = pkg_.head.ret;
        if (ret_code == RPC_SUCCESS && pkg_.head.len > 0)
        {
            if (!cache.rsp->ParsePartialFromArray(pkg_.body, pkg_.head.len))
                ret_code = RPC_PARSE_MSG_ERR;
        }
    }

    m_context_ctrl->awake(seq_id, ret_code);
}

}  // namespace pepper

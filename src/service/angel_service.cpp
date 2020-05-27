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

uint32_t AngelService::add_channel(IAngelChannel* channel_)
{
    if (channel_)
    {
        uint32_t channel_index = m_channels.size() + 1;
        channel_->set_recv_func([=](const AngelPkgHead& head_, const char* data_, size_t len_, src_) {
            on_recv(channel_index, head_, data_, len_, src_);
        });
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
    head.len = req_.ByteSizeLong();

    // 从第一个通道发送
    auto ret = send(0, head, req_, broadcast_);
    if (ret != RPC_SUCCESS)
        return ret;

    if (rsp_)
    {
        auto coro = CoroObjMgr::instance().cur_coro();
        assert(coro);
        auto result = m_rpc_cache.insert({new_seq_id, {rsp_, gid_, cmd_)}});
        if (result.second)
        {
            // todo 这是要过期时间，不是超时间隔
            ret = m_context_ctrl->pending(new_seq_id, coro, timeout_);
            if (ret == RPC_TIME_OUT)
                m_rpc_cache.erase(new_seq_id);
            return ret;
        }
        else
            return RPC_SYS_ERR;
    }

    return RPC_SUCCESS;
}

int32_t AngelService::async_rpc(uint64_t gid_, uint32_t cmd_, const google::protobuf::Message& req_,
                                google::protobuf::Message* rsp_, NextFun next_fun_, Context* context_, uint64_t dest_,
                                bool broadcast_, uint32_t timeout_)
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
    head.len = req_.ByteSizeLong();

    // 都从第一个channel发
    auto ret = send(0, head, req_, broadcast_);
    if (ret != RPC_SUCCESS)
        return ret;

    if (rsp_)
    {
        new_seq_id = m_context_ctrl->async_pending(
            new_seq_id,
            [=](int32_t ret_code) {
                if (ret_code == RPC_TIME_OUT)
                    m_rpc_cache.erase(new_seq_id);
                return next_fun_(ret_code);
            },
            context_, timeout_);
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

void AngelService::on_recv(uint32_t channel_index_, const AngelPkgHead& head_, const char* data_, size_t len_,
                           uint64_t src_)
{
    if (head_.msg_type == REQ_MSG)
    {
        deal_request(channel_index_, head_, data_, len_, src_);
    }
    else
    {
        deal_response(head_, data_, len_);
    }
}

void AngelService::deal_request(uint32_t channel_index_, const AngelPkgHead& head_, const char* data_, size_t len_,
                                uint64_t src_)
{
    deal_request_impl(channel_index_, head_, data_, len_, src_);
}

bool AngelService::deal_request_impl(uint32_t channel_index_, const AngelPkgHead& head_, const char* data_, size_t len_,
                                     uint64_t src_)
{
    auto iter = m_methods.find(head_.cmd);
    if (iter == m_methods.end())
        return false;

    if (iter->second.is_private)
        return false;

    auto& rpc_method = iter->second;
    std::unique_ptr<google::protobuf::Message> new_req(rpc_method.request->New());
    if (!new_req->ParsePartialFromArray(data_, len_))
        return false;

    std::unique_ptr<google::protobuf::Message> new_rsp(rpc_method.response->New());
    std::unique_ptr<AngelContext> context(new AngelContext(channel_index_, head_, new_req.get(), new_rsp.get()));
    if (m_context_ctrl->is_use_corotine())
    {
        if (!CoroObjMgr::instance().spawn([&]() { deal_method(context.get(), rpc_method.service, rpc_method.method); }))
            return false;
    }
    else
    {
        auto context_ptr = context.get();
        context->set_finish_fun([=]() { method_finish(context_ptr); });

        deal_method(context.get(), rpc_method.service, rpc_method.method);

        if (context->is_finish())
            method_finish(context.get());
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
    if (context_->is_finish())
        method_finish(context_.get());
}

void AngelService::method_finish(AngelContext* context_)
{
    if (!(context_->m_head.pkg_flag & FLAG_DONT_RSP))
    {
        context_->m_head.pkg_flag |= FLAG_DONT_RSP;
        context_->m_head.msg_type = RSP_PKG;
        send(context_->m_channel_index, context_->m_head, *(context_->m_rsp));
    }

    // 结束的时候释放资源
    delete context_->m_rsp;
    delete context_->m_req;
    delete context_;
}

int32_t AngelService::send(uint32_t channel_index_, const AngelPkgHead& head_, const google::protobuf::Message& msg_,
                           bool broadcast_)
{
    if (channel_index_ == 0 || channel_index_ > m_channels.size())
        return RPC_SYS_ERR;

    AngelPkgHead* send_head = reintrepret_cast<AngelPkgHead*>(m_send_buf);
    *send_head = head_;
    char* body = reinterpret_cast<char*>(send_head + 1);
    if (head_.len > SEND_BUF_LEN - sizeof(AngelPkgHead))
        return RPC_SYS_ERR;

    if (head_.len > 0)
    {
        if (!msg_.SerializeToArray(body, SEND_BUF_LEN - sizeof(AngelPkgHead)))
            return RPC_SERIALIZE_MSG_ERR;
    }

    if (broadcast_)
        return m_channels[channel_index_ - 1]->broadcast(m_send_buf, head_.len + sizeof(AngelPkgHead));
    else
        return m_channels[channel_index_ - 1]->send(m_send_buf, head_.len + sizeof(AngelPkgHead));
}

void AngelService::deal_response(const AngelPkgHead& head_, const char* data_, size_t len_)
{
    uint64_t seq_id = head_.seq_id;
    auto iter = m_rpc_cache.find(seq_id);
    if (iter == m_rpc_cache.end())
        return;

    auto cache = iter->second;
    assert(cache.rsp);
    m_rpc_cache.erase(iter);

    int32_t ret_code = RPC_SUCCESS;
    if (head_.gid != cache.gid || head_.cmd != cache.cmd)
        ret_code = RPC_SYS_ERR;
    else
    {
        ret_code = head_.ret;
        if (ret_code == RPC_SUCCESS && len_ > 0)
        {
            if (!cache.rsp->ParseFromArray(data_, len_))
                ret_code = RPC_PARSE_MSG_ERR;
        }
    }

    if (m_context_ctrl->is_use_corotine())
        m_context_ctrl->awake(seq_id, ret_code);
    else
        m_context_ctrl->async_awake(seq_id, ret_code);
}

}  // namespace pepper

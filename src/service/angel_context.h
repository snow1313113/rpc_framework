/*
 * * file name: angel_context.h
 * * description: ...
 * * author: snow
 * * create time:2020  7 08
 * */

#ifndef _ANGEL_CONTEXT_H_
#define _ANGEL_CONTEXT_H_

#include <functional>
#include <string>
#include "angel_pkg.h"
#include "base/context_controller.h"
#include "base/rpc_error.h"
#include "google/protobuf/message.h"
#include "google/protobuf/service.h"

namespace pepper
{
// 和angel service配套的上下文信息
struct AngelContext : public Context, public google::protobuf::RpcController
{
    AngelContext() = default;
    AngelContext(uint32_t channel_index_, const AngelPkgHead& head_, google::protobuf::Message* req_,
                 google::protobuf::Message* rsp_)
    {
        m_err_msg = "";
        m_channel_index = channel_index_;
        m_head = head_;
        m_req = req_;
        m_rsp = rsp_;
    }

    virtual void Reset() override
    {
        m_err_msg = "";
        m_channel_index = 0;
        std::memset(&m_head, 0, sizeof(m_head));
        m_req = nullptr;
        m_rsp = nullptr;
    }

    virtual bool Failed() const override { return m_ret_code != 0; }
    virtual std::string ErrorText() const override { return m_err_msg; }
    virtual void StartCancel() override {}

    virtual void SetFailed(const std::string& reason) override
    {
        m_ret_code = RPC_SYS_ERR;
        m_err_msg = reason;
    }
    virtual bool IsCanceled() const override { return false; }
    virtual void NotifyOnCancel(google::protobuf::Closure* callback) override {}

    std::string m_err_msg = "";
    uint32_t m_channel_index = 0;
    AngelPkgHead m_head;
    google::protobuf::Message* m_req = nullptr;
    google::protobuf::Message* m_rsp = nullptr;
};

}  // namespace pepper

#endif

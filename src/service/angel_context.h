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
struct AngelContext : public Context, public google::protobuf::RpcController
{
    AngelContext() = default;
    AngelContext(uint32_t index_, const AngelPkgHead& pkg_head_, google::protobuf::Message* request_,
                 google::protobuf::Message* response_)
    {
        err_msg = "";

        m_head.gid = pkg_head_.gid;
        m_head.seq_id = pkg_head_.seq_id;
        m_head.cmd = pkg_head_.cmd;
        m_head.pkg_flag = pkg_head_.pkg_flag;

        m_channel_index = index_;

        req = request_;
        rsp = response_;
    }

    virtual void Reset() override
    {
        m_err_msg = "";

        m_head.src = 0;
        m_head.dest = 0;
        m_head.gid = 0;
        m_head.seq_id = 0;
        m_head.cmd = 0;
        m_head.pkg_flag = 0;
        m_head.msg_type = 0;
        m_channel_index = 0;
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

    template <typename T>
    const T* Req() const
    {
        return ::google::protobuf::down_cast<T*>(req);
    }

    template <typename T>
    T* Rsp() const
    {
        return ::google::protobuf::down_cast<T*>(rsp);
    }

    std::string m_err_msg = "";
    AngelPkgHead m_head;
    uint32_t m_channel_index = 0;

protected:
    friend class AngelService;
    // 请求包结构
    google::protobuf::Message* req = nullptr;
    // 回包结构
    google::protobuf::Message* rsp = nullptr;
};

}  // namespace pepper

#endif

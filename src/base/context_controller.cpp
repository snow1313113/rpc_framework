/*
 * * file name: context_controller.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  4 06
 * */

#include "context_controller.h"
#include "coroutine_mgr.h"
#include "rpc_error.h"

namespace pepper
{
bool ContextController::init(bool use_coro_)
{
    if (!m_init)
    {
        m_use_corotine = use_coro_;
        m_base_seq_id = 0;
        m_init = true;
    }
    return true;
}

uint32_t ContextController::process_timeout(uint64_t now_)
{
    return m_timer_mgr.timeout(now_);
}

int32_t ContextController::pending(uint64_t seq_id_, CoroObj* coro_obj_, uint64_t expire_time_)
{
    if (!m_use_corotine || !coro_obj_)
        return RPC_SYS_ERR;

    assert(coro_obj_);

    if (seq_id_ == 0)
        seq_id_ = generate_seq_id();

    uint32_t timer_id = m_timer_mgr.add([=]() { awake(seq_id_, RPC_TIME_OUT); }, expire_time_);
    if (timer_id == 0)
        return RPC_SYS_ERR;

    Context context;
    context.coro_obj = coro_obj_;
    context.timer_id = timer_id;
    auto result = m_context_cache.insert({seq_id_, &context});
    if (result.second)
        coro_obj_->yield();
    else
        return RPC_SYS_ERR;

    return context.ret_code;
}

void ContextController::awake(uint64_t seq_id_, int32_t ret_code_)
{
    auto iter = m_context_cache.find(seq_id_);
    if (iter == m_context_cache.end())
        return;

    auto context = iter->second;
    assert(context);
    m_context_cache.erase(iter);

    if (ret_code_ != RPC_TIME_OUT)
        m_timer_mgr.cancel(context->timer_id);

    context->ret_code = ret_code_;
    context->timer_id = 0;

    assert(context->coro_obj);
    // 唤醒协程
    context->coro_obj->resume();
}

uint64_t ContextController::async_pending(uint64_t seq_id_, Context::NextFun next_fun_, Context* context_,
                                          uint64_t expire_time_)
{
    if (!next_fun_ || !context_)
        return 0;

    if (m_use_corotine)
        return 0;

    if (seq_id_ == 0)
        seq_id_ = generate_seq_id();

    uint32_t timer_id = m_timer_mgr.add([=]() { async_awake(seq_id_, RPC_TIME_OUT); }, expire_time_);
    if (timer_id == 0)
        return 0;

    context_->timer_id = timer_id;
    auto result = m_context_cache.insert({seq_id_, context_});
    if (result.second)
        context_->set_next(next_fun_);

    return seq_id_;
}

void ContextController::async_awake(uint64_t seq_id_, int32_t ret_code_)
{
    auto iter = m_context_cache.find(seq_id_);
    if (iter == m_context_cache.end())
        return;

    auto context = iter->second;
    m_context_cache.erase(iter);

    if (ret_code_ != RPC_TIME_OUT)
        m_timer_mgr.cancel(context->timer_id);

    context->ret_code = ret_code_;
    context->timer_id = 0;

    // 继续事务的流程
    context->run(context->ret_code);
}

uint64_t ContextController::generate_seq_id()
{
    return ++m_base_seq_id;
}

bool ContextController::is_use_corotine() const
{
    return m_use_corotine;
}

size_t ContextController::pending_context_num() const
{
    return m_context_cache.size();
}

}  // namespace pepper

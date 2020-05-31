/*
 * * file name: context_controller.h
 * * description: 上下文控制器，支持异步回调和协程两种上下文
 * * author: snow
 * * create time:2020  4 06
 * */

#ifndef _CONTEXT_CONTROLLER_H_
#define _CONTEXT_CONTROLLER_H_

#include <cassert>
#include <functional>
#include <unordered_map>
#include "util/timer_mgr.h"

namespace pepper
{
class CoroObj;
// 上下文对象，可以实现Context的子类来带上更多自己的信息
struct Context
{
    /// 返回error code, 传入的也是一个error code，如果是超时回调，则传入RPC_TIME_OUT
    using NextFun = std::function<int32_t(int32_t)>;
    /// 流程结束的时候需要回调的函数
    using FinishFun = std::function<void()>;

    int32_t m_ret_code = 0;
    void set_finish_fun(const FinishFun& fun_) { m_finish_fun = fun_; }
    void set_finish_fun(FinishFun&& fun_) { m_finish_fun = std::move(fun_); }
    bool is_finish() const { return m_ret_code || !m_next_fun; }

private:
    friend class ContextController;
    void run(int32_t ret_code_)
    {
        assert(m_next_fun);
        auto tmp_next_fun = m_next_fun;
        m_next_fun = nullptr;
        m_ret_code = tmp_next_fun(ret_code_);
        if (is_finish())
        {
            m_finish_fun();
        }
    }
    void set_next(const NextFun& fun_) { m_next_fun = fun_; }
    void set_next(NextFun&& fun_) { m_next_fun = std::move(fun_); }

private:
    uint32_t m_timer_id = 0;
    CoroObj* m_coro_obj = nullptr;
    NextFun m_next_fun = nullptr;
    FinishFun m_finish_fun = nullptr;
};

class ContextController
{
public:
    /// pending超时回调
    using PendingTimeoutFunc = std::function<void(uint64_t)>;

    /// use_coro表示是否使用协程，不然就是异步的
    bool init(bool use_coro_);
    /// 处理定时器，传入当前时间
    uint32_t process_timeout(uint64_t now_);
    /// 挂起当前协程事务，seq_id_可以generate_seq_id来生成，如果传0则内部自己生成一个
    int32_t pending(uint64_t seq_id_, uint64_t expire_time_, const PendingTimeoutFunc& func_ = nullptr);
    /// 挂起异步事务，seq_id_可以generate_seq_id来生成，如果传0则内部自己生成一个，返回最终使用的seq_id_
    uint64_t async_pending(uint64_t seq_id_, const Context::NextFun& next_fun_, Context* context_,
                           uint64_t expire_time_, const PendingTimeoutFunc& func_ = nullptr);
    /// 唤醒之前的上下文
    void awake(uint64_t seq_id_, int32_t ret_code_);
    /// 产生一个唯一ID
    uint64_t generate_seq_id();
    /// 当前是否选择了用协程
    bool is_use_corotine() const;
    /// 当前挂起的上下文数量
    size_t pending_context_num() const;

private:
    /// 超时队列
    TimerMgr m_timer_mgr;
    /// 挂起的上下文cache
    std::unordered_map<uint64_t, Context*> m_context_cache;
    /// seq_id计数器
    uint64_t m_base_seq_id = 0;
    /// 是否初始化的标记
    bool m_init = false;
    /// 是否使用协程
    bool m_use_corotine = false;
};

}  // namespace pepper

#endif

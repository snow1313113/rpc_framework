/*
 * * file name: timer_mgr.h
 * * description: 用set和map实现的定时器，简单直接
 * * author: snow
 * * create time:2020  4 07
 * */

#ifndef _TIMER_MGR_H_
#define _TIMER_MGR_H_

#include <functional>
#include <set>
#include <unordered_map>

namespace pepper
{
class TimerMgr
{
public:
    using Task = std::function<void()>;

    /// 添加定时器，返回一个唯一的定时器ID，interval_time如果是0则表示只执行一次
    uint32_t add(const Task& task, uint64_t expire_time, uint32_t interval_time = 0);
    uint32_t add(Task&& task, uint64_t expire_time, uint32_t interval_time = 0);
    /// 取消定时器
    bool cancel(uint32_t time_id);
    /// 处理所有比now超时的timer，返回处理了多少个
    uint32_t timeout(uint64_t now);

private:
    uint32_t gen_id();

public:
    struct Timer
    {
        /// 递增的唯一序号
        uint32_t seq_id;
        /// 循环重复定时器间隔时间ms
        uint32_t interval_time;
        /// 到期时间ms
        uint64_t expire_time;
        /// 要执行的任务
        Task task;

        friend bool operator<(const Timer& left, const Timer& right)
        {
            if (left.expire_time < right.expire_time)
                return true;
            else if (left.expire_time > right.expire_time)
                return false;
            else if (left.seq_id < right.seq_id)
                return true;
            else
                return false;
        }
    };

private:
    // timer集合，根据expire_time和seq id来排序
    std::set<Timer> m_timers;
    // seq id到expire_time的映射
    std::unordered_map<uint32_t, uint64_t> m_id_map;
    // time id 累加器
    uint32_t m_base_id = 0;
};

}  // namespace pepper

#endif

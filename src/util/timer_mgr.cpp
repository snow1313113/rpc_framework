/*
 * * file name: timer_mgr.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  4 07
 * */

#include "timer_mgr.h"
#include <cassert>

namespace pepper
{
uint32_t TimerMgr::add(const Task& task, uint64_t expire_time, uint32_t interval_time)
{
    uint32_t new_id = gen_id();
    if (!m_id_map.insert({new_id, expire_time}).second)
        return 0;

    if (!m_timers.insert({new_id, interval_time, expire_time, task}).second)
        return 0;

    return new_id;
}

uint32_t TimerMgr::add(Task&& task, uint64_t expire_time, uint32_t interval_time)
{
    uint32_t new_id = gen_id();
    if (!m_id_map.insert({new_id, expire_time}).second)
        return 0;

    if (!m_timers.insert({new_id, interval_time, expire_time, std::move(task)}).second)
        return 0;

    return new_id;
}

bool TimerMgr::cancel(uint32_t time_id)
{
    auto iter = m_id_map.find(time_id);
    if (iter == m_id_map.end())
        return false;

    m_timers.erase({time_id, 0, iter->second, nullptr});
    m_id_map.erase(iter);
    return true;
}

uint32_t TimerMgr::timeout(uint64_t now)
{
    uint32_t count = 0;
    while (!m_timers.empty())
    {
        auto iter = m_timers.begin();
        if (iter->expire_time > now)
            break;

        // 拷贝一份出来后先删除定时器，防止task中有逻辑对这个定时器有操作
        Timer tmp = (*iter);
        m_id_map.erase(iter->seq_id);
        m_timers.erase(iter);

        tmp.task();
        ++count;

        // 是循环定时器，重新加进去
        if (tmp.interval_time > 0)
        {
            tmp.expire_time += tmp.interval_time;
            auto result = m_id_map.insert({tmp.seq_id, tmp.expire_time});
            assert(result.second);
            auto result2 = m_timers.insert(tmp);
            assert(result2.second);
        }
    };

    return count;
}

uint32_t TimerMgr::gen_id()
{
    ++m_base_id;
    if (m_base_id == 0)
        ++m_base_id;
    return ++m_base_id;
}

}  // namespace pepper

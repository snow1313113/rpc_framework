/*
 * * file name: coroutine_mgr.h
 * * description: 私有栈协程，对底层api做的封装，可以考虑支持根据宏来切换用不同的coroutine库
 * * author: snow
 * * create time:2020  4 06
 * */

#ifndef _COROUTINE_MGR_H_
#define _COROUTINE_MGR_H_

#include <functional>
#include "coctx.h"
#include "singleton.h"

namespace pepper
{
using CoroFunc = std::function<void()>;

class CoroObj;
class CoroObjMgr : public Singleton<CoroObjMgr>
{
public:
    /// 设置协程栈大小
    void set_stack_size(size_t size_);
    /// 获取协程栈大小
    size_t stack_size() const;
    /// 设置是否进行内存段保护，会增加额外内存
    void set_mem_protect(bool protect_);
    /// 是否设置内存保护
    bool mem_protect() const;
    /// 获取当前在跑的协程对象数量
    size_t running_coro_num() const;
    /// 获取当前所有的协程对象数量
    size_t total_coro_num() const;

    /// 启动一个协程，协程对象的初始化可能失败，失败则没有切换到协程
    bool spawn(const CoroFunc &task_);
    bool spawn(CoroFunc &&task_);

    /// 如果当前是在协程中，则返回当前协程的CoroObj指针，不然则返回nullptr
    CoroObj *cur_coro();

private:
    friend struct Singleton<CoroObjMgr>;
    CoroObjMgr(const CoroObjMgr &) = delete;
    CoroObjMgr(CoroObjMgr &&) = delete;
    CoroObjMgr &operator=(const CoroObjMgr &) = delete;
    CoroObjMgr();
    ~CoroObjMgr();

    CoroObj *allocate();
    void free(CoroObj *coro_obj_);
    static void *run_loop(void *arg_, void *arg2_);

private:
    size_t m_stack_size;
    bool m_need_protect;
};

/// 协程对象，只提供两个接口
class CoroObj
{
public:
    /// 唤醒当前协程
    void resume();
    /// 切回主协程
    void yield();

private:
    friend class CoroObjMgr;
    CoroObj() = default;
    CoroObj(const CoroObj &) = delete;
    CoroObj(CoroObj &&) = delete;
    CoroObj &operator=(const CoroObj &) = delete;
    ~CoroObj();
    bool init(size_t stack_size, bool need_protect, coctx_pfn_t f);

private:
    struct coctx_t m_context;
    char *m_stack = nullptr;
    size_t m_stack_size = 0;
    CoroFunc m_task = nullptr;
};

}  // namespace pepper

#endif

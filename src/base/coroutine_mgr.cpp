/*
 * * file name: coroutine_mgr.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  4 06
 * */

#include "coroutine_mgr.h"
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <list>
#include <memory>
#include <set>

extern "C"
{
    extern void coctx_swap(coctx_t *, coctx_t *) asm("coctx_swap");
};

namespace pepper
{
static std::size_t sys_page_size()
{
    static std::size_t size = ::sysconf(_SC_PAGESIZE);
    return size;
}

// 不同线程各自一份协程管理数据，不过我们目前的应用场景不会有多线程的情况
struct ThreadLocalData
{
    struct coctx_t main_context;
    CoroObj *current;
    std::list<CoroObj *> free_coros;
    std::set<CoroObj *> used_coros;
};

// thread_local修饰的对象不能放在类中，所以只能放这里了
static thread_local ThreadLocalData *thread_local_data = nullptr;

CoroObjMgr::CoroObjMgr() : m_stack_size(64 * 1024), m_need_protect(true)
{
    if (!thread_local_data)
    {
        thread_local_data = new ThreadLocalData;
        thread_local_data->current = nullptr;
    }
}

CoroObjMgr::~CoroObjMgr()
{
    if (thread_local_data)
    {
        std::unique_ptr<ThreadLocalData> ptr(thread_local_data);
        thread_local_data = nullptr;
        std::for_each(ptr->free_coros.begin(), ptr->free_coros.end(), [](const auto &temp) { delete temp; });
        std::for_each(ptr->used_coros.begin(), ptr->used_coros.end(), [](const auto &temp) { delete temp; });
    }
}

void CoroObjMgr::set_stack_size(size_t size_)
{
    m_stack_size = size_;
}

size_t CoroObjMgr::stack_size() const
{
    return m_stack_size;
}

void CoroObjMgr::set_mem_protect(bool protect_)
{
    m_need_protect = protect_;
}

bool CoroObjMgr::mem_protect() const
{
    return m_need_protect;
}

size_t CoroObjMgr::running_coro_num() const
{
    return thread_local_data->used_coros.size();
}

size_t CoroObjMgr::total_coro_num() const
{
    return thread_local_data->used_coros.size() + thread_local_data->free_coros.size();
}

bool CoroObjMgr::spawn(const CoroFunc &task_)
{
    // 只允许在主协程中起新协程
    assert(thread_local_data->current == nullptr);
    assert(task_);
    auto coro = allocate();
    if (!coro)
        return false;

    coro->m_task = task_;
    coro->resume();
    return true;
}

bool CoroObjMgr::spawn(CoroFunc &&task_)
{
    // 只允许在主协程中起新协程
    assert(thread_local_data->current == nullptr);
    assert(task_);
    auto coro = allocate();
    if (!coro)
        return false;

    coro->m_task = std::move(task_);
    coro->resume();
    return true;
}

CoroObj *CoroObjMgr::cur_coro()
{
    return thread_local_data->current;
}

CoroObj *CoroObjMgr::allocate()
{
    CoroObj *coro = nullptr;
    if (thread_local_data->free_coros.empty())
    {
        coro = new CoroObj;
        if (coro->init(m_stack_size, m_need_protect, CoroObjMgr::run_loop) == false)
        {
            delete coro;
            return nullptr;
        }
    }
    else
    {
        coro = thread_local_data->free_coros.front();
        thread_local_data->free_coros.pop_front();
    }
    thread_local_data->used_coros.insert(coro);
    return coro;
}

void CoroObjMgr::free(CoroObj *coro_obj_)
{
    if (thread_local_data->used_coros.erase(coro_obj_) > 0)
        thread_local_data->free_coros.push_front(coro_obj_);
}

void *CoroObjMgr::run_loop(void *arg_, void *arg2_)
{
    auto coro = reinterpret_cast<CoroObj *>(arg_);
    while (true)
    {
        coro->m_task();
        coro->m_task = nullptr;
        CoroObjMgr::instance().free(coro);
        coro->yield();
    }
    return nullptr;
}

CoroObj::~CoroObj()
{
    if (m_stack)
        munmap(m_stack, m_stack_size);
}

bool CoroObj::init(size_t stack_size, bool need_protect, coctx_pfn_t f)
{
    size_t page_size = sys_page_size();
    size_t total_size = (((stack_size + page_size - 1) / page_size) + (need_protect ? 2 : 0)) * page_size;
    auto mem = mmap(0, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
        return false;

    m_stack = reinterpret_cast<char *>(mem);
    m_stack_size = total_size;
    if (need_protect)
    {
        // 需要内存屏障，所以把头尾各截出一个页大小作为屏障
        mprotect(m_stack, page_size, PROT_NONE);
        mprotect(m_stack + m_stack_size - page_size, page_size, PROT_NONE);
    }

    coctx_init(&m_context);
    m_context.ss_sp = m_stack + (need_protect ? page_size : 0);
    m_context.ss_size = m_stack_size - (need_protect ? 2 * page_size : 0);
    coctx_make(&m_context, f, this, 0);
    return true;
}

void CoroObj::resume()
{
    assert(thread_local_data->current == nullptr);
    thread_local_data->current = this;
    // 切入到协程
    coctx_swap(&(thread_local_data->main_context), &m_context);
}

void CoroObj::yield()
{
    assert(thread_local_data->current);
    thread_local_data->current = nullptr;
    // 切回主协程
    coctx_swap(&m_context, &(thread_local_data->main_context));
}

}  // namespace pepper

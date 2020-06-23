/*
 * * file name: base_svr.h
 * * description: ...
 * * author: snow
 * * create time:2020  4 25
 * */

#ifndef _BASE_SVR_H_
#define _BASE_SVR_H_

#include "base/context_controller.h"

namespace pepper
{
/// Policy-based design去实现基础server，需要不同的service就当作模版参数传进来组合吧
template <typename... TServices>
class BaseSvr : public TServices...
{
public:
    bool init(bool use_coro_)
    {
        if (!m_context_ctrl.init(user_coro_))
            return false;

        (TServices::set_context_ctrl(&m_context_ctrl), ...);
        return 0;
    }

    /// 定时处理service的update，返回一共处理了多少事件
    size_t update()
    {
        size_t total_count = 0;
        (total_count += TServices::loop_once(), ...);
        return total_count;
    }

    virtual ~BaseSvr() = default;

protected:
    ContextController m_context_ctrl;
};

}  // namespace pepper

#endif

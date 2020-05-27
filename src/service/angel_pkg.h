/*
 * * file name: angel_pkg.h
 * * description: ...
 * * author: snow
 * * create time:2020  4 29
 * */

#ifndef _ANGEL_PKG_H_
#define _ANGEL_PKG_H_

namespace pepper
{
struct AngelPkgHead
{
    enum MSG_TYPE : uint8_t
    {
        REQ_MSG = 0,
        RSP_MSG = 1,
    };

    enum FLAG : uint8_t
    {
        FLAG_NULL = 0x0,
        FLAG_DONT_RSP = 0x1,
    };

    uint64_t gid = 0;
    uint64_t seq_id = 0;
    uint32_t cmd = 0;
    uint16_t ret = 0;
    uint8_t pkg_flag = 0;
    MSG_TYPE msg_type = REQ_MSG;
    uint32_t len = 0;
};
}  // namespace pepper

#endif

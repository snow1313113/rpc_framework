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
    uint64_t gid = 0;
    uint64_t seq_id = 0;
    uint32_t cmd = 0;
    uint16_t ret = 0;
    uint8_t pkg_flag = 0;
    uint8_t msg_type = 0;
    uint32_t len = 0;
    uint32_t magic = 0;
};
}  // namespace pepper

#endif

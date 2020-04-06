/*
 * * file name: singleton.h
 * * description: 没有什么特别的，就是经典的Meyers’ Singleton
 * * author: snow
 * * create time:2020  4 01
 * */

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

namespace pepper
{
template <typename T>
struct Singleton
{
    static T &instance()
    {
        static T only;
        return only;
    }

protected:
    ~Singleton() {}
};

}  // namespace pepper

#endif

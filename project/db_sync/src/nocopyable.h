/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: nocopyable.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.1.13   1.0     Create
******************************************************************************/


#ifndef __NOCOPYABLE_H__
#define __NOCOPYABLE_H__

namespace db_sync
{
    class nocopyable
    {
    protected:
        nocopyable(){}
        virtual ~nocopyable(){}
    private:
        nocopyable(const nocopyable&);
        nocopyable& operator = (const nocopyable&);
    };
}
#endif


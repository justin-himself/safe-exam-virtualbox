/** @file
 * VirtualPox ThreadTask class definition
 */

/*
 * Copyright (C) 2015-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_ThreadTask_h
#define MAIN_INCLUDED_ThreadTask_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "VPox/com/string.h"

/**
 * The class ThreadVoidData is used as a base class for any data which we want to pass into a thread
 */
struct ThreadVoidData
{
public:
    ThreadVoidData() { }
    virtual ~ThreadVoidData() { }
};


class ThreadTask
{
public:
    ThreadTask(const Utf8Str &t)
        : m_strTaskName(t)
        , mAsync(false)
    { }

    virtual ~ThreadTask()
    { }

    HRESULT createThread(void);
    HRESULT createThreadWithType(RTTHREADTYPE enmType);

    inline Utf8Str getTaskName() const { return m_strTaskName; }
    bool isAsync() { return mAsync; }

protected:
    HRESULT createThreadInternal(RTTHREADTYPE enmType);
    static DECLCALLBACK(int) taskHandlerThreadProc(RTTHREAD thread, void *pvUser);

    ThreadTask() : m_strTaskName("GenericTask")
    { }

    Utf8Str m_strTaskName;
    bool mAsync;

private:
    virtual void handler() = 0;
};

#endif /* !MAIN_INCLUDED_ThreadTask_h */


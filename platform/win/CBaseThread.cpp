/*
 * Copyright (C) 2015   Jeremy Chen jeremy_cz@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <fdbus/CBaseThread.h>

namespace ipc {
namespace fdbus {
CBaseThread::CBaseThread(const char* thread_name)
    : mThread(0)
    , mPriority(THREAD_PRIORITY_NORMAL)
    , mThreadName(thread_name ? thread_name : "")
{
    mInvalidTid = GetCurrentThreadId();
    mThreadId = mInvalidTid;
}

CBaseThread::~CBaseThread()
{
    //wait();
}

bool CBaseThread::start(uint32_t flag)
{
    if (started())
    {
        return false;
    }

    if (flag & FDB_WORKER_EXE_IN_PLACE)
    {
        {
            std::lock_guard<std::mutex> _l(mMutex);
            if (started())
            {
                return false;
            }
            mThreadId = GetCurrentThreadId();
        }
        threadFunc(this);
        return true;
    }
    else
    {
        std::lock_guard<std::mutex> _l(mMutex);
        if (started())
        {
            return true;
        }
        mThread = reinterpret_cast<CBASE_tThreadHnd>(_beginthreadex(0, 0,
                  CBaseThread::threadFunc,
                  this, CREATE_SUSPENDED, 0));
        if ( 0 == mThread )
        {
            return false;
        }
        else
        {
            mThreadId = GetThreadId(mThread);
            applyPriority(mPriority);
            if ( static_cast<DWORD>(-1) == ResumeThread(mThread) )
            {
                return false;
            }
        }
    }
    return true;
}

bool CBaseThread::started()
{
    return mThreadId != mInvalidTid;
}

bool CBaseThread::join(uint32_t milliseconds)
{
    if (isSelf())
    {
        return false;
    }
    int32_t rc = ERROR_SUCCESS;
    DWORD status(0);

    status = WaitForSingleObject(mThread, INFINITE);

    if ( WAIT_OBJECT_0 != status )
    {
        rc = status;
        if ( WAIT_FAILED == status )
        {
            rc = GetLastError();
        }
    }

    CloseHandle(mThread);
    mThread = 0;

    return !rc;
}

bool CBaseThread::isSelf() const
{
    return GetCurrentThreadId() == mThreadId;
}

bool CBaseThread::priority(int32_t level)
{
    bool status(false);

    switch ( level )
    {
        case THREAD_PRIORITY_ABOVE_NORMAL:
        case THREAD_PRIORITY_BELOW_NORMAL:
        case THREAD_PRIORITY_HIGHEST:
        case THREAD_PRIORITY_IDLE:
        case THREAD_PRIORITY_LOWEST:
        case THREAD_PRIORITY_NORMAL:
        case THREAD_PRIORITY_TIME_CRITICAL:
            mPriority = level;
            status = applyPriority(mPriority);
            break;

        default:
            break;
    }

    return status;
}

bool CBaseThread::applyPriority(int32_t level)
{
    return (0 != SetThreadPriority(mThread, level));
}

bool CBaseThread::name(const char* thread_name)
{
    if (thread_name)
    {
        mThreadName = thread_name;
    }
    return true;
}

unsigned int __stdcall CBaseThread::threadFunc
(
    void *d
)
{
    CBaseThread *self = static_cast<CBaseThread *>(d);

    while (!self->isSelf())
    {   // Thread might be started before mThreadId is updated at start().
	// In this case isSelf() might return with false even in self
	// thread. Wait until isSelf() become valid.
        sysdep_sleep(FDB_WAIT_SELF_READY);
    }

    self->run();
    self->mThreadId = self->mInvalidTid;
    return 0;
}

CBASE_tProcId CBaseThread::getPid()
{
    return GetCurrentThreadId();
}
}
}


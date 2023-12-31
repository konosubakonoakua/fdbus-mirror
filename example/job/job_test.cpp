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

#define FDB_LOG_TAG "JOB_TEST"
#include <fdbus/fdbus.h>
#include <iostream>

using namespace ipc::fdbus;
static CBaseWorker sender("peer1");
static CBaseWorker receiver("peer2");

/* create a job which print message at worker thread */
class CGreetingJob : public CBaseJob
{
public:
    CGreetingJob(const char *greeting_word)
    {
        mGreetingWord = greeting_word;
    }
protected:
    /* callback called at worker thread */
    void run(Ptr &ref)
    {
        std::cout << mGreetingWord << std::endl;
        FDB_LOG_I("Job is received at %s.\n", ref->worker()->name().c_str());
    }
private:
    std::string mGreetingWord;
};

/* a timer sending job at 200ms interval cyclically */
class CSenderTimer : public CBaseLoopTimer
{
public:
    CSenderTimer(CBaseWorker &receiver, int32_t interval, bool sync, bool urgent)
        : CBaseLoopTimer(interval, true)
        , mReceiver(receiver)
        , mSync(sync)
        , mUrgent(urgent)
    {}
protected:
    /* called when timeout */
    void run()
    {
        /* send job to receiver worker thread asynchronously
         * it just throws the job to receiver and will not block
         */
        if (mSync)
        {
            mReceiver.sendSync(new CGreetingJob("sync hello, world!"), 0, mUrgent);
            const char *data = "lambda sync hello, world!";
            mReceiver.sendSync([data](CBaseJob::Ptr &)
                    {
                        printf("%s\n", data);
                    }, 0, mUrgent);
        }
        else
        {
            mReceiver.sendAsync(new CGreetingJob("async hello, world!"), mUrgent);
            const char *data = "lambda async hello, world!";
            mReceiver.sendAsync([data](CBaseJob::Ptr &)
                    {
                        printf("%s\n", data);
                    }, mUrgent);
        }
        FDB_LOG_I("Job is sent.\n");
    }
private:
    CBaseWorker &mReceiver;
    bool mSync;
    bool mUrgent;
};

class CTransientTimer : public CBaseLoopTimer
{
public:
    CTransientTimer()
        : CBaseLoopTimer(5, true)
    {}
protected:
    void run()
    {
        std::cout << "transient timer running" << std::endl;
    }
};

int main(int argc, char **argv)
{
    /* start fdbus context thread */
    FDB_CONTEXT->start();
    sender.start(); /* start sender worker thread */
    receiver.start(); /* start receiver worker thread */

    for (int i = 0; i < 20; ++i)
    {
        /*
         * create sender timer and attach it with sender worker thread
         * job is sent from sender to receiver
         */
        auto sender_timer = new CSenderTimer(receiver, 80 + (i << 1), true, !!(i & 1));
        sender_timer->attach(&sender, true);
    }

    {
    /*
     * create sender timer and attach it with receiver worker thread
     * job is sent from receiver to sender 
     */
    auto sender_timer = new CSenderTimer(sender, 100, false, false);
    sender_timer->attach(&receiver, true);
    }

    CBaseWorker worker_for_transient_timer;
    worker_for_transient_timer.start();
    while (1)
    {
        auto transient_timer = new CTransientTimer();
        transient_timer->attach(&worker_for_transient_timer, true);
        sysdep_sleep(5);
        delete transient_timer;
        std::cout << "timer is destroyed." << std::endl;
    }

    /* convert main thread into worker */
    CBaseWorker background_worker;
    background_worker.start(FDB_WORKER_EXE_IN_PLACE);
}

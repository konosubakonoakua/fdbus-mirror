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

#ifndef _CNSCONFIG_H_
#define _CNSCONFIG_H_

#include <stdint.h>
#include <fdbus/common_defs.h>
#include <list>

namespace ipc {
namespace fdbus {
typedef std::list<std::string> tPendingServiceReqTbl;

class CNsConfig
{
public:
#if !defined(FDB_CFG_SOCKET_PATH)
#define FDB_CFG_SOCKET_PATH "/tmp"
#endif

#define NS_CFG_NR_HB_RETRIES            5
#define NS_CFG_HB_INTERVAL              1000
#define NS_CFG_HB_TIMEOUT               (NS_CFG_NR_HB_RETRIES * NS_CFG_HB_INTERVAL)
#define NS_CFG_HS_RECONNECT_INTERVAL    1500
#define NS_CFG_NS_RECONNECT_INTERVAL    500
#define NS_CFG_CHECK_IP_INTERVAL        500
#define NS_CFG_ADDRESS_BIND_RETRY_CNT   5
#ifdef FDB_CONFIG_UDS_ABSTRACT
    #define NS_CFG_UDS_ADDRESS_PREFIX  "@"
#else
    #define NS_CFG_UDS_ADDRESS_PREFIX  ""
#endif

    static const char *getHostServerName()
    {
        return FDB_HOST_SERVER_NAME;
    }

    static const char *getNameServerName()
    {
        return FDB_NAME_SERVER_NAME;
    }

    static const char *getNameServerIPCPath()
    {
        return NS_CFG_UDS_ADDRESS_PREFIX FDB_CFG_SOCKET_PATH "/" "fdb-ns";
    }

    static const char *getNameServerIPCUrl()
    {
        return FDB_URL_IPC NS_CFG_UDS_ADDRESS_PREFIX FDB_CFG_SOCKET_PATH "/" "fdb-ns";
    }

    static const char *getHostServerIPCPath()
    {
        return NS_CFG_UDS_ADDRESS_PREFIX FDB_CFG_SOCKET_PATH "/" "fdb-hs";
    }

    static const char *getHostServerIPCUrl()
    {
        return FDB_URL_IPC NS_CFG_UDS_ADDRESS_PREFIX FDB_CFG_SOCKET_PATH "/" "fdb-hs";
    }

    static const char *getNameServerTCPUrl(bool secure)
    {
        return secure ? FDB_URL_TCPS FDB_LOCAL_HOST ":60003" :
                        FDB_URL_TCP FDB_LOCAL_HOST ":60002";
    }

    static const char *getNameServerTCPPort(bool secure)
    {
        return secure ? "60003" : "60002";
    }

    static int32_t getIntNameServerTCPPort(bool secure)
    {
        return secure ? 60003 : 60002;
    }

    static const char *getHostServerTCPPort(bool secure)
    {
        return secure ? "60001" : "60000";
    }

    static int32_t getIntHostServerTCPPort(bool secure)
    {
        return secure ? 60001 : 60000;
    }

    static const char *getIPCPathBase()
    {
        return NS_CFG_UDS_ADDRESS_PREFIX FDB_CFG_SOCKET_PATH "/" "fdb-ipc";
    }

    static const char *getIPCUrlBase()
    {
        return FDB_URL_IPC NS_CFG_UDS_ADDRESS_PREFIX FDB_CFG_SOCKET_PATH "/" "fdb-ipc";
    }

    static int32_t getTCPPortMin()
    {
        return 60005;
    }

    static int32_t getTCPPortMax()
    {
        return 65000;
    }

    /* Number of un-acknowledged heartbeats before lose is detected */
    static int32_t getHeartBeatRetryNr()
    {
        return NS_CFG_NR_HB_RETRIES;
    }

    /* How long heartbeat is regarded as lost if un-acknowledged */
    static int32_t getHeartBeatTimeout()
    {
        return NS_CFG_HB_TIMEOUT + 500;
    }

    /* Interval between each heartbeat */
    static int32_t getHeartBeatInterval()
    {
        return NS_CFG_HB_INTERVAL;
    }

    static int32_t getHsReconnectInterval()
    {
        return NS_CFG_HS_RECONNECT_INTERVAL;
    }

    static int32_t getCheckIpInterval()
    {
        return NS_CFG_CHECK_IP_INTERVAL;
    }

    static int32_t getNsReconnectInterval()
    {
        return NS_CFG_NS_RECONNECT_INTERVAL;
    }

    static int32_t getAddressBindRetryCnt()
    {
        return NS_CFG_ADDRESS_BIND_RETRY_CNT;
    }
};
}
}
#endif


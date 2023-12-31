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

#include <iostream>
#include <fdbus/fdb_option_parser.h>
#include <fdbus/CBaseServer.h>
#include <fdbus/CFdbContext.h>

using namespace ipc::fdbus;

class CNotificationCenter : public CBaseServer
{
public:
    CNotificationCenter(const char *name, char **peer_array, uint32_t num_peers,
                        int32_t self_exportable_level)
        : CBaseServer(name ? name : FDB_NOTIFICATION_CENTER_NAME)
    {
        enableEventCache(true);
        enableEventRoute(true);
        enableUDP(true);
        setExportableLevel((self_exportable_level < 0) ? FDB_EXPORTABLE_DOMAIN : self_exportable_level);
        for (uint32_t i = 0; i < num_peers; ++i)
        {
            addPeerRouter(peer_array[i]);
        }
    }
};

int main(int argc, char **argv)
{
    int32_t help = 0;
    char *server_name = 0;
    char *peers = 0;
    int32_t self_exportable_level = -1;
    const struct fdb_option core_options[] = {
            { FDB_OPTION_STRING, "server_name", 'n', &server_name },
            { FDB_OPTION_STRING, "peers", 'p', &peers },
            { FDB_OPTION_INTEGER, "self_exportable_level", 'l', &self_exportable_level },
            { FDB_OPTION_BOOLEAN, "help", 'h', &help }
    };

    fdb_parse_options(core_options, ARRAY_LENGTH(core_options), &argc, argv);
    if (help)
    {
        std::cout << "FDBus - Fast Distributed Bus" << std::endl;
        std::cout << "    SDK version " << FDB_DEF_TO_STR(FDB_VERSION_MAJOR) "."
                                           FDB_DEF_TO_STR(FDB_VERSION_MINOR) "."
                                           FDB_DEF_TO_STR(FDB_VERSION_BUILD) << std::endl;
        std::cout << "    LIB version " << CFdbContext::getFdbLibVersion() << std::endl;
        std::cout << "Usage: ntfcenter[ -n service name][ -p peer_name_1[,peer_name_2][,...]][-l exportable level]" << std::endl;
        std::cout << "FDBus Notification Center" << std::endl;
        std::cout << "    -n: FDBus service name for notification center" << std::endl;
        std::cout << "    -p: Server name of connected notification centers, separated by ','" << std::endl;
        std::cout << "    -l self_level: specify exportable level of NC" << std::endl;
        return 0;
    }

    uint32_t num_peers = 0;
    char **peer_array = peers ? strsplit(peers, ",", &num_peers) : 0;

    CNotificationCenter nc(server_name, peer_array, num_peers, self_exportable_level);
    nc.bind();
    FDB_CONTEXT->start(FDB_WORKER_EXE_IN_PLACE);
    return 0;
}


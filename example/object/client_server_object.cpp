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
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#define FDB_LOG_TAG "FDBTEST"
#include <fdbus/fdbus.h>
#include FDB_IDL_EXAMPLE_H
#include <fdbus/CFdbProtoMsgBuilder.h>

using namespace ipc::fdbus;

#define OBJ_FROM_SERVER_TO_CLIENT   1
#define FDB_NUM_OF_CLIENT_OBJECT    8
#define FDB_SHUTDOWN_TEST           0

#if 1
CBaseNotificationCenter<void *> nc;
class myNotification : public CBaseNotification<void*>
{
protected:
    /*
     * run() will be executed once the specified event happens.
     */
    void run(void *& data)
    {
    }

};
myNotification nf;

class ntf_test
{
public:
    void process(CMethodNotification<void *, ntf_test> *ntf, void * &data)
    {
    }
};

ntf_test aaa;
CMethodNotification<void *, ntf_test> m_ntf(&aaa, &ntf_test::process);
CGenericMethodNotification<ntf_test> m_ntf1(&aaa, &ntf_test::process);
#endif

static CBaseWorker main_worker;
static CBaseWorker mediaplayer_worker("mediaplayer_worker");

enum EMessageId
{
    REQ_METADATA,
    REQ_RAWDATA,
    REQ_CREATE_MEDIAPLAYER,
    NTF_ELAPSE_TIME,
    NTF_MEDIAPLAYER_CREATED,
};

class CMyMessage : public CBaseMessage 
{
public:
    CMyMessage(FdbMsgCode_t code = FDB_INVALID_ID)
        : CBaseMessage(code)
    {
        memset(my_buf, 0, sizeof(my_buf));
    }
private:
    uint8_t my_buf[1024 * 1024];
};

void printMetadata(FdbObjectId_t obj_id, const CFdbMsgMetadata *metadata)
{
    uint64_t time_c2s;
    uint64_t time_s2r;
    uint64_t time_r2c;
    uint64_t time_total;
    CFdbMessage::parseTimestamp(metadata, time_c2s, time_s2r, time_r2c, time_total);
#ifdef __WIN32__
    FDB_LOG_I("OBJ %d, client->server: %llu, arrive->reply: %llu, reply->receive: %llu, total: %llu\n",
                obj_id, time_c2s, time_s2r, time_r2c, time_total);
#else
    FDB_LOG_I("OBJ %d, client->server: %lu, arrive->reply: %lu, reply->receive: %lu, total: %lu\n",
                obj_id, time_c2s, time_s2r, time_r2c, time_total);
#endif
}

template <typename T>
class CMyServer;
static std::vector<CMyServer<CFdbBaseObject> *> my_server_objects;
template <typename T>
class CBroadcastTimer : public CMethodLoopTimer<CMyServer<T> > 
{
public:
    CBroadcastTimer(CMyServer<T> *server)
        : CMethodLoopTimer<CMyServer<T> >(1500, true, server, &CMyServer<T>::broadcastElapseTime)
    {}
};

template <class T>
class CMyServer : public T
{
public:
    CBroadcastTimer<T> *mTimer;
    CMyServer(const char *name, CBaseWorker *worker = 0, CFdbBaseContext *context = 0)
        : T(name, worker, context)
    {
        mTimer = new CBroadcastTimer<T>(this);
        mTimer->attach(&main_worker, false, false);
    }
    ~CMyServer()
    {
        delete mTimer;
    }

    void broadcastElapseTime(CMethodLoopTimer<CMyServer<T> > *timer)
    {
        NFdbExample::ElapseTime et;
        et.set_hour(1);
        et.set_minute(10);
        et.set_second(35);
        CFdbProtoMsgBuilder builder(et);
        this->broadcast(NTF_ELAPSE_TIME, builder, "my_topic");
        char raw_data[1920];
        memset(raw_data, '=', sizeof(raw_data));
        raw_data[1919] = '\0';
        this->broadcast(NTF_ELAPSE_TIME, raw_data, 1920, "raw_buffer");
        this->broadcast(NTF_ELAPSE_TIME);
    }
    
protected:
    void onOnline(const CFdbOnlineInfo &info);
    void onOffline(const CFdbOnlineInfo &info)
    {
        FDB_LOG_I("OBJ %d server session shutdown: %d\n", this->objId(), info.mSid);
        if (info.mFirstOrLast)
        {
            mTimer->disable();
        }
    }

    void onInvoke(CBaseJob::Ptr &msg_ref)
    {
        auto msg = castToMessage<CBaseMessage *>(msg_ref);
        static int32_t elapse_time = 0;
        switch (msg->code())
        {
            case REQ_METADATA:
            {
                NFdbExample::SongId song_id;
                CFdbProtoMsgParser parser(song_id);
                if (!msg->deserialize(parser))
                {
                    msg->status(msg_ref, FDB_ST_MSG_DECODE_FAIL, "Fail to decode request!");
                    return;
                }

                FDB_LOG_I("OBJ %d requested song id is: %d\n", this->objId(), song_id.id());
                NFdbExample::NowPlayingDetails rep_msg;
#if 0
                rep_msg.set_artist("刘德华");
                rep_msg.set_album("十年华语金曲");
                rep_msg.set_genre("乡村");
                rep_msg.set_title("忘情水");
#else
                rep_msg.set_artist("Liu Dehua");
                rep_msg.set_album("Ten Year's Golden Song");
                rep_msg.set_genre("County");
                rep_msg.set_title("Wang Qing Shui");
#endif
                rep_msg.set_file_name("Lau Dewa");
                rep_msg.set_elapse_time(elapse_time++);
                CFdbProtoMsgBuilder builder(rep_msg);
                msg->reply(msg_ref, builder);
            }
            break;
            case REQ_RAWDATA:
            {
                // const char *buffer = (char *)msg->getPayloadBuffer();
                int32_t size = msg->getPayloadSize();
                FDB_LOG_I("OBJ %d Invoke of raw buffer is received: size: %d\n", this->objId(), size);
                msg->status(msg_ref, FDB_ST_OK, "REQ_RAWDATA is processed successfully!");
            }
            break;
            case REQ_CREATE_MEDIAPLAYER:
            {
                #if 0
                for (int32_t i = 0; i < 5; ++i)
                {
                    CMyServer<CFdbBaseObject> *obj = new CMyServer<CFdbBaseObject>("mediaplayer", &mediaplayer_worker);
                    FdbObjectId_t obj_id = obj->bind(dynamic_cast<CBaseEndpoint *>(this));
                    NFdbExample::FdbMsgObjectInfo obj_info;
                    obj_info.set_obj_id(obj_id);
                    this->broadcast(NTF_MEDIAPLAYER_CREATED, obj_info);
                    my_server_objects.push_back(obj);
                }
                #endif
            }
            break;
            default:
            break;
        }
    }

    void onSubscribe(CBaseJob::Ptr &msg_ref)
    {
        auto msg = castToMessage<CFdbMessage *>(msg_ref);
        const CFdbMsgSubscribeItem *sub_item;
        FDB_BEGIN_FOREACH_SIGNAL(msg, sub_item)
        {
            auto msg_code = sub_item->msg_code();
            const char *topic = "";
            if (sub_item->has_topic())
            {
                topic = sub_item->topic().c_str();
            }
            FdbSessionId_t sid = msg->session();

            FDB_LOG_I("OBJ %d message %d, topic %s of session %d is registered!\n", this->objId(), msg_code, topic, sid);

            switch (msg_code)
            {
                case NTF_ELAPSE_TIME:
                {
                    // Warning! topic might be NULL!
                    std::string str_topic(topic);
                    if (!str_topic.compare("my_topic"))
                    {
                        NFdbExample::ElapseTime et;
                        et.set_hour(1);
                        et.set_minute(10);
                        et.set_second(35);
                        CFdbProtoMsgBuilder builder(et);
                        msg->broadcast(msg_code, builder, topic);
                    }
                    else if (!str_topic.compare("raw_buffer"))
                    {
                        std::string raw_data = "raw buffer test for broadcast.";
                        msg->broadcast(NTF_ELAPSE_TIME, raw_data.c_str(), (int32_t)raw_data.length() + 1, "raw_buffer");
                    }
                }
                break;
                default:
                break;
            }

        }
        FDB_END_FOREACH_SIGNAL()
    }

    void onCreateObject(CBaseEndpoint *endpoint, CFdbMessage *msg)
    {
        FDB_LOG_I("Create object %d because message %d is received.\n", msg->objectId(), msg->code());
        auto obj = new CMyServer<CFdbBaseObject>("mediaplayer", &mediaplayer_worker);
        obj->bind(dynamic_cast<CBaseEndpoint *>(this), msg->objectId());
    }
};

template <typename T>
class CMyClient;
static std::vector<CMyClient<CFdbBaseObject> *> my_client_objects;
template <typename T>
class CInvokeTimer : public CMethodLoopTimer<CMyClient<T> >
{
public:
    CInvokeTimer(CMyClient<T> *client)
        : CMethodLoopTimer<CMyClient<T> >(1000, true, client, &CMyClient<T>::callServer)
    {}
};

template <typename T>
class CMyClient : public T
{
public:
    CInvokeTimer<T> *mTimer;
    CMyClient(const char *name, CBaseWorker *worker = 0, CFdbBaseContext *context = 0)
        : T(name, worker, context)
    {
        mTimer = new CInvokeTimer<T>(this);
        mTimer->attach(&main_worker, false, false);
    }
    ~CMyClient()
    {
        delete mTimer;
    }

    void callServer(CMethodLoopTimer<CMyClient<T> > *timer)
    {
        NFdbExample::SongId song_id;
        song_id.set_id(1234);
        CBaseJob::Ptr ref(new CMyMessage(REQ_METADATA));
        CFdbProtoMsgBuilder builder(song_id);
        this->invoke(ref, builder);
        auto msg = castToMessage<CMyMessage *>(ref);
        printMetadata(this->objId(), msg->metadata());

        if (msg->isStatus())
        {
            int32_t id;
            std::string reason;
            if (!msg->decodeStatus(id, reason))
            {
                FDB_LOG_E("onReply: fail to decode status!\n");
                return;
            }
            if (msg->isError())
            {
                FDB_LOG_I("sync reply: status is received: msg code: %d, id: %d, reason: %s\n", msg->code(), id, reason.c_str());
            }
            return;
        }

        NFdbExample::NowPlayingDetails now_playing;
        CFdbProtoMsgParser parser(now_playing);
        if (msg->deserialize(parser))
        {
            auto artist = now_playing.artist().c_str();
            auto album = now_playing.album().c_str();
            auto genre = now_playing.genre().c_str();
            auto title = now_playing.title().c_str();
            auto file_name = "";
            if (now_playing.has_file_name())
            {
                file_name = now_playing.file_name().c_str();
            }
            auto folder_name = "";
            if (now_playing.has_folder_name())
            {
                folder_name = now_playing.folder_name().c_str();
            }
            int32_t elapse_time = now_playing.elapse_time();
            FDB_LOG_I("OBJ %d, artist: %s, album: %s, genre: %s, title: %s, file name: %s, folder name: %s, elapse time: %d\n",
                        this->objId(), artist, album, genre, title, file_name, folder_name, elapse_time);
        }
        else
        {
            FDB_LOG_I("OBJ %d Error! Unable to decode message!!!\n", this->objId());
        }

        std::string raw_buffer("raw buffer test for invoke()!");
        this->invoke(REQ_RAWDATA, raw_buffer.c_str(), (int32_t)raw_buffer.length() + 1);
    }

protected:
    void onOnline(const CFdbOnlineInfo &info);
    void onOffline(const CFdbOnlineInfo &info)
    {
        FDB_LOG_I("OBJ %d client session shutdown: %d\n", this->objId(), info.mSid);
        if (info.mFirstOrLast)
        {
            mTimer->disable();
        }
    }

    void onBroadcast(CBaseJob::Ptr &msg_ref)
    {
        auto msg = castToMessage<CBaseMessage *>(msg_ref);
        FDB_LOG_I("OBJ %d Broadcast is received: %d; topic: %s\n", this->objId(), msg->code(), msg->topic().c_str());
        //CFdbMsgMetadata md;
        //msg->metadata(md);
        //printMetadata(md);

        switch (msg->code())
        {
            case NTF_ELAPSE_TIME:
            {
                std::string topic(msg->topic());
                if (!topic.compare("my_topic"))
                {
                    NFdbExample::ElapseTime et;
                    CFdbProtoMsgParser parser(et);
                    if (msg->deserialize(parser))
                    {
                        FDB_LOG_I("OBJ %d elapse time is received: hour: %d, minute: %d, second: %d\n",
                                    this->objId(), et.hour(), et.minute(), et.second());
                    }
                    else
                    {
                        FDB_LOG_E("Unable to decode NFdbExample::ElapseTime!\n");
                    }
                }
                else if (!topic.compare("raw_buffer"))
                {
                    // const char *buffer = (char *)msg->getPayloadBuffer();
                    int32_t size = msg->getPayloadSize();
                    //std::cout << "OBJ " << this->objId() << " Broadcast of raw buffer is received: size: " << size << ", data: " << buffer << std::endl;
                    FDB_LOG_I("OBJ %d Broadcast of raw buffer is received: size: %d\n", this->objId(), size);
                }
            }
            break;
            case NTF_MEDIAPLAYER_CREATED:
            {
                NFdbExample::FdbMsgObjectInfo obj_info;
                CFdbProtoMsgParser parser(obj_info);
                msg->deserialize(parser);
                auto obj = new CMyClient<CFdbBaseObject>("mediaplayer", &mediaplayer_worker);
                obj->connect(dynamic_cast<CBaseEndpoint *>(this), obj_info.obj_id());
                my_client_objects.push_back(obj);
            }
            break;
            default:
            break;
        }
    }

    void onInvoke(CBaseJob::Ptr &msg_ref)
    {
    }

    void onReply(CBaseJob::Ptr &msg_ref)
    {
        auto msg = castToMessage<CBaseMessage *>(msg_ref);
        FDB_LOG_I("OBJ %d response is receieved. sn: %d\n", this->objId(), msg->sn());
        printMetadata(this->objId(),  msg->metadata());

        switch (msg->code())
        {
            case REQ_METADATA:
            {
                if (msg->isStatus())
                {
                    if (msg->isError())
                    {
                        int32_t error_code;
                        std::string reason;
                        if (!msg->decodeStatus(error_code, reason))
                        {
                            FDB_LOG_E("onReply: fail to decode status!\n");
                            return;
                        }
                        FDB_LOG_I("onReply(): status is received: msg code: %d, error_code: %d, reason: %s\n",
                              msg->code(), error_code, reason.c_str());
                    }
                    return;
                }
                NFdbExample::NowPlayingDetails now_playing;
                CFdbProtoMsgParser parser(now_playing);
                if (msg->deserialize(parser))
                {
                    auto artist = now_playing.artist().c_str();
                    auto album = now_playing.album().c_str();
                    auto genre = now_playing.genre().c_str();
                    auto title = now_playing.title().c_str();
                    auto file_name = "";
                    if (now_playing.has_file_name())
                    {
                        file_name = now_playing.file_name().c_str();
                    }
                    auto folder_name = "";
                    if (now_playing.has_folder_name())
                    {
                        folder_name = now_playing.folder_name().c_str();
                    }
                    int32_t elapse_time = now_playing.elapse_time();
                    FDB_LOG_I("OBJ %d artist: %s, album: %s, genre: %s, title: %s, file name: %s, folder name: %s, elapse time: %d\n",
                                this->objId(), artist, album, genre, title, file_name, folder_name, elapse_time);
                }
                else
                {
                    FDB_LOG_I("OBJ %d Error! Unable to decode message!!!\n", this->objId());
                }
            }
            break;
            case REQ_RAWDATA:
            {
                if (msg->isStatus())
                {
                    int32_t error_code;
                    std::string reason;
                    if (!msg->decodeStatus(error_code, reason))
                    {
                        FDB_LOG_E("onReply: fail to decode status!\n");
                        return;
                    }
                    FDB_LOG_I("onReply(): status is received: msg code: %d, error_code: %d, reason: %s\n",
                          msg->code(), error_code, reason.c_str());
                    return;
                }
            }
            break;
            default:
            break;
        }
    }

    void onStatus(CBaseJob::Ptr &msg_ref
                        , int32_t error_code
                        , const char *description)
    {
        CBaseMessage *msg = castToMessage<CBaseMessage *>(msg_ref);
        if (msg->isSubscribe())
        {
            if (msg->isError())
            {
            }
            else
            {
                FDB_LOG_I("OBJ %d subscribe is ok! sn: %d is received.\n", this->objId(), msg->sn());
            }
        }
        FDB_LOG_I("OBJ %d Reason: %s\n", this->objId(), description);
    }
    void onCreateObject(CBaseEndpoint *endpoint, CFdbMessage *msg)
    {
        FDB_LOG_I("Create object %d because message %d is received.\n", msg->objectId(), msg->code());
        CMyServer<CFdbBaseObject> *obj = new CMyServer<CFdbBaseObject>("mediaplayer", &mediaplayer_worker);
        obj->bind(dynamic_cast<CBaseEndpoint *>(this), msg->objectId());
    }
};

template<typename T>
void CMyServer<T>::onOnline(const CFdbOnlineInfo &info)
{
    FDB_LOG_I("OBJ %d server session up: %d\n", this->objId(), info.mSid);
    if (this->isPrimary())
    {
#ifdef OBJ_FROM_SERVER_TO_CLIENT
        for (int j = 0; j < FDB_NUM_OF_CLIENT_OBJECT; ++j)
        {
            char obj_id[64];
            sprintf(obj_id, "obj%u", j);
            std::string obj_name = this->name() + obj_id;
            auto obj = new CMyClient<CFdbBaseObject>(obj_name.c_str(), &mediaplayer_worker);
            obj->connect(this->endpoint(), 1000 + j);
        }
#endif
    }
    else
    {
        mTimer->enable();
    }
}

template<typename T>
void CMyClient<T>::onOnline(const CFdbOnlineInfo &info)
{
    FDB_LOG_I("OBJ %d client session online: %d\n", this->objId(), info.mSid);
    if (info.mFirstOrLast)
    {
        CFdbMsgSubscribeList subscribe_list;
        this->addNotifyItem(subscribe_list, NTF_ELAPSE_TIME, "my_topic");
        this->addNotifyItem(subscribe_list, NTF_ELAPSE_TIME, "raw_buffer");
        this->addNotifyItem(subscribe_list, NTF_ELAPSE_TIME);
        this->addNotifyItem(subscribe_list, NTF_MEDIAPLAYER_CREATED);
        /*
         * register NTF_MANUAL_UPDATE for manual update: it will not
         * update unless update() is called
         */
        this->subscribe(subscribe_list);
    }
    if (this->isPrimary())
    {
#if 0
        this->invoke(REQ_CREATE_MEDIAPLAYER);
        CMyClient<CFdbBaseObject> *obj = new CMyClient<CFdbBaseObject>("mediaplayer1", &mediaplayer_worker);
        obj->connect(dynamic_cast<CBaseEndpoint *>(this), 1);

        obj = new CMyClient<CFdbBaseObject>("mediaplayer1", &mediaplayer_worker);
        obj->connect(dynamic_cast<CBaseEndpoint *>(this), 2);
#endif
#ifndef OBJ_FROM_SERVER_TO_CLIENT
        for (int j = 0; j < FDB_NUM_OF_CLIENT_OBJECT; ++j)
        {
            char obj_id[64];
            sprintf(obj_id, "obj%u", j);
            std::string obj_name = this->name() + obj_id;
            auto obj = new CMyClient<CFdbBaseObject>(obj_name.c_str(), &mediaplayer_worker);
            obj->connect(this->endpoint(), 1000 + j);
        }
#endif
    }
    else
    {
        mTimer->enable();
    }
}

class CObjTestTimer : public CBaseLoopTimer
{
public:
    CObjTestTimer()
        : CBaseLoopTimer(1000, true)
        , mCount(0)
    {}
protected:
    void run()
    {
        mCount++;
        if (mCount%2 == 0){
            worker()->name("");
        }else{
            const char* nullp = 0;
            worker()->name(nullp);
        }
        
    #if 0
        for (std::vector<CMyClient<CFdbBaseObject> *>::iterator it = my_client_objects.begin();
                it != my_client_objects.end(); ++it)
        {
            CMyClient<CFdbBaseObject> *obj = *it;
            obj->disconnect();
        }
        for (std::vector<CMyServer<CFdbBaseObject> *>::iterator it = my_server_objects.begin();
                it != my_server_objects.end(); ++it)
        {
            CMyServer<CFdbBaseObject> *obj = *it;
            obj->unbind();
        }
    #endif
    }
    int mCount;
};

int main(int argc, char **argv)
{
    //shared_ptr_test();
    FDB_CONTEXT->start();
    fdbLogStaticContent(true);
    main_worker.start();
    mediaplayer_worker.start();

     auto worker_ptr = &main_worker;
    //CBaseWorker *worker_ptr = 0;

    const char *servers = 0;
    const char *clients = 0;
    const char *public_key = 0;
    const char *private_key = 0;
    const char *root_ca = 0;
    int32_t help = 0;
    const struct fdb_option core_options[] = {
        { FDB_OPTION_STRING, "servers", 's', &servers },
        { FDB_OPTION_STRING, "clients", 'c', &clients },
        { FDB_OPTION_STRING, "public_key", 'p', &public_key },
        { FDB_OPTION_STRING, "private_key", 'v', &private_key },
        { FDB_OPTION_STRING, "root_ca", 'r', &root_ca },
        { FDB_OPTION_BOOLEAN, "help", 'h', &help }
    };
    fdb_parse_options(core_options, ARRAY_LENGTH(core_options), &argc, argv);
    if (help)
    {
        printf("Usage: cmd [-s server1,server2,...][ -c client1,client2,...]\n");
        return 0;
    }
    uint32_t nr_servers = 0;
    char **server_list = servers ? strsplit(servers, ",", &nr_servers) : 0;
    uint32_t nr_clients = 0;
    char **client_list = clients ? strsplit(clients, ",", &nr_clients) : 0;

    CObjTestTimer obj_test_timer;
    obj_test_timer.attach(&main_worker, true);

    while (1)
    {
    std::vector<CMyServer<CBaseServer> *> servers;
    std::vector<CMyClient<CBaseClient> *> clients;
    std::vector<CFdbBaseContext *> server_contexts;
    std::vector<CFdbBaseContext *> client_contexts;

    if (server_list)
    {
        for (uint32_t i = 0; i < nr_servers; ++i)
        {
            std::string server_name = server_list[i];
            std::string url(FDB_URL_SVC);
            url += server_name;
            server_name += "_server";
            auto context = new CFdbBaseContext();
            server_contexts.push_back(context);
            context->start();
            auto server = new CMyServer<CBaseServer>(server_name.c_str(), worker_ptr, context);
            server->createSSL(public_key, private_key, root_ca);
            server->keepAlive(12, 3);
            servers.push_back(server);
            server->enableUDP(true);
            server->bind(url.c_str());
        }
    }

    if (client_list)
    {
        for (uint32_t i = 0; i < nr_clients; ++i)
        {
            std::string server_name = client_list[i];
            std::string url(FDB_URL_SVC);
            url += server_name;
            server_name += "_client";
            auto context = new CFdbBaseContext();
            client_contexts.push_back(context);
            context->start();
            auto client = new CMyClient<CBaseClient>(server_name.c_str(), worker_ptr, context);
            client->createSSL(public_key, private_key, root_ca);
            client->keepAlive(12, 3);
            clients.push_back(client);
            client->enableUDP(true);
            client->enableReconnect(true);
            client->connect(url.c_str());
        }
    }

#if FDB_SHUTDOWN_TEST == 1
    sysdep_sleep(5000);

    FDB_LOG_I("================ Destroying clients ================\n");
    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        (*it)->mTimer->disable();
        (*it)->prepareDestroy();
        delete *it;
    }
    FDB_LOG_I("================ Destroying servers ================\n");
    for (auto it = servers.begin(); it != servers.end(); ++it)
    {
        (*it)->mTimer->disable();
        (*it)->prepareDestroy();
        delete *it;
    }
    FDB_LOG_I("================ Destroying client contexts ================\n");
    for (auto it = client_contexts.begin(); it != client_contexts.end(); ++it)
    {
        (*it)->exit();
        (*it)->join();
        delete (*it);
    }
    FDB_LOG_I("================ Destroying server contexts ================\n");
    for (auto it = server_contexts.begin(); it != server_contexts.end(); ++it)
    {
        (*it)->exit();
        (*it)->join();
        delete (*it);
    }
    sysdep_sleep(1000);
#else
    CBaseWorker background_worker;
    background_worker.start(FDB_WORKER_EXE_IN_PLACE);
#endif
    }

    return 0;
}


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

#include "FdbusGlobal.h"
#include <fdbus/CBaseClient.h>
#include <fdbus/CFdbMessage.h>
#include <fdbus/CLogProducer.h>
#include <fdbus/CFdbContext.h>
#define FDB_LOG_TAG "FDB_JNI"
#include <fdbus/fdb_log_trace.h>

namespace ipc {
namespace fdbus {
#define FDB_MSG_TYPE_JNI_INVOKE (FDB_MSG_TYPE_SYSTEM + 1)

class CJniClient : public CBaseClient
{
public:
    CJniClient(JNIEnv *env, const char *name, jobject java_client);
    CJniClient(const char *name);
    ~CJniClient();
protected:
    void onOnline(const CFdbOnlineInfo &info);
    void onOffline(const CFdbOnlineInfo &info);
    void onReply(CBaseJob::Ptr &msg_ref);
    void onGetEvent(CBaseJob::Ptr &msg_ref);
    void onBroadcast(CBaseJob::Ptr &msg_ref);
private:
    jobject mJavaClient;
};

class CJniInvokeMsg : public CBaseMessage
{
public:
    jobject mUserData;

    CJniInvokeMsg(FdbMsgCode_t code, jobject user_data, EFdbQOS qos)
        : CBaseMessage(code, qos)
        , mUserData(user_data)
    {
    }
    FdbMessageType_t getTypeId()
    {
        return FDB_MSG_TYPE_JNI_INVOKE;
    }
    ~CJniInvokeMsg()
    {
        if (mUserData)
        {
            JNIEnv *env = CGlobalParam::obtainJniEnv();
            if (env)
            {
                env->DeleteGlobalRef(mUserData);
                mUserData = 0;
            }
        }
    }
};

CBaseClient *FDB_createJniClient(const char *name)
{
    return new CJniClient(name);
}

CJniClient::CJniClient(JNIEnv *env, const char *name, jobject java_client)
    : CBaseClient(name)
    , mJavaClient(env->NewGlobalRef(java_client))
{
    enableReconnect(true);
}

CJniClient::CJniClient(const char *name)
    : CBaseClient(name)
    , mJavaClient(0)
{
    enableReconnect(true);
}
    
CJniClient::~CJniClient()
{
    disconnect();
    if (mJavaClient)
    {
        JNIEnv *env = CGlobalParam::obtainJniEnv();
        if (env)
        {
            env->DeleteGlobalRef(mJavaClient);
            mJavaClient = 0;
        }
    }
}

void CJniClient::onOnline(const CFdbOnlineInfo &info)
{
    if (!mJavaClient)
    {
        CFdbBaseObject::onOnline(info);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        env->CallVoidMethod(mJavaClient, CFdbusClientParam::mOnOnline, info.mSid, (jint)info.mQOS);
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniClient::onOffline(const CFdbOnlineInfo &info)
{
    if (!mJavaClient)
    {
        CFdbBaseObject::onOffline(info);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        env->CallVoidMethod(mJavaClient, CFdbusClientParam::mOnOffline, info.mSid, (jint)info.mQOS);
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniClient::onReply(CBaseJob::Ptr &msg_ref)
{
    auto msg = castToMessage<CFdbMessage *>(msg_ref);
    if (!mJavaClient || (msg->getTypeId() < 0))
    {
        CFdbBaseObject::onReply(msg_ref);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        int32_t error_code = FDB_ST_OK;
        if (msg->isStatus())
        {
            std::string reason;
            if (!msg->decodeStatus(error_code, reason))
            {
                FDB_LOG_E("onReply: fail to decode status!\n");
                error_code = FDB_ST_MSG_DECODE_FAIL;
            }
        }

        CJniInvokeMsg *jni_msg = 0;
        if (msg->getTypeId() == FDB_MSG_TYPE_JNI_INVOKE)
        {
            jni_msg = castToMessage<CJniInvokeMsg *>(msg_ref);
        }

#if 0
        jobject user_data = 0;
        if (msg->mUserData)
        {
            user_data = env->NewLocalRef(msg->mUserData);
            env->DeleteGlobalRef(msg->mUserData);
            msg->mUserData = 0;
        }
#endif
        env->CallVoidMethod(mJavaClient,
                            CFdbusClientParam::mOnReply,
                            msg->session(),
                            msg->code(),
                            CGlobalParam::createRawPayloadBuffer(env, msg),
                            error_code,
                            jni_msg ? jni_msg->mUserData : 0
                            );
        if (jni_msg)
        {
            env->DeleteGlobalRef(jni_msg->mUserData);
            jni_msg->mUserData = 0;
        }
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniClient::onGetEvent(CBaseJob::Ptr &msg_ref)
{
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        auto msg = castToMessage<CFdbMessage *>(msg_ref);
        int32_t error_code = FDB_ST_OK;
        if (msg->isStatus())
        {
            std::string reason;
            if (!msg->decodeStatus(error_code, reason))
            {
                FDB_LOG_E("onReply: fail to decode status!\n");
                error_code = FDB_ST_MSG_DECODE_FAIL;
            }
        }

        CJniInvokeMsg *jni_msg = 0;
        if (msg->getTypeId() == FDB_MSG_TYPE_JNI_INVOKE)
        {
            jni_msg = castToMessage<CJniInvokeMsg *>(msg_ref);
        }

#if 0
        jobject user_data = 0;
        if (msg->mUserData)
        {
            user_data = env->NewLocalRef(msg->mUserData);
            env->DeleteGlobalRef(msg->mUserData);
            msg->mUserData = 0;
        }
#endif
        env->CallVoidMethod(mJavaClient,
                            CFdbusClientParam::mOnGetEvent,
                            msg->session(),
                            msg->code(),
                            env->NewStringUTF(msg->topic().c_str()),
                            CGlobalParam::createRawPayloadBuffer(env, msg),
                            error_code,
                            jni_msg ? jni_msg->mUserData : 0
                            );
        if (jni_msg)
        {
            env->DeleteGlobalRef(jni_msg->mUserData);
            jni_msg->mUserData = 0;
        }
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniClient::onBroadcast(CBaseJob::Ptr &msg_ref)
{
    if (!mJavaClient)
    {
        CFdbBaseObject::onBroadcast(msg_ref);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        auto msg = castToMessage<CBaseMessage *>(msg_ref);
        if (msg)
        {
            auto c_topic = msg->topic().c_str();
            jstring topic = env->NewStringUTF(c_topic);
            env->CallVoidMethod(mJavaClient,
                                CFdbusClientParam::mOnBroadcast,
                                msg->session(),
                                msg->code(),
                                topic,
                                CGlobalParam::createRawPayloadBuffer(env, msg)
                                );
        }
    }
    CGlobalParam::releaseJniEnv(env);
}
}
}
using namespace ipc::fdbus;

JNIEXPORT jlong JNICALL Java_ipc_fdbus_FdbusClient_fdb_1create
  (JNIEnv *env, jobject thiz, jstring name)
{
    const char* c_name = 0;
    if (name)
    {
        c_name = env->GetStringUTFChars(name, 0);
    }
    
    auto endpoint_name = c_name;
    if (!endpoint_name)
    {
        endpoint_name = "default client";
        FDB_LOG_W("Java_FdbusClient_fdb_1create: using %s as default name!\n", endpoint_name);
    }
    jlong handle = (jlong) new CJniClient(env, endpoint_name, thiz);
    if (c_name)
    {
        env->ReleaseStringUTFChars(name, c_name);
    }
    return handle;
}

JNIEXPORT void JNICALL Java_ipc_fdbus_FdbusClient_fdb_1destroy
  (JNIEnv *, jobject, jlong handle)
{
    auto client = (CJniClient *)handle;
    if (client)
    {
        client->prepareDestroy();
        delete client;
    }
}
 
JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1connect
  (JNIEnv *env, jobject, jlong handle, jstring url)
{
    bool ret = false;
    const char* c_url = env->GetStringUTFChars(url, 0);
    if (c_url)
    {
        CJniClient *client = (CJniClient *)handle;
        if (client)
        {
            client->connect(c_url);
            ret = true;
        }
        env->ReleaseStringUTFChars(url, c_url);
    }
    return ret;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1disconnect
  (JNIEnv *env, jobject, jlong handle)
{
    auto client = (CJniClient *)handle;
    if (client)
    {
        client->disconnect();
        return true;
    }
    return false;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1invoke_1async
                              (JNIEnv *env,
                              jobject,
                              jlong handle,
                              jint code,
                              jbyteArray pb_data,
                              jstring log_data,
                              jobject user_data,
                              jint timeout,
                              jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }
    
    jbyte *c_array = 0;
    int len_arr = 0;
    if (pb_data)
    {
        c_array = env->GetByteArrayElements(pb_data, 0);
        len_arr = env->GetArrayLength(pb_data);
    }

    const char* c_log_data = 0;
    if (log_data)
    {
        c_log_data = env->GetStringUTFChars(log_data, 0);
    }

    jboolean ret = false;
    if (user_data && env->IsInstanceOf(user_data, CFdbusActionParam::mClass))
    {
        auto callback = env->NewGlobalRef(user_data);
        ret = client->invoke(code, [callback](CBaseJob::Ptr &msg_ref, CFdbBaseObject *obj)
            {
                CGlobalParam::callAction(callback, msg_ref, CGlobalParam::REPLY);
            },
            (const void *)c_array, len_arr, 0, timeout, (EFdbQOS)qos, c_log_data
        );
    }
    else
    {
        auto msg = new CJniInvokeMsg(code, user_data ? env->NewGlobalRef(user_data) : 0, (EFdbQOS)qos);
        if (c_log_data)
        {
            msg->setLogData(c_log_data);
        }
        ret = client->invoke(msg, (const void *)c_array, len_arr, timeout);
    }

    if (c_log_data)
    {
        env->ReleaseStringUTFChars(log_data, c_log_data);
    }
    if (c_array)
    {
        env->ReleaseByteArrayElements(pb_data, c_array, 0);
    }
    
    return ret;
}

JNIEXPORT jobject JNICALL Java_ipc_fdbus_FdbusClient_fdb_1invoke_1sync
                              (JNIEnv *env,
                               jobject,
                               jlong handle,
                               jint code,
                               jbyteArray pb_data,
                               jstring log_data,
                               jint timeout,
                               jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return 0;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return 0;
    }

    jbyte *c_array = 0;
    int len_arr = 0;
    if (pb_data)
    {
        c_array = env->GetByteArrayElements(pb_data, 0);
        len_arr = env->GetArrayLength(pb_data);
    }

    const char* c_log_data = 0;
    if (log_data)
    {
        c_log_data = env->GetStringUTFChars(log_data, 0);
    }

    auto invoke_msg = new CBaseMessage(code, (EFdbQOS)qos);
    CBaseJob::Ptr ref(invoke_msg);

    if (c_log_data)
    {
        invoke_msg->setLogData(c_log_data);
        env->ReleaseStringUTFChars(log_data, c_log_data);
    }
    
    jboolean ret = client->invoke(ref, (const void *)c_array, len_arr, timeout);
    if (c_array)
    {
        env->ReleaseByteArrayElements(pb_data, c_array, 0);
    }
    if (!ret)
    {
        FDB_LOG_E("Java_ipc_fdbus_FdbusClient_fdb_1invoke_1sync: unable to call method: %d\n", code);
        return 0;
    }

    int32_t error_code = FDB_ST_OK;
    if (invoke_msg->isStatus())
    {
        std::string reason;
        if (!invoke_msg->decodeStatus(error_code, reason))
        {
            FDB_LOG_E("onReply: fail to decode status!\n");
            error_code = FDB_ST_MSG_DECODE_FAIL;
        }
    }

    jmethodID constructor = env->GetMethodID(
                                    CFdbusMessageParam::mClass,
                                    "<init>",
                                    "(II[BI)V");
    if (!constructor)
    {
        FDB_LOG_E("Java_ipc_fdbus_FdbusClient_fdb_1invoke_1sync: unable to get constructor.\n");
        return 0;
    }
    return env->NewObject(CFdbusMessageParam::mClass,
                                    constructor,
                                    invoke_msg->session(),
                                    code,
                                    CGlobalParam::createRawPayloadBuffer(env, invoke_msg),
                                    error_code);
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1send
                              (JNIEnv *env,
                               jobject,
                               jlong handle,
                               jint code,
                               jbyteArray pb_data,
                               jstring log_data,
                               jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }
    
    jbyte *c_array = 0;
    int len_arr = 0;
    if (pb_data)
    {
        c_array = env->GetByteArrayElements(pb_data, 0);
        len_arr = env->GetArrayLength(pb_data);
    }

    const char* c_log_data = 0;
    if (log_data)
    {
        c_log_data = env->GetStringUTFChars(log_data, 0);
    }

    jboolean ret = client->send((FdbMsgCode_t)code,
                        (const void *)c_array,
                        len_arr,
                        (EFdbQOS)qos,
                        c_log_data);
    if (c_array)
    {
        env->ReleaseByteArrayElements(pb_data, c_array, 0);
    }
    if (c_log_data)
    {
        env->ReleaseStringUTFChars(log_data, c_log_data);
    }
    return ret;
}

static jint getSubscriptionList(JNIEnv *env,
                                jobject sub_items,
                                CJniClient *client,
                                CFdbMsgSubscribeList &subscribe_list)
{
    CGlobalParam::tSubscriptionTbl sub_tbl;
    CGlobalParam::getSubscriptionList(env, sub_items, sub_tbl);

    int32_t len = (int32_t)sub_tbl.size();
    for (int32_t i = 0; i < len; ++i)
    {
        client->addNotifyItem(subscribe_list, sub_tbl[i].mCode, sub_tbl[i].mTopic.c_str());
    }

    return len;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1subscribe
                        (JNIEnv *env,
                         jobject,
                         jlong handle,
                         jobject sub_items,
                         jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }

    CFdbMsgSubscribeList subscribe_list;
    jint len = getSubscriptionList(env, sub_items, client, subscribe_list);
    return len ? client->subscribe(subscribe_list, 0, (EFdbQOS)qos) : false;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1unsubscribe
                        (JNIEnv *env,
                         jobject,
                         jlong handle,
                         jobject sub_items,
                         jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }

    CFdbMsgSubscribeList subscribe_list;
    jint len = getSubscriptionList(env, sub_items, client, subscribe_list);
    return len ? client->unsubscribe(subscribe_list, (EFdbQOS)qos) : false;
}

JNIEXPORT jstring JNICALL Java_ipc_fdbus_FdbusClient_fdb_1endpoint_1name
  (JNIEnv *env, jobject, jlong handle)
{
    auto client = (CJniClient *)handle;
    const char *name = "";
    
    if (client)
    {
        name = client->name().c_str();
    }
    
    return env->NewStringUTF(name);
}

JNIEXPORT jstring JNICALL Java_ipc_fdbus_FdbusClient_fdb_1bus_1name
  (JNIEnv *env, jobject, jlong handle)
{
    auto client = (CJniClient *)handle;
    const char *name = "";
      
    if (client)
    {
        name = client->nsName().c_str();
    }
      
    return env->NewStringUTF(name);
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1log_1enabled
  (JNIEnv *env, jobject, jlong handle, jint msg_type)
{
    auto client = (CJniClient *)handle;
    if (client)
    {
        CLogProducer *logger = FDB_CONTEXT->getLogger();
        if (logger && logger->checkLogEnabled((EFdbMessageType)msg_type, 0, client))
        {
            return true;
        }
    }

    return false;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1publish
                            (JNIEnv *env,
                             jobject,
                             jlong handle,
                             jint event,
                             jstring topic,
                             jbyteArray event_data,
                             jstring log_data,
                             jboolean always_update,
                             jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }
    
    jbyte *c_array = 0;
    int len_arr = 0;
    if (event_data)
    {
        c_array = env->GetByteArrayElements(event_data, 0);
        len_arr = env->GetArrayLength(event_data);
    }
    
    const char* c_log_data = 0;
    if (log_data)
    {
        c_log_data = env->GetStringUTFChars(log_data, 0);
    }

    const char *c_topic = 0;
    if (topic)
    {
        c_topic = env->GetStringUTFChars(topic, 0);
    }
    
    jboolean ret = client->publish((FdbMsgCode_t)event,
                                   (const uint8_t *)c_array,
                                   len_arr,
                                   c_topic,
                                   always_update,
                                   (EFdbQOS)qos,
                                   c_log_data);
    if (c_array)
    {
        env->ReleaseByteArrayElements(event_data, c_array, 0);
    }
    if (c_log_data)
    {
        env->ReleaseStringUTFChars(log_data, c_log_data);
    }
    if (c_topic)
    {
        env->ReleaseStringUTFChars(topic, c_topic);
    }
    return ret;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusClient_fdb_1get_1event_1async
                            (JNIEnv *env,
                             jobject,
                             jlong handle,
                             jint event,
                             jstring topic,
                             jobject user_data,
                             jint timeout,
                             jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }

    const char *c_topic = 0;
    if (topic)
    {
        c_topic = env->GetStringUTFChars(topic, 0);
    }

    auto msg = new CJniInvokeMsg(event, user_data ? env->NewGlobalRef(user_data) : 0, (EFdbQOS)qos);
    
    jboolean ret = client->get(msg, c_topic, timeout);

    if (c_topic)
    {
        env->ReleaseStringUTFChars(topic, c_topic);
    }
    
    return ret;
}

JNIEXPORT jobject JNICALL Java_ipc_fdbus_FdbusClient_fdb_1get_1event_1sync
                            (JNIEnv *env,
                             jobject,
                             jlong handle,
                             jint event,
                             jstring topic,
                             jint timeout,
                             jint qos)
{
    auto client = (CJniClient *)handle;
    if (!client)
    {
        return 0;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return 0;
    }

    const char *c_topic = 0;
    if (topic)
    {
        c_topic = env->GetStringUTFChars(topic, 0);
    }
    
    auto invoke_msg = new CBaseMessage(event, (EFdbQOS)qos);
    CBaseJob::Ptr ref(invoke_msg);
    
    jboolean ret = client->get(ref, c_topic, timeout);

    if (c_topic)
    {
        env->ReleaseStringUTFChars(topic, c_topic);
    }
    
    if (!ret)
    {
        FDB_LOG_E("Java_ipc_fdbus_FdbusClient_fdb_1get_1event_1sync: unable to call method: %d\n", event);
        return 0;
    }
    
    int32_t error_code = FDB_ST_OK;
    if (invoke_msg->isStatus())
    {
        std::string reason;
        if (!invoke_msg->decodeStatus(error_code, reason))
        {
            FDB_LOG_E("onReply: fail to decode status!\n");
            error_code = FDB_ST_MSG_DECODE_FAIL;
        }
    }
    
    jmethodID constructor = env->GetMethodID(
                                    CFdbusMessageParam::mClass,
                                    "<init>",
                                    "(IILjava/lang/String;[BI)V");
    if (!constructor)
    {
        FDB_LOG_E("Java_ipc_fdbus_FdbusClient_fdb_1get_1event_1sync: unable to get constructor: %d\n", ret);
        return 0;
    }
    return env->NewObject(CFdbusMessageParam::mClass,
                                    constructor,
                                    invoke_msg->session(),
                                    event,
                                    topic,
                                    CGlobalParam::createRawPayloadBuffer(env, invoke_msg),
                                    error_code);
}

static const JNINativeMethod gFdbusClientMethods[] = {
    {(char *)"fdb_create",
             (char *)"(Ljava/lang/String;)J",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1create},
    {(char *)"fdb_destroy",
             (char *)"(J)V",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1destroy},
    {(char *)"fdb_connect",
             (char *)"(JLjava/lang/String;)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1connect},
    {(char *)"fdb_disconnect",
             (char *)"(J)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1disconnect},
    {(char *)"fdb_invoke_async",
             (char *)"(JI[BLjava/lang/String;Ljava/lang/Object;II)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1invoke_1async},
    {(char *)"fdb_invoke_sync",
             (char *)"(JI[BLjava/lang/String;II)Lipc/fdbus/FdbusMessage;",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1invoke_1sync},
    {(char *)"fdb_send",
             (char *)"(JI[BLjava/lang/String;I)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1send},
    {(char *)"fdb_subscribe",
             (char *)"(JLjava/util/ArrayList;I)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1subscribe},
    {(char *)"fdb_unsubscribe",
             (char *)"(JLjava/util/ArrayList;I)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1unsubscribe},
    {(char *)"fdb_endpoint_name",
             (char *)"(J)Ljava/lang/String;",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1endpoint_1name},
    {(char *)"fdb_bus_name",
             (char *)"(J)Ljava/lang/String;",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1bus_1name},
    {(char *)"fdb_log_enabled",
             (char *)"(JI)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1log_1enabled},
    {(char *)"fdb_publish",
             (char *)"(JILjava/lang/String;[BLjava/lang/String;ZI)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1publish},
    {(char *)"fdb_get_event_async",
             (char *)"(JILjava/lang/String;Ljava/lang/Object;II)Z",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1get_1event_1async},
    {(char *)"fdb_get_event_sync",
             (char *)"(JILjava/lang/String;II)Lipc/fdbus/FdbusMessage;",
             (void*) Java_ipc_fdbus_FdbusClient_fdb_1get_1event_1sync},
};
  
int register_fdbus_client(JNIEnv *env)
{
    return CGlobalParam::jniRegisterNativeMethods(env,
                         "ipc/fdbus/FdbusClient",
                         gFdbusClientMethods,
                         Fdb_Num_Elems(gFdbusClientMethods));
    return 0;
}
  

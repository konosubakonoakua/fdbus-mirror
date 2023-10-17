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
#include <fdbus/CBaseServer.h>
#include <fdbus/CFdbMessage.h>
#include <fdbus/CLogProducer.h>
#include <fdbus/CFdbContext.h>
#define FDB_LOG_TAG "FDB_JNI"
#include <fdbus/fdb_log_trace.h>

namespace ipc {
namespace fdbus {

class CJniServer : public CBaseServer
{
public:
    CJniServer(JNIEnv *env, const char *name, jobject &java_server);
    CJniServer(const char *name);
    ~CJniServer();
protected:
    void onOnline(const CFdbOnlineInfo &info);
    void onOffline(const CFdbOnlineInfo &info);
    void onInvoke(CBaseJob::Ptr &msg_ref);
    void onSubscribe(CBaseJob::Ptr &msg_ref);
private:
    jobject mJavaServer;
};

CBaseServer *FDB_createJniServer(const char *name)
{
    return new CJniServer(name);
}

CJniServer::CJniServer(JNIEnv *env, const char *name, jobject &java_server)
    : CBaseServer(name)
    , mJavaServer(env->NewGlobalRef(java_server))
{
}

CJniServer::CJniServer(const char *name)
    : CBaseServer(name)
    , mJavaServer(0)
{
}

CJniServer::~CJniServer()
{
    unbind();
    if (mJavaServer)
    {
        JNIEnv *env = CGlobalParam::obtainJniEnv();
        if (env)
        {
            env->DeleteGlobalRef(mJavaServer);
            mJavaServer = 0;
        }
    }
}

void CJniServer::onOnline(const CFdbOnlineInfo &info)
{
    if (!mJavaServer)
    {
        CFdbBaseObject::onOnline(info);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        env->CallVoidMethod(mJavaServer, CFdbusServerParam::mOnOnline, info.mSid,
                            info.mFirstOrLast, (jint)info.mQOS);
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniServer::onOffline(const CFdbOnlineInfo &info)
{
    if (!mJavaServer)
    {
        CFdbBaseObject::onOffline(info);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        env->CallVoidMethod(mJavaServer, CFdbusServerParam::mOnOffline, info.mSid,
                            info.mFirstOrLast, (jint)info.mQOS);
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniServer::onInvoke(CBaseJob::Ptr &msg_ref)
{
    if (!mJavaServer)
    {
        CFdbBaseObject::onInvoke(msg_ref);
        return;
    }
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        auto msg = castToMessage<CFdbMessage *>(msg_ref);
        if (msg)
        {
            CBaseJob::Ptr *msg_handle = msg->needReply() ? new CBaseJob::Ptr(msg_ref) : 0;
            env->CallVoidMethod(mJavaServer,
                                CFdbusServerParam::mOnInvoke,
                                msg->session(),
                                msg->code(),
                                CGlobalParam::createRawPayloadBuffer(env, msg),
                                (int64_t)msg_handle
                                );
        }
    }
    CGlobalParam::releaseJniEnv(env);
}

void CJniServer::onSubscribe(CBaseJob::Ptr &msg_ref)
{
    JNIEnv *env = CGlobalParam::obtainJniEnv();
    if (env)
    {
        jclass cls_arr_list = env->FindClass("java/util/ArrayList");
        if (!cls_arr_list)
        {
            FDB_LOG_E("onSubscribe: unable find java/util/ArrayList!\n");
            return;
        }
        jmethodID constructor = env->GetMethodID(cls_arr_list, "<init>", "()V");
        if (!constructor)
        {
            FDB_LOG_E("onSubscribe: unable to get constructor for ava/util/ArrayList!\n");
            return;
        }
        jobject obj_arr_list = env->NewObject(cls_arr_list, constructor, "");
        if (!obj_arr_list)
        {
            FDB_LOG_E("onSubscribe: unable to create object of ava/util/ArrayList!\n");
            return;
        }
        jmethodID arr_list_add = env->GetMethodID(cls_arr_list, "add" , "(Ljava/lang/Object;)Z");
        if (!arr_list_add)
        {
            FDB_LOG_E("onSubscribe: unable to find add method for ava/util/ArrayList!\n");
            return;
        }

        constructor = env->GetMethodID(
                                    CFdbusSubscribeItemParam::mClass,
                                    "<init>",
                                    "(ILjava/lang/String;)V");
        if (!constructor)
        {
            FDB_LOG_E("onSubscribe: unable to get constructor for subscribe item!\n");
            return;
        }
        
        auto msg = castToMessage<CFdbMessage *>(msg_ref);
        const CFdbMsgSubscribeItem *sub_item;
        /* iterate all message id subscribed */
        FDB_BEGIN_FOREACH_SIGNAL(msg, sub_item)
        {
            auto msg_code = sub_item->msg_code();
            const char *c_topic = "";
            if (sub_item->has_topic())
            {
                c_topic = sub_item->topic().c_str();
            }
            jstring topic = c_topic ? env->NewStringUTF(c_topic) : 0;
            jobject j_sub_item = env->NewObject(CFdbusSubscribeItemParam::mClass,
                                                constructor, msg_code, topic);
            if (!j_sub_item)
            {
                FDB_LOG_E("onSubscribe: unable to create subscribe item!\n");
                continue;
            }
            env->CallBooleanMethod(obj_arr_list, arr_list_add, j_sub_item);
        }
        FDB_END_FOREACH_SIGNAL()

        CBaseJob::Ptr *msg_handle = new CBaseJob::Ptr(msg_ref);
        env->CallVoidMethod(mJavaServer,
                            CFdbusServerParam::mOnSubscribe,
                            msg->session(),
                            (int64_t)msg_handle,
                            obj_arr_list
                            );
    }
    CGlobalParam::releaseJniEnv(env);
}
}
}
using namespace ipc::fdbus;

JNIEXPORT jlong JNICALL Java_ipc_fdbus_FdbusServer_fdb_1create
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
        endpoint_name = "default server";
        FDB_LOG_W("Java_FdbusServer_fdb_1create: using %s as default name!\n", endpoint_name);
    }
    jlong handle = (jlong) new CJniServer(env, endpoint_name, thiz);
    if (c_name)
    {
        env->ReleaseStringUTFChars(name, c_name);
    }
    return handle;
}

JNIEXPORT void JNICALL Java_ipc_fdbus_FdbusServer_fdb_1destroy
  (JNIEnv *, jobject, jlong handle)
{
    auto server = (CJniServer *)handle;
    if (server)
    {
        server->prepareDestroy();
        delete server;
    }
}
 
JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusServer_fdb_1bind
  (JNIEnv *env, jobject, jlong handle, jstring url)
{
    bool ret = false;
    const char* c_url = env->GetStringUTFChars(url, 0);
    if (c_url)
    {
        auto server = (CJniServer *)handle;
        if (server)
        {
            server->bind(c_url);
            ret = true;
        }
        env->ReleaseStringUTFChars(url, c_url);
    }
    return ret;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusServer_fdb_1unbind
  (JNIEnv *, jobject, jlong handle)
{
    auto server = (CJniServer *)handle;
    if (server)
    {
        server->unbind();
        return true;
    }

    return false;
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusServer_fdb_1broadcast
                          (JNIEnv *env,
                           jobject,
                           jlong handle,
                           jint msg_code,
                           jstring topic,
                           jbyteArray pb_data,
                           jstring log_data,
                           jint qos)
{
    auto server = (CJniServer *)handle;
    if (!server)
    {
        return false;
    }
    if (qos >= (jint)FDB_QOS_INVALID)
    {
        return false;
    }

    const char *c_log_data = 0;
    if (log_data)
    {
        c_log_data = env->GetStringUTFChars(log_data, 0);
    }

    const char *c_topic = 0;
    if (topic)
    {
        c_topic = env->GetStringUTFChars(topic, 0);
    }

    jbyte *c_array = 0;
    int len_arr = 0;
    if (pb_data)
    {
        c_array = env->GetByteArrayElements(pb_data, 0);
        len_arr = env->GetArrayLength(pb_data);
    }

    bool ret = server->broadcast(msg_code, c_array, len_arr, c_topic, (EFdbQOS)qos, c_log_data);

    if (c_log_data)
    {
        env->ReleaseStringUTFChars(log_data, c_log_data);
    }
    if (c_array)
    {
        env->ReleaseByteArrayElements(pb_data, c_array, 0);
    }
    if (c_topic)
    {
        env->ReleaseStringUTFChars(topic, c_topic);
    }

    return ret;
}

JNIEXPORT jstring JNICALL Java_ipc_fdbus_FdbusServer_fdb_1endpoint_1name
  (JNIEnv *env, jobject, jlong handle)
{
    auto server = (CJniServer *)handle;
    const char *name = "";
   
    if (server)
    {
        name = server->name().c_str();
    }
   
   return env->NewStringUTF(name);
}

JNIEXPORT jstring JNICALL Java_ipc_fdbus_FdbusServer_fdb_1bus_1name
  (JNIEnv *env, jobject, jlong handle)
{
    auto server = (CJniServer *)handle;
    const char *name = "";
 
    if (server)
    {
        name = server->nsName().c_str();
    }
 
    return env->NewStringUTF(name);
}

JNIEXPORT jboolean JNICALL Java_ipc_fdbus_FdbusServer_fdb_1log_1enabled
  (JNIEnv *env, jobject, jlong handle, jint msg_type)
{
    auto server = (CJniServer *)handle;
    if (server)
    {
        auto logger = FDB_CONTEXT->getLogger();
        if (logger && logger->checkLogEnabled((EFdbMessageType)msg_type, 0, server))
        {
            return true;
        }
    }
    
    return false;
}

JNIEXPORT void JNICALL Java_ipc_fdbus_FdbusServer_fdb_1enable_1event_1cache
                            (JNIEnv *env, jobject, jlong handle, jboolean enable)
{
    auto server = (CJniServer *)handle;
    if (server)
    {
        server->enableEventCache(enable);
    }
}

JNIEXPORT void JNICALL Java_ipc_fdbus_FdbusServer_fdb_1init_1event_1cache
                            (JNIEnv *env,
                             jobject,
                             jlong handle,
                             jint event,
                             jstring topic,
                             jbyteArray event_data,
                             jboolean allow_event_route)
{
    auto server = (CJniServer *)handle;
    if (!server)
    {
        return;
    }
    
    const char *c_topic = 0;
    if (topic)
    {
        c_topic = env->GetStringUTFChars(topic, 0);
    }
    
    jbyte *c_array = 0;
    int len_arr = 0;
    if (event_data)
    {
        c_array = env->GetByteArrayElements(event_data, 0);
        len_arr = env->GetArrayLength(event_data);
    }

    server->initEventCache(event, c_topic, c_array, len_arr, allow_event_route);
    
    if (c_array)
    {
        env->ReleaseByteArrayElements(event_data, c_array, 0);
    }
    if (c_topic)
    {
        env->ReleaseStringUTFChars(topic, c_topic);
    }
}


static const JNINativeMethod gFdbusServerMethods[] = {
    {(char *)"fdb_create",
             (char *)"(Ljava/lang/String;)J",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1create},
    {(char *)"fdb_destroy",
             (char *)"(J)V",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1destroy},
    {(char *)"fdb_bind",
             (char *)"(JLjava/lang/String;)Z",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1bind},
    {(char *)"fdb_unbind",
             (char *)"(J)Z",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1unbind},
    {(char *)"fdb_broadcast",
             (char *)"(JILjava/lang/String;[BLjava/lang/String;I)Z",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1broadcast},
    {(char *)"fdb_endpoint_name",
             (char *)"(J)Ljava/lang/String;",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1endpoint_1name},
    {(char *)"fdb_bus_name",
             (char *)"(J)Ljava/lang/String;",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1bus_1name},
    {(char *)"fdb_log_enabled",
             (char *)"(JI)Z",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1log_1enabled},
    {(char *)"fdb_enable_event_cache",
             (char *)"(JZ)V",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1enable_1event_1cache},
    {(char *)"fdb_init_event_cache",
             (char *)"(JILjava/lang/String;[BZ)V",
             (void*) Java_ipc_fdbus_FdbusServer_fdb_1init_1event_1cache}
};
  
int register_fdbus_server(JNIEnv *env)
{
    return CGlobalParam::jniRegisterNativeMethods(env,
                         "ipc/fdbus/FdbusServer",
                         gFdbusServerMethods,
                         Fdb_Num_Elems(gFdbusServerMethods));
}
  

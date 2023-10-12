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

package ipc.fdbus;
import ipc.fdbus.FdbusClientListener;
import ipc.fdbus.SubscribeItem;
import ipc.fdbus.Fdbus;
import ipc.fdbus.FdbusMsgBuilder;

import java.util.ArrayList;

public class FdbusClient
{
    private native long fdb_create(String name);
    private native void fdb_destroy(long native_handle);
    private native boolean fdb_connect(long native_handle, String url);
    private native boolean fdb_disconnect(long native_handle);
    private native boolean fdb_invoke_async(long native_handle,
                                            int msg_code,
                                            byte[] pb_data,
                                            String log_msg,
                                            Object user_data,
                                            int timeout,
                                            int qos);
    private native FdbusMessage fdb_invoke_sync(long native_handle,
                                                int msg_code,
                                                byte[] pb_data,
                                                String log_msg,
                                                int timeout,
                                                int qos);
    private native boolean fdb_send(long native_handle,
                                    int msg_code,
                                    byte[] pb_data,
                                    String log_msg,
                                    int qos);
    private native boolean fdb_subscribe(long native_handle, 
                                         ArrayList<SubscribeItem> sub_list,
                                         int qos);
    private native boolean fdb_unsubscribe(long native_handle,
                                           ArrayList<SubscribeItem> sub_list,
                                           int qos);
    
    private native String fdb_endpoint_name(long native_handle);
    private native String fdb_bus_name(long native_handle);
    private native boolean fdb_log_enabled(long native_handle, int msg_type);

    private native boolean fdb_publish(long native_handle,
                                       int event,
                                       String topic,
                                       byte[] event_data,
                                       String log_msg,
                                       boolean force_update,
                                       int qos);
    
    private native boolean fdb_get_event_async(long native_handle,
                                               int event,
                                               String topic,
                                               Object user_data,
                                               int timeout,
                                               int qos);
    private native FdbusMessage fdb_get_event_sync(long native_handle,
                                                   int event,
                                                   String topic,
                                                   int timeout,
                                                   int qos);
	
    private long mNativeHandle;
    private FdbusClientListener mFdbusListener;

    private void initialize(String name, FdbusClientListener listener)
    {
        mNativeHandle = fdb_create(name);
        mFdbusListener = listener;
    }
    
    /*
     * create fdbus client
     * @name - name of the client for debugging; can be any string
     * @listener - callbacks to handle events from server
     */
    public FdbusClient(String name, FdbusClientListener listener)
    {
        initialize(name, listener);
    }

    FdbusClient(long handle)
    {
        mNativeHandle = handle;
        mFdbusListener = null;
    }

    /*
     * create fdbus client with default name
     */
    public FdbusClient(FdbusClientListener listener)
    {
        initialize(null, listener);
    }

    public FdbusClient(String name)
    {
        initialize(name, null);
    }

    public FdbusClient()
    {
        initialize(null, null);
    }

    /*
     * set server event listener
     * @listener - callbacks to handle events from server
     */
    public void setListener(FdbusClientListener listener)
    {
        mFdbusListener = listener;
    }

    /*
     * destroy a client
     */
    public void destroy()
    {
        long handle = mNativeHandle;
        mNativeHandle = 0;
        if (handle != 0)
        {
            fdb_destroy(handle);
        }
    }

    /*
     * connect to server
     * @url - url of server to connect in the following format:
     *     tcp://ip address:port number
     *     ipc://directory to unix domain socket
     *     svc://server name: own server name and get address dynamically
     *         allocated by name server
     */
    public boolean connect(String url)
    {
        return fdb_connect(mNativeHandle, url);
    }

    /*
     * reconnect to server
     * The client should has been connected with connect(String url) then
     *     disconnect with disconnect()
     */
    public boolean connect()
    {
        return connect("svc://" + busName());
    }

    /*
     * disconnect to server
     */
    public boolean disconnect()
    {
        return fdb_disconnect(mNativeHandle);
    }

    /*
     * invoke method call upon connected server asynchronously
     * @msg_code - message id
     * @msg - message to send (protobuf format by default)
     * @user_data - user data that will be returned at onReply()
     * @timeout - how long onReply() should be called (0 - forever)
     * The method return immediately without blocking. reply from server is
     *    received from onReply()
     */
    public boolean invokeAsync(int msg_code, Object msg, Object user_data, int timeout, int qos)
    {
        FdbusMsgBuilder builder = Fdbus.encodeMessage(msg, logEnabled(Fdbus.FDB_MT_REQUEST));
        if (builder == null)
        {
            return false;
        }

        return fdb_invoke_async(mNativeHandle,
                                msg_code,
                                builder.toBuffer(),
                                builder.toString(),
                                user_data,
                                timeout,
                                qos);
    }

    public boolean invokeAsync(int msg_code, Object msg, Object user_data)
    {
        return invokeAsync(msg_code, msg, user_data, 0, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    /*
     * invoke method call upon connected server synchronously
     * @msg_code - message id
     * @msg - message to send (protobuf format by default)
     * @timeout - how long onReply() should be called (0 - forever)
     * The method blocks until server replies or timer expires.
     */
    public FdbusMessage invokeSync(int msg_code, Object msg, int timeout, int qos)
    {
        FdbusMsgBuilder builder = Fdbus.encodeMessage(msg, logEnabled(Fdbus.FDB_MT_REQUEST));
        if (builder == null)
        {
            return null;
        }
        
        return fdb_invoke_sync(mNativeHandle,
                               msg_code,
                               builder.toBuffer(),
                               builder.toString(),
                               timeout,
                               qos);
    }

    public FdbusMessage invokeSync(int msg_code, Object msg)
    {
        return invokeSync(msg_code, msg, 0, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    /*
     * send message to server without reply expected (one-shot)
     * @msg_code - message id
     * @msg - message to send (protobuf format by default)
     * The method sends message to server and return immediately without reply
     *      from server
     */
    public boolean send(int msg_code, Object msg, int qos)
    {
        FdbusMsgBuilder builder = Fdbus.encodeMessage(msg, logEnabled(Fdbus.FDB_MT_REQUEST));
        if (builder == null)
        {
            return false;
        }
        
        return fdb_send(mNativeHandle,
                        msg_code,
                        builder.toBuffer(),
                        builder.toString(),
                        qos);
    }

    public boolean send(int msg_code, Object msg)
    {
        return send(msg_code, msg, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    /*
     * subscribe a list of events from server
     * @sub_list - list of events to be subscribed
     * The method is typically called at onOnline() to subscribe a list of
     *     events when a server is connected
     */
    public boolean subscribe(ArrayList<SubscribeItem> sub_list, int qos)
    {
        return fdb_subscribe(mNativeHandle, sub_list, qos);
    }

    public boolean subscribe(ArrayList<SubscribeItem> sub_list)
    {
        return subscribe(sub_list, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    /*
     * unsubscribe a list of events from server
     * @sub_list - list of events to be unsubscribed
     */
    public boolean unsubscribe(ArrayList<SubscribeItem> sub_list, int qos)
    {
        return fdb_unsubscribe(mNativeHandle, sub_list, qos);
    }

    public boolean unsubscribe(ArrayList<SubscribeItem> sub_list)
    {
        return unsubscribe(sub_list, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    /*
     * get endpoint name of the client
     */
    public String endpointName()
    {
        return fdb_endpoint_name(mNativeHandle);
    }

    /*
     * get bus name the client is connected
     * Note that only the client connected with svc://svc_name have bus name,
     * , e.g., svc_name
     */
    public String busName()
    {
        return fdb_bus_name(mNativeHandle);
    }

    public boolean logEnabled(int msg_type)
    {
        return fdb_log_enabled(mNativeHandle, msg_type);
    }

    public boolean publish(int event, String topic, Object msg, boolean always_update, int qos)
    {
        FdbusMsgBuilder builder = Fdbus.encodeMessage(msg, logEnabled(Fdbus.FDB_MT_REQUEST));
        if (builder == null)
        {
            return false;
        }
        return fdb_publish(mNativeHandle,
                           event,
                           topic,
                           builder.toBuffer(),
                           builder.toString(),
                           always_update,
                           qos);
    }

    public boolean publish(int event, String topic, Object msg)
    {
        return publish(event, topic, msg, false, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    public boolean getAsync(int event, String topic, Object user_data, int timeout, int qos)
    {
        return fdb_get_event_async(mNativeHandle, event, topic, user_data, timeout, qos);
    }

    public boolean getAsync(int event, String topic, Object user_data)
    {
        return getAsync(event, topic, user_data, 0, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    public FdbusMessage getSync(int event, String topic, int timeout, int qos)
    {
        return fdb_get_event_sync(mNativeHandle, event, topic, timeout, qos);
    }

    public FdbusMessage getSync(int event, String topic)
    {
        return getSync(event, topic, 0, Fdbus.FDB_QOS_TRY_SECURE_RELIABLE);
    }

    private void callbackOnline(int sid, int qos)
    {
        if (mFdbusListener != null)
        {
            try {
                mFdbusListener.onOnline(sid, qos);
            } catch (Exception e) {
                System.out.println(e);
            }
        }
    }
    
    private void callbackOffline(int sid, int qos)
    {
        if (mFdbusListener != null)
        {
            try {
                mFdbusListener.onOffline(sid, qos);
            } catch (Exception e) {
                System.out.println(e);
            }
        }
    }
    
    private void callbackReply(int sid,
                               int msg_code,
                               byte[] payload,
                               int status,
                               Object user_data)
    {
        if (mFdbusListener != null)
        {
            FdbusMessage msg = new FdbusMessage(sid, msg_code, payload, user_data, status);
            try {
                mFdbusListener.onReply(msg);
            } catch (Exception e) {
                System.out.println(e);
            }
        }
    }

private void callbackGetEvent(int sid,
                              int msg_code,
                              String topic,
                              byte[] payload,
                              int status,
                              Object user_data)
{
    if (mFdbusListener != null)
    {
        FdbusMessage msg = new FdbusMessage(sid, msg_code, topic, payload, user_data, status);
        try {
            mFdbusListener.onGetEvent(msg);
        } catch (Exception e) {
            System.out.println(e);
        }
    }
}
    
    private void callbackBroadcast(int sid,
                                   int msg_code,
                                   String topic,
                                   byte[] payload)
    {   
        if (mFdbusListener != null)
        {
            FdbusMessage msg = new FdbusMessage(sid, msg_code, payload);
            msg.topic(topic);
            try {
                mFdbusListener.onBroadcast(msg);
            } catch (Exception e) {
                System.out.println(e);
            }
        }
    }
}


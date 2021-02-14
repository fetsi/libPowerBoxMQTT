#include "PowerBox3PX.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "pahomqtt/MQTTClient.h"
#include "json/json.hpp"
#include <memory>


using namespace std;
using json = nlohmann::json;



class PowerBox3PX::impl {

private:
    const string host;
    const uint16_t port;
    const string clientId;
    const string user;
    const string pwd;
    const topics pubTopics;
    const string subTopic;
    bool verbose;
    MQTTClient client;
    int sock1_st;
    int sock2_st;
    int sock3_st;
    PoweBox3PXAsyncNotifiable *notifiable;
    volatile MQTTClient_deliveryToken deliveredtoken; 

    /*-------------------------------------------------------------------------------------------*/
    /*Private utility functions*/

    /**
     *@brief Sets the deliveredToken member of the PowerBox3PX::impl object.
     *@details Used by the messageDelivered() callback to set the deliveredToken member
     *which can then be compared against the message-specific token set by MQTTClient_publishMessage()
     *in the thread that initiated the message sending.
     **/
    void setDeliveredToken(MQTTClient_deliveryToken dt) {
        deliveredtoken = dt;
    }

    /**
     *@brief Handles the PowerBox3PX-specific handling of the MQTT message
     *@details Calls the parser function to extract current socket states from the message
     *and compares them to previously cached states to determine if some of the sockets have
     *changed state.
     @param[in] msg Reference to the message payload string that was received
     **/    
    void handleMessage(const string &msg) {
        /*Cache the socket states before the message is parsed*/
        int sock1_prev_st = sock1_st;
        int sock2_prev_st = sock2_st;
        int sock3_prev_st = sock3_st;

        parseSockStates(msg);

        /*Invoke events if socket states have changed*/
        if(sock1_st != sock1_prev_st) {
            if(verbose) cout << "Socket 1 changed state from " << sock1_prev_st << " to " << sock1_st << endl;
            notifiable->receiveSocketEvent(1, sock1_st);
        }
        if(sock2_st != sock2_prev_st) {
            if(verbose) cout << "Socket 2 changed state from " << sock2_prev_st << " to " << sock2_st << endl;
            notifiable->receiveSocketEvent(2, sock2_st);
        }
        if(sock3_st != sock3_prev_st) {
            if(verbose) cout << "Socket 3 changed state from " << sock3_prev_st << " to " << sock3_st << endl;
            notifiable->receiveSocketEvent(3, sock3_st);
        }
    }



    /**
     *@brief Parses socket states from a JSON string and stores them in appropriate fields in the PowerBox3PX::impl object 
     *@details Attempts to catch all exceptions thrown by the json library. If an exception occurs
     *when parsing an element within the Sockets array, the error is printed to stderr and the function
     *attempts to continue parsing the rest of the sockets if they  exist in the array.
     *@returns 0 if all socket states were successfully parsed, a non-zero integer otherwise. 
     **/
    int parseSockStates(const string &json_str) {
        json js;
        bool gotSock1, gotSock2, gotSock3;
        try {
            js = json::parse(json_str);
        }
        catch(...) {
            cerr << "Failed to parse MQTT message " << json_str << " as JSON" << endl;
            return 1;
        }  
        if(js.contains("Outputs") == false) {
            cerr << "JSON object doesn't contain an \"Outputs\" element" << endl;
            return 2;
        }
        auto sockets = js["Outputs"];
        if(js["Outputs"].is_array() == false || js["Outputs"].size() != 3) {
            cerr << "\"Outputs\" element is not an array of size 3" << endl;
            return 3;
        }
        for(auto s : sockets) {
            if(s.contains("ID") == false) {
                cerr << "Element of Sockets array doesn't contain ID" << endl;
                continue;
            }
            if(s.contains("State") == false) {
                cerr << "Element of Sockets array doesn't contain State" << endl;
                continue;
            }

            int sid, state;
            try {
                sid = s["ID"].get<int>();
                state = s["State"].get<int>();
            }
            catch(nlohmann::detail::type_error &e) {
                cerr << "Error in parsing a socket: " << e.what() << endl;
            }
            if(state != 0 && state != 1) {
                cerr << "Invalid socket state " << state << endl;
                continue;
            }

            switch(sid) {
            case 1:
                sock1_st = state;
                gotSock1 = true;
                break;
            case 2:
                sock2_st = state;
                gotSock2 = true;
                break;
            case 3:
                sock3_st = state;
                gotSock3 = true;
                break;
            default:
                cerr << "Invalid socket ID " << sid << endl;
                continue;
            }
        }
        if(!gotSock1 || !gotSock2 || !gotSock3) {
            return 4;
        }
        return 0;    
    }




public:
    /*-------------------------------------------------------------------------------------------*/
    /*Public functions that are part of the PowerBox3PX public interface*/

    impl(string _host, uint16_t _port, string _clientId, string _user, string _pwd,
        topics _pubTopics, std::string _subTopic, bool _verbose) : 
        host(_host), port(_port), clientId(_clientId), user(_user), pwd(_pwd), 
        pubTopics(_pubTopics), subTopic(_subTopic), verbose(_verbose),
        sock1_st(SOCKET_UNINITIALIZED), sock2_st(SOCKET_UNINITIALIZED), 
        sock3_st(SOCKET_UNINITIALIZED), deliveredtoken(0)
        {
            string url("tcp://" + host + ":" + to_string(port));

            if(MQTTClient_create(&client, url.c_str(), clientId.c_str(),
                MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS ) {
                throw std::runtime_error("Failed to create MQTT client");
            }
            if(MQTTClient_setCallbacks(client, this, connectionLost, messageArrived, messageDelivered) != MQTTCLIENT_SUCCESS) {
                throw std::runtime_error("Failed to set MQTT callbacks");
            }
        }

    ~impl() { 
        MQTTClient_destroy(&client);
    }

    int connect() {
        MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
        opts.keepAliveInterval = 20;
        opts.cleansession = 1;
        opts.username = user.c_str();
        opts.password = pwd.c_str();

        
        int rc = MQTTClient_connect(client, &opts);
        if(rc != MQTTCLIENT_SUCCESS) {
            return rc;
        }
        return MQTTCLIENT_SUCCESS;
    }

    int disconnect() {
        return MQTTClient_disconnect(client, 10000);
    }

    int monitor(PoweBox3PXAsyncNotifiable *_notifiable) {
        if(!_notifiable ) {
            cerr << "Pointer to PoweBox3PXAsyncNotifiable must be non-null" << endl;
            return -1;
        }
        notifiable = _notifiable;
        if(verbose) cout << "Subscribing to topic " << endl << subTopic << endl;
        return MQTTClient_subscribe(client, subTopic.c_str(), 1);
    }

    int setSocket(int socketNum, int state) {
        string topic;
        char *msgdata;
        MQTTClient_deliveryToken token;

        /*Choose topic according to socket number*/
        switch(socketNum) {
        case 1:
            topic = pubTopics.soc1_topic;
            break;
        case 2:
            topic = pubTopics.soc2_topic;
            break;
        case 3:
            topic = pubTopics.soc3_topic;
            break;
        default:
            return 1;
        }
        MQTTClient_message msg = MQTTClient_message_initializer;
        /*Set message payload according to new socket state*/
        switch(state) {
            case SOCKET_OFF:
            case SOCKET_ON: {
                size_t len = to_string(state).size() + 1; /*Size of actual msg string + terminating \0*/
                msgdata = static_cast<char*>(malloc(len)); 
                if(!msgdata) {
                    cerr << "Failed to allocate memory for msg payload" << endl;
                    return 2;
                }
                snprintf(msgdata, len, "%d", state);
                msg.payload = static_cast<void*>(msgdata);
                msg.payloadlen = len;
                break;
            }
            default:
                cerr << "Invalid socket command " << state << endl;
                return 3;
        }
        msg.qos = 1;
        msg.retained = 0;
        deliveredtoken = 0;
        MQTTClient_publishMessage(client, topic.c_str(), &msg, &token);
        if(verbose) {
            cout << "Waiting for publication of " << "\"" << (const char*)msgdata << "\"" <<
            " on topic " << topic << " for client " << clientId << " with token " << token << endl;
        }
        while(deliveredtoken != token);
        free(msgdata);
        return 0;
    }




    /*-------------------------------------------------------------------------------------------*/
    /*Static callback methods for libPaho async events*/
    
    static void connectionLost(void *context, char *cause) {
        PowerBox3PX::impl *instance = (PowerBox3PX::impl*) context;
        if(instance->verbose) cout << "Connection to MQTT broker lost: " << cause << endl;
    }

    static int messageArrived(void *context, char *topic, int topicLen, MQTTClient_message *msg) {
        PowerBox3PX::impl *instance = (PowerBox3PX::impl*) context;
        string payload_str((const char*) msg->payload, msg->payloadlen);

        if(instance->verbose) {
            cout << "Received a message on topic " << "\"" << topic << "\"" << ":" 
            << "\"" << payload_str << "\"" << endl;
        }

        instance->handleMessage(payload_str);        
        MQTTClient_freeMessage(&msg);
        MQTTClient_free(topic);
        return 1;
    }

    static void messageDelivered(void *context, MQTTClient_deliveryToken dt) {
        PowerBox3PX::impl *instance = (PowerBox3PX::impl*) context;
        if(!dynamic_cast<PowerBox3PX::impl*>(instance)) {
            cerr << "Context is of invalid type!" << endl;
            return;
        }
        if(instance->verbose) cout << "Message with token " << dt << " delivered successfully" << endl;
        instance->setDeliveredToken(dt);
    }
};







/*-------------------------------------------------------------------------------------------*/
/*Implementation stubs for the PowerBox3PX public interface functions that redirect the call to
*the impl object*/

PowerBox3PX::PowerBox3PX(string brokerIP, uint16_t brokerPort, string clientId, string username, string password, 
    topics publishTopics, string subscribeTopic, bool verbose) : 
    pImpl(std::make_unique<impl>(brokerIP, brokerPort, clientId, username, password, publishTopics, subscribeTopic, verbose)) {}

PowerBox3PX:: ~PowerBox3PX() = default;

int PowerBox3PX::connect() { return pImpl->connect(); }
int PowerBox3PX::disconnect() { return pImpl->disconnect(); }

int PowerBox3PX::monitor(PoweBox3PXAsyncNotifiable *_notifiable) {return pImpl->monitor(_notifiable);}
int PowerBox3PX::setSocket(int socketNum, int state) { return pImpl->setSocket(socketNum, state); }


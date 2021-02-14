#ifndef POWERBOX_3PX_H
#define POWERBOX_3PX_H

#include <memory>
#include <experimental/propagate_const>
#include <string>
#include <cstdint>


struct topics {
    std::string soc1_topic;
    std::string soc2_topic;
    std::string soc3_topic;
};

enum socketState {
    SOCKET_UNINITIALIZED = -1,
    SOCKET_OFF = 0,
    SOCKET_ON  = 1
};


/**
 *@brief Interface implemented by classes that wish to be notified about changes in socket states
 *@details An Abstract Base Class that must be inherited by the class of which an instance is passed
 *to the monitor() function of PoweBox3PX class
 **/
class PoweBox3PXAsyncNotifiable {
public:
    PoweBox3PXAsyncNotifiable() {}
    virtual ~PoweBox3PXAsyncNotifiable() {}

    virtual void receiveSocketEvent(int socket, int state) = 0;
};



class PowerBox3PX {

class impl; /*Forward declare implementation class*/
std::experimental::propagate_const<std::unique_ptr<impl>> pImpl; /*Const-propagating smart pointer to implementation*/

public:
    PowerBox3PX(std::string brokerIP, uint16_t brokerPort, std::string clientId, std::string username, std::string password,
        topics publishTopics, std::string subscribeTopic, bool verbose);
    ~PowerBox3PX();

    /**
     *@brief Attempts to connect the client to an MQTT broker
     *@details Uses the parameters that were supplied to the constructor of this object
     *@returns 0 on success, a non-zero integer on failure. 
     *1: Connection refused: Unacceptable protocol version
     *2: Connection refused: Identifier rejected
     *3: Connection refused: Server unavailable
     *4: Connection refused: Bad user name or password
     *5: Connection refused: Not authorized
     **/
    int connect();

    /**
     *@brief Tells the library to start monitoring on the state of the sockets
     *@details Internally, this means that the library subscribes to the MQTT topics
     *inside the topics struct that was provided in the class constructor
     *@param[in] notifiable Pointer to a PoweBox3PXAsyncNotifiable -derived object that will be notified
     *when changes in socket states occur. Note that the library invokes another thread for monitoring
     *the sockets and thus calls to functions of notifiable will be made from a different thread than
     *the one from which monitor() was called. Synchronizing the threads must be done by the calling
     *application if necessary.
     *@returns 0 on success, a non-zero integer on failure
     **/
    int monitor(PoweBox3PXAsyncNotifiable *_notifiable);

    /**
     *@brief Attempts to disconnect the client from the MQTT broker
     *@returns 0 on success, a non-zero integer on failure
     **/
    int disconnect();

    /**
     *@brief Attempts to update the state of a given socket by publishing a MQTT message to the socket's topic
     *@param[in] socketNum Number of the socket. Must be 1,2 or 3.
     *@param[in] state New socket state. Must be 0 or 1
     *@returns 0 on success, a non-zero integer on failure
     **/
    int setSocket(int socketNum, int state);

};






#endif
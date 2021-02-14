#include "PowerBox3PX.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <memory>


class Watcher : public PoweBox3PXAsyncNotifiable {
public:
    Watcher() : a(0) {}
    ~Watcher() {}


    void receiveSocketEvent(int socket, int state) {
        std::cout << "ReceiveSocketEvent: socket " << socket << " state " << state << std::endl;
        a = state;

        std::cout << "a is now " << a <<std::endl;
        
    }

private:
    int a;

};


int main(int argc, char *argv[]) {

    topics pubt = {
        "netio/PowerBOX-B7/output/1/action", 
        "netio/PowerBOX-B7/output/2/action", 
        "netio/PowerBOX-B7/output/3/action" 
    };
    std::string subt("devices/PowerBOX-B7/messages/devicebound/");
    PowerBox3PX pb("192.168.100.15", 1883, "controller_app", "powerbox-b7", "jaska123", pubt, subt, false);
    int rc;
    rc = pb.connect();
    if(rc == 0) {
        std::cout << "Connected to broker!" << std::endl;
    }
    else {
        std::cerr << "Failed to connect to broker. Return code " << rc << std::endl;
        return 2;
    }

    std::unique_ptr<Watcher> w = std::make_unique<Watcher>();
    pb.monitor((PoweBox3PXAsyncNotifiable*) w.get());

    
    std::string sockStr;
    std::string stStr;
    int sockNum;
    int stNum;
    do {
        std::cout << "Give socket number (Q or q to exit): ";
        std::cin >> sockStr;
        
        if(sockStr.compare("1") == 0) {
            sockNum = 1;
        }
        else if(sockStr.compare("2") == 0) {
            sockNum = 2;
        }
        else if(sockStr.compare("3") == 0) {
            sockNum = 3;
        }
        else {
            std::cerr << "Invalid socket number" << std::endl;
            continue;
        }

        while(true) {
            std::cout << "Give socket state: ";
            std::cin >> stStr;

            if(stStr.compare("0") == 0) {
                stNum = 0;
                break;
            }
            else if(stStr.compare("1") == 0) {
                stNum = 1;
                break;
            }
            else {
                std::cerr << "Invalid socket state" << std::endl;
                continue;           
            }
        }
        pb.setSocket(sockNum, stNum);
        

    } while (sockStr.compare("Q") && sockStr.compare("q"));

    

    rc = pb.disconnect();
    if(rc == 0) {
        std::cout << "Disconnected from broker!" << std::endl;
    }
    else {
        std::cerr << "Failed to disconnect from broker. Return code " << rc << std::endl;
        return 3;
    }
    
    return 0;

}

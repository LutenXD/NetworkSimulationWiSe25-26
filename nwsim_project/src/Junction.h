//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __CROSSTRAFFIC_JUNCTION_H_
#define __CROSSTRAFFIC_JUNCTION_H_

#include <omnetpp.h>
#include <queue>
#include <string>

using namespace omnetpp;

enum JunctionMode {
    QUEUE_MODE,      // Original queueing logic with timing penalties
    CIRCLE_MODE,     // Simple circular routing with no queue
    ROUNDROBIN_MODE  // Round-robin direction scheduling
};

struct QueuedVehicle {
    cMessage* msg;
    std::string arrivalDirection;
    simtime_t queueEnterTime;
    
    QueuedVehicle(cMessage* m, const std::string& dir) 
        : msg(m), arrivalDirection(dir), queueEnterTime(simTime()) {}
};

class Junction : public cSimpleModule {
public:
    virtual ~Junction();
    
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);
    
private:
    // Mode configuration
    JunctionMode junctionMode;
    
    // Queue mode variables
    std::queue<QueuedVehicle> vehicleQueue;
    cMessage* roundaboutTimer = nullptr;
    std::string lastProcessedDirection;
    bool isProcessing;
    
    // Circle mode variables
    cMessage* circleTimer = nullptr;
    
    // Round-robin mode variables
    cMessage* roundRobinTimer = nullptr;
    std::string currentOpenDirection;
    int directionIndex; // 0=north, 1=east, 2=south, 3=west
    std::queue<QueuedVehicle> northQueue;
    std::queue<QueuedVehicle> eastQueue;
    std::queue<QueuedVehicle> southQueue;
    std::queue<QueuedVehicle> westQueue;
    
    // Statistics signals
    simsignal_t queueLengthSignal;
    simsignal_t junctionProcessingTimeSignal;
    simsignal_t vehicleCountSignal;
    
    // Common methods
    std::string getNextHop(const std::string& destination, const std::string& arrivalGate);
    std::string getCounterClockwiseExit(const std::string& arrivalGate);
    bool isExitJunction(const std::string& destination);
    std::string getDirectionFromGate(const std::string& gateName);
    // Routing helpers for choosing shortest path around roundabout
    std::string getJunctionForEndpoint(const std::string& endpoint);
    std::string getClockwiseExit(const std::string& arrivalGate);
    std::string getBestNextGateTowards(const std::string& destination, const std::string& arrivalGate);
    
    // Queue mode methods
    void processVehicleQueue();
    
    // Circle mode methods
    void processCircleMode(cMessage* msg);
    std::string getCircularRoute(const std::string& arrivalGate);
    
    // Round-robin mode methods
    void processRoundRobinMode(cMessage* msg);
    void switchToNextDirection();
    void processDirectionQueue(const std::string& direction);
    std::queue<QueuedVehicle>& getDirectionQueue(const std::string& direction);
};

#endif

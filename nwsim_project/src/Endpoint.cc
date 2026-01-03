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

#include "Endpoint.h"
#include "Vehicle_m.h"
#include <random>
#include <algorithm>

Define_Module(Endpoint);

unsigned int Endpoint::vehicleCounter = 0;
std::vector<std::string> Endpoint::allEndpoints = {
    "EndNW", "EndNE", "EndEN", "EndES",
    "EndSE", "EndSW", "EndWS", "EndWN"
};

Endpoint::~Endpoint()
{
    cancelAndDelete(spawnTimer);
}

void Endpoint::initialize()
{
    spawnTimer = new cMessage("spawnTimer");
    
    // Initialize statistics signals
    travelTimeSignal = registerSignal("travelTime");
    junctionTimeSignal = registerSignal("junctionTime");
    junctionCountSignal = registerSignal("junctionCount");
    
    simtime_t spawnInterval = par("spawnInterval");
    simtime_t spawnOffset = par("spawnOffset");
    if (spawnInterval > 0) {
        ASSERT(spawnOffset >= 0);
        scheduleAt(simTime() + spawnOffset, spawnTimer);
    }
}

void Endpoint::handleMessage(cMessage* msg)
{
    if (msg == spawnTimer) {
        spawnVehicle();
        simtime_t spawnInterval = par("spawnInterval");
        ASSERT(spawnInterval > 0);
        scheduleAt(simTime() + spawnInterval, spawnTimer);
    }
    else if (dynamic_cast<Vehicle*>(msg)) {
        Vehicle* veh = dynamic_cast<Vehicle*>(msg);
        ASSERT(std::string(veh->getDstEndpoint()) == std::string(getName()));
        
        // Calculate and emit travel time statistics
        simtime_t travelTime = simTime() - veh->getStartTime();
        simtime_t totalJunctionTime = veh->getTotalJunctionTime();
        long junctionCount = veh->getJunctionCount();
        
        emit(travelTimeSignal, travelTime);
        emit(junctionTimeSignal, totalJunctionTime);
        emit(junctionCountSignal, junctionCount);
        
        EV << "Vehicle " << veh->getVehNumber() << " arrived at " << getName() 
           << ". Travel time: " << travelTime << "s, Junction time: " << totalJunctionTime 
           << "s, Junctions visited: " << junctionCount << endl;
           
        delete msg;
    }
}

void Endpoint::spawnVehicle()
{
    Vehicle* veh = new Vehicle();
    veh->setVehNumber(vehicleCounter++);
    veh->setSrcEndpoint(getName());
    std::string dstEndpoint = getDstEndpoint();
    veh->setDstEndpoint(dstEndpoint.c_str());
    veh->setName(std::string("for " + dstEndpoint).c_str());
    
    // Initialize timing information
    veh->setStartTime(simTime());
    veh->setTotalJunctionTime(0);
    veh->setJunctionCount(0);
    
    EV << "Spawning vehicle " << veh->getVehNumber() << " from " << getName() 
       << " to " << dstEndpoint << " at time " << simTime() << endl;
       
    send(veh, "conn$o");
}

std::string Endpoint::getDstEndpoint()
{
    std::string myName = getName();

    // Create a copy of all endpoints and remove the current endpoint
    std::vector<std::string> availableEndpoints = allEndpoints;
    availableEndpoints.erase(
        std::remove(availableEndpoints.begin(), availableEndpoints.end(), myName),
        availableEndpoints.end()
    );

    // Use OMNeT++'s random number generator for reproducibility
    int randomIndex = intuniform(0, availableEndpoints.size() - 1);
    return availableEndpoints[randomIndex];
}

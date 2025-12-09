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

Define_Module(Endpoint);

unsigned int Endpoint::vehicleCounter = 0;
std::map<char, std::string> Endpoint::directionMapping = {
    {'N', "S"},
    {'S', "N"},
    {'E', "W"},
    {'W', "E"},
};

Endpoint::~Endpoint()
{
    cancelAndDelete(spawnTimer);
}

void Endpoint::initialize()
{
    spawnTimer = new cMessage("spawnTimer");
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
    send(veh, "conn$o");
}

std::string Endpoint::getDstEndpoint()
{
    std::string myName = getName();
    std::string dstName = "End" + directionMapping.at(myName.at(3));
    dstName.push_back(myName.at(4));
    ASSERT(myName.size() == dstName.size());
    return dstName;
}

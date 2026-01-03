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

#include "Junction.h"
#include "Vehicle_m.h"

Define_Module(Junction);

Junction::~Junction()
{
    cancelAndDelete(roundaboutTimer);
    cancelAndDelete(circleTimer);
    cancelAndDelete(roundRobinTimer);
    
    // Clean up any remaining vehicles in queue (only for queue mode)
    while (!vehicleQueue.empty()) {
        delete vehicleQueue.front().msg;
        vehicleQueue.pop();
    }
    
    // Clean up round-robin queues
    while (!northQueue.empty()) {
        delete northQueue.front().msg;
        northQueue.pop();
    }
    while (!eastQueue.empty()) {
        delete eastQueue.front().msg;
        eastQueue.pop();
    }
    while (!southQueue.empty()) {
        delete southQueue.front().msg;
        southQueue.pop();
    }
    while (!westQueue.empty()) {
        delete westQueue.front().msg;
        westQueue.pop();
    }
}

void Junction::initialize()
{
    // Read junction mode from configuration
    const char* modeStr = par("junctionMode").stringValue();
    if (strcmp(modeStr, "circle") == 0) {
        junctionMode = CIRCLE_MODE;
        EV << "Junction " << getName() << ": Using circle mode (no queue)" << endl;
    } else if (strcmp(modeStr, "roundrobin") == 0) {
        junctionMode = ROUNDROBIN_MODE;
        EV << "Junction " << getName() << ": Using round-robin mode" << endl;
    } else {
        junctionMode = QUEUE_MODE;
        EV << "Junction " << getName() << ": Using queue mode (with timing penalties)" << endl;
    }
    
    roundaboutTimer = new cMessage("roundaboutTimer");
    circleTimer = new cMessage("circleTimer");
    roundRobinTimer = new cMessage("roundRobinTimer");
    lastProcessedDirection = "";
    isProcessing = false;
    
    // Initialize statistics signals
    queueLengthSignal = registerSignal("queueLength");
    junctionProcessingTimeSignal = registerSignal("junctionProcessingTime");
    vehicleCountSignal = registerSignal("vehicleCount");
    
    // Initialize round-robin mode
    if (junctionMode == ROUNDROBIN_MODE) {
        directionIndex = 0; // Start with north
        currentOpenDirection = "north";
        scheduleAt(simTime() + 30.0, roundRobinTimer); // First 30-second interval
        EV << "Round-robin mode: Starting with direction " << currentOpenDirection << endl;
    }
}

void Junction::handleMessage(cMessage* msg)
{
    if (msg == roundaboutTimer) {
        // Process the next vehicle in the queue (queue mode only)
        processVehicleQueue();
    }
    else if (msg == circleTimer) {
        // Timer expired for circle mode - this shouldn't happen as we send immediately
        EV << "Circle timer expired unexpectedly" << endl;
    }
    else if (msg == roundRobinTimer) {
        // Switch to next direction in round-robin mode
        switchToNextDirection();
        scheduleAt(simTime() + 30.0, roundRobinTimer); // Schedule next 30-second interval
    }
    else {
        Vehicle* veh = dynamic_cast<Vehicle*>(msg);
        if (!veh) {
            EV << "Received non-Vehicle message" << endl;
            delete msg;
            return;
        }
        
        if (junctionMode == CIRCLE_MODE) {
            processCircleMode(msg);
        } else if (junctionMode == ROUNDROBIN_MODE) {
            processRoundRobinMode(msg);
        } else {
            // Original queue mode logic - REMOVED ENTRY PENALTIES
            std::string arrivalGate = msg->getArrivalGate()->getName();
            std::string direction = getDirectionFromGate(arrivalGate);
            
            EV << "Junction " << getName() << ": Vehicle for " << veh->getDstEndpoint() 
               << " entering roundabout from " << direction << " (" << arrivalGate << ")" << endl;
            
            // Create queued vehicle entry
            QueuedVehicle queuedVeh(msg, direction);
            
            bool wasEmpty = vehicleQueue.empty();
            bool processImmediately = wasEmpty && !isProcessing;
            
            vehicleQueue.push(queuedVeh);
            
            // Emit queue length statistics
            emit(queueLengthSignal, (long)vehicleQueue.size());
            
            if (processImmediately) {
                // Vehicle processed immediately - no penalties
                EV << "Vehicle processed immediately, no delay penalties" << endl;
                isProcessing = true;
                scheduleAt(simTime() + 3.0, roundaboutTimer); // Base 3-second processing
            } else if (wasEmpty) {
                // First vehicle in queue - no entry penalty, just base processing
                EV << "First vehicle in queue, no entry penalty" << endl;
                scheduleAt(simTime() + 3.0, roundaboutTimer); // Just 3s processing
            } else {
                // Vehicle added to existing queue - no entry penalty
                EV << "Vehicle added to queue (size: " << vehicleQueue.size() << "), no entry penalty" << endl;
            }
        }
    }
}

void Junction::processVehicleQueue()
{
    if (vehicleQueue.empty()) {
        isProcessing = false;
        return;
    }
    
    isProcessing = true;
    QueuedVehicle queuedVeh = vehicleQueue.front();
    vehicleQueue.pop();
    
    cMessage* msg = queuedVeh.msg;
    Vehicle* veh = dynamic_cast<Vehicle*>(msg);
    std::string destination = veh->getDstEndpoint();
    std::string arrivalGate = msg->getArrivalGate()->getName();
    std::string currentDirection = queuedVeh.arrivalDirection;
    
    // Calculate junction processing time and update vehicle
    simtime_t junctionProcessingTime = simTime() - queuedVeh.queueEnterTime;
    veh->setTotalJunctionTime(veh->getTotalJunctionTime() + junctionProcessingTime);
    veh->setJunctionCount(veh->getJunctionCount() + 1);
    
    // Emit statistics
    emit(junctionProcessingTimeSignal, junctionProcessingTime);
    emit(vehicleCountSignal, (long)veh->getVehNumber());
    
    // Calculate if this vehicle was processed immediately (no queue time)
    bool wasProcessedImmediately = (simTime() - queuedVeh.queueEnterTime) < 0.1; // Small epsilon for immediate processing
    
    std::string outputGate;
    
    // Check if this junction is the exit point for the destination
    if (isExitJunction(destination)) {
        // Exit the roundabout to the destination endpoint
        outputGate = getNextHop(destination, arrivalGate);
        EV << "Junction " << getName() << ": Vehicle for " << destination 
           << " exiting roundabout to " << outputGate << endl;
    } else {
        // Continue counter-clockwise in the roundabout
        outputGate = getCounterClockwiseExit(arrivalGate);
        EV << "Junction " << getName() << ": Vehicle for " << destination 
           << " continuing counter-clockwise to " << outputGate << endl;
    }
    
    // Calculate exit delay
    double exitDelay = 0.0;
    if (!wasProcessedImmediately) {
        // Check if next vehicle is from same direction for exit penalty removal
        bool sameDirectionNext = false;
        if (!vehicleQueue.empty()) {
            sameDirectionNext = (vehicleQueue.front().arrivalDirection == currentDirection);
        }
        
        if (!sameDirectionNext) {
            exitDelay = 3.0; // Add 3-second exit penalty
            EV << "Adding 3s exit penalty (next vehicle from different direction or no next vehicle)" << endl;
        } else {
            EV << "Removing exit penalty (next vehicle from same direction: " << currentDirection << ")" << endl;
        }
    }
    
    // Send the vehicle (with delay factored into next vehicle scheduling)
    send(msg, outputGate.c_str());
    
    // Update last processed direction
    lastProcessedDirection = currentDirection;
    
    // Schedule next vehicle if queue is not empty
    if (!vehicleQueue.empty()) {
        double nextDelay = 3.0 + exitDelay; // Base processing time + exit delay from current vehicle
        // NO ENTRY PENALTY for next vehicle
        
        scheduleAt(simTime() + nextDelay, roundaboutTimer);
    } else {
        isProcessing = false;
    }
    
    // Emit updated queue length
    emit(queueLengthSignal, (long)vehicleQueue.size());
}

bool Junction::isExitJunction(const std::string& destination)
{
    std::string junctionName = getName();
    
    // Determine if this junction is the correct exit point for the destination
    if (junctionName == "JuncNW") {
        return (destination == "EndNW" || destination == "EndWN");
    }
    else if (junctionName == "JuncNE") {
        return (destination == "EndNE" || destination == "EndEN");
    }
    else if (junctionName == "JuncSE") {
        return (destination == "EndSE" || destination == "EndES");
    }
    else if (junctionName == "JuncSW") {
        return (destination == "EndSW" || destination == "EndWS");
    }
    
    return false;
}

std::string Junction::getCounterClockwiseExit(const std::string& arrivalGate)
{
    std::string junctionName = getName();
    
    // Counter-clockwise movement in the roundabout
    // The roundabout flow: JuncNW -> JuncNE -> JuncSE -> JuncSW -> JuncNW
    
    if (junctionName == "JuncNW") {
        // From JuncNW, counter-clockwise goes to JuncSW (south)
        return "south$o";
    }
    else if (junctionName == "JuncNE") {
        // From JuncNE, counter-clockwise goes to JuncNW (west)
        return "west$o";
    }
    else if (junctionName == "JuncSE") {
        // From JuncSE, counter-clockwise goes to JuncNE (north)
        return "north$o";
    }
    else if (junctionName == "JuncSW") {
        // From JuncSW, counter-clockwise goes to JuncSE (east)
        return "east$o";
    }
    
    // Fallback
    return "north$o";
}

std::string Junction::getNextHop(const std::string& destination, const std::string& arrivalGate)
{
    std::string junctionName = getName();
    
    // Direct routing to endpoints from their adjacent junctions
    if (junctionName == "JuncNW") {
        if (destination == "EndNW") return "north$o";
        else if (destination == "EndWN") return "west$o";
    }
    else if (junctionName == "JuncNE") {
        if (destination == "EndNE") return "north$o";
        else if (destination == "EndEN") return "east$o";
    }
    else if (junctionName == "JuncSW") {
        if (destination == "EndSW") return "south$o";
        else if (destination == "EndWS") return "west$o";
    }
    else if (junctionName == "JuncSE") {
        if (destination == "EndSE") return "south$o";
        else if (destination == "EndES") return "east$o";
    }
    
    // Fallback - should not reach here for valid destinations
    EV << "WARNING: No direct route for destination " << destination 
       << " from junction " << junctionName << endl;
    return "north$o"; // Default fallback
}

std::string Junction::getDirectionFromGate(const std::string& gateName)
{
    // Extract direction from gate name (e.g., "north$i" -> "north")
    if (gateName.find("north") != std::string::npos) return "north";
    if (gateName.find("south") != std::string::npos) return "south";
    if (gateName.find("east") != std::string::npos) return "east";
    if (gateName.find("west") != std::string::npos) return "west";
    
    // Fallback
    return "unknown";
}

void Junction::processCircleMode(cMessage* msg)
{
    Vehicle* veh = dynamic_cast<Vehicle*>(msg);
    std::string destination = veh->getDstEndpoint();
    std::string arrivalGate = msg->getArrivalGate()->getName();
    
    // Track junction processing time (3 seconds for circle mode)
    veh->setTotalJunctionTime(veh->getTotalJunctionTime() + 3.0);
    veh->setJunctionCount(veh->getJunctionCount() + 1);
    
    // Emit statistics
    emit(junctionProcessingTimeSignal, 3.0);
    emit(vehicleCountSignal, (long)veh->getVehNumber());
    emit(queueLengthSignal, 0L); // No queue in circle mode
    
    EV << "Junction " << getName() << ": Vehicle for " << destination 
       << " entering from " << arrivalGate << " (Circle Mode)" << endl;
    
    std::string outputGate;
    
    // Check if this junction is the exit point for the destination
    if (isExitJunction(destination)) {
        // Exit to the destination endpoint
        outputGate = getNextHop(destination, arrivalGate);
        EV << "Junction " << getName() << ": Vehicle for " << destination 
           << " exiting to destination: " << outputGate << endl;
    } else {
        // Continue in circular route: SW -> SE -> NE -> NW -> SW
        outputGate = getCircularRoute(arrivalGate);
        EV << "Junction " << getName() << ": Vehicle for " << destination 
           << " continuing circular route to " << outputGate << endl;
    }
    
    // Send the vehicle after 3 seconds processing time
    sendDelayed(msg, 3.0, outputGate.c_str());
}

std::string Junction::getCircularRoute(const std::string& arrivalGate)
{
    std::string junctionName = getName();
    
    // Circular routing: SW -> SE -> NE -> NW -> SW
    if (junctionName == "JuncSW") {
        // From SW, go to SE (east)
        return "east$o";
    }
    else if (junctionName == "JuncSE") {
        // From SE, go to NE (north)
        return "north$o";
    }
    else if (junctionName == "JuncNE") {
        // From NE, go to NW (west)
        return "west$o";
    }
    else if (junctionName == "JuncNW") {
        // From NW, go to SW (south)
        return "south$o";
    }
    
    // Fallback
    EV << "WARNING: Unknown junction for circular route: " << junctionName << endl;
    return "east$o";
}

void Junction::processRoundRobinMode(cMessage* msg)
{
    Vehicle* veh = dynamic_cast<Vehicle*>(msg);
    std::string destination = veh->getDstEndpoint();
    std::string arrivalGate = msg->getArrivalGate()->getName();
    std::string direction = getDirectionFromGate(arrivalGate);
    
    EV << "Junction " << getName() << ": Vehicle for " << destination 
       << " entering from " << direction << " (Round-Robin Mode, open: " << currentOpenDirection << ")" << endl;
    
    // Check if the vehicle arrives from the currently open direction
    if (direction == currentOpenDirection) {
        // Vehicle can pass immediately - no queue time, just processing
        veh->setTotalJunctionTime(veh->getTotalJunctionTime() + 0.0); // No delay for immediate processing
        veh->setJunctionCount(veh->getJunctionCount() + 1);
        
        // Emit statistics
        emit(junctionProcessingTimeSignal, 0.0);
        emit(vehicleCountSignal, (long)veh->getVehNumber());
        // Vehicle can pass immediately
        std::string outputGate;
        
        // Check if this junction is the exit point for the destination
        if (isExitJunction(destination)) {
            // Exit to the destination endpoint
            outputGate = getNextHop(destination, arrivalGate);
            EV << "Junction " << getName() << ": Vehicle from open direction " << direction 
               << " exiting to destination: " << outputGate << endl;
        } else {
            // Continue counter-clockwise in the roundabout
            outputGate = getCounterClockwiseExit(arrivalGate);
            EV << "Junction " << getName() << ": Vehicle from open direction " << direction 
               << " continuing to " << outputGate << endl;
        }
        
        // Send immediately (no delay in round-robin for open direction)
        send(msg, outputGate.c_str());
    } else {
        // Vehicle must wait in queue for its direction
        EV << "Junction " << getName() << ": Vehicle from " << direction 
           << " queued (open direction is " << currentOpenDirection << ")" << endl;
        
        QueuedVehicle queuedVeh(msg, direction);
        getDirectionQueue(direction).push(queuedVeh);
        
        // Emit queue length statistics for all direction queues
        long totalQueueLength = northQueue.size() + eastQueue.size() + southQueue.size() + westQueue.size();
        emit(queueLengthSignal, totalQueueLength);
    }
}

void Junction::switchToNextDirection()
{
    // Process any remaining vehicles from the current open direction
    processDirectionQueue(currentOpenDirection);
    
    // Switch to next direction clockwise: north -> east -> south -> west -> north
    directionIndex = (directionIndex + 1) % 4;
    
    switch (directionIndex) {
        case 0: currentOpenDirection = "north"; break;
        case 1: currentOpenDirection = "east"; break;
        case 2: currentOpenDirection = "south"; break;
        case 3: currentOpenDirection = "west"; break;
    }
    
    EV << "Junction " << getName() << ": Switching to direction " << currentOpenDirection << endl;
    
    // Process queued vehicles from the new open direction
    processDirectionQueue(currentOpenDirection);
}

void Junction::processDirectionQueue(const std::string& direction)
{
    std::queue<QueuedVehicle>& queue = getDirectionQueue(direction);
    
    while (!queue.empty()) {
        QueuedVehicle queuedVeh = queue.front();
        queue.pop();
        
        cMessage* msg = queuedVeh.msg;
        Vehicle* veh = dynamic_cast<Vehicle*>(msg);
        std::string destination = veh->getDstEndpoint();
        std::string arrivalGate = msg->getArrivalGate()->getName();
        
        // Calculate how long the vehicle waited in queue
        simtime_t waitTime = simTime() - queuedVeh.queueEnterTime;
        veh->setTotalJunctionTime(veh->getTotalJunctionTime() + waitTime);
        veh->setJunctionCount(veh->getJunctionCount() + 1);
        
        // Emit statistics
        emit(junctionProcessingTimeSignal, waitTime);
        emit(vehicleCountSignal, (long)veh->getVehNumber());
        
        std::string outputGate;
        
        // Check if this junction is the exit point for the destination
        if (isExitJunction(destination)) {
            // Exit to the destination endpoint
            outputGate = getNextHop(destination, arrivalGate);
            EV << "Junction " << getName() << ": Queued vehicle for " << destination 
               << " exiting to destination: " << outputGate << endl;
        } else {
            // Continue counter-clockwise in the roundabout
            outputGate = getCounterClockwiseExit(arrivalGate);
            EV << "Junction " << getName() << ": Queued vehicle for " << destination 
               << " continuing to " << outputGate << endl;
        }
        
        // Send immediately (vehicles from queue are processed when their direction opens)
        send(msg, outputGate.c_str());
    }
    
    // Emit updated queue length after processing
    long totalQueueLength = northQueue.size() + eastQueue.size() + southQueue.size() + westQueue.size();
    emit(queueLengthSignal, totalQueueLength);
}

std::queue<QueuedVehicle>& Junction::getDirectionQueue(const std::string& direction)
{
    if (direction == "north") return northQueue;
    else if (direction == "east") return eastQueue;
    else if (direction == "south") return southQueue;
    else if (direction == "west") return westQueue;
    else {
        EV << "WARNING: Unknown direction " << direction << ", using north queue" << endl;
        return northQueue;
    }
}

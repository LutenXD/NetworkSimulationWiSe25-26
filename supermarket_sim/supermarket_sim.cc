//
// Supermarket Simulation Implementation
// Simulates customers, balancer, and cashiers in a supermarket
//

#include <omnetpp.h>
#include <queue>
#include <vector>
#include <algorithm>
#include "supermarket_sim_m.h"

using namespace omnetpp;

//==============================================================================
// CASHIER CLASS
//==============================================================================
class Cashier : public cSimpleModule
{
  private:
    std::queue<CustomerMsg*> customerQueue;
    cMessage *processCustomerTimer;
    bool isBusy;
    int cashierIndex;
    CustomerMsg *currentCustomer;  // Track current customer being served
    
    // Timing for idle time calculation
    simtime_t lastServiceEndTime;
    simtime_t totalIdleTime;
    
    // Statistics
    int customersServed;
    double totalServiceTime;
    int totalItemsProcessed;
    
    // Statistics signals
    simsignal_t queueLengthSignal;
    simsignal_t waitingTimeSignal;
    simsignal_t serviceTimeSignal;
    simsignal_t idleTimeSignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void processNextCustomer();
    void startService(CustomerMsg *customer);
    void finishService();
};

Define_Module(Cashier);

void Cashier::initialize()
{
    processCustomerTimer = new cMessage("processCustomer");
    isBusy = false;
    cashierIndex = getIndex();
    currentCustomer = nullptr;
    
    // Initialize timing
    lastServiceEndTime = simTime();
    totalIdleTime = 0;
    
    // Initialize statistics
    customersServed = 0;
    totalServiceTime = 0.0;
    totalItemsProcessed = 0;
    
    // Register statistics signals
    queueLengthSignal = registerSignal("queueLength");
    waitingTimeSignal = registerSignal("waitingTime");
    serviceTimeSignal = registerSignal("serviceTime");
    idleTimeSignal = registerSignal("idleTime");
    
    // Record initial queue length
    emit(queueLengthSignal, 0);
}

void Cashier::handleMessage(cMessage *msg)
{
    if (msg == processCustomerTimer) {
        // Finish serving current customer
        finishService();
        processNextCustomer();
    }
    else if (CustomerMsg *customer = dynamic_cast<CustomerMsg*>(msg)) {
        // New customer arrived
        EV << "Cashier " << cashierIndex << " received customer " << customer->getCustomerId() 
           << " with " << customer->getNumberOfItems() << " items\n";
        
        customerQueue.push(customer);
        
        // Record queue length change
        emit(queueLengthSignal, (long)customerQueue.size());
        
        if (!isBusy) {
            processNextCustomer();
        }
    }
}

void Cashier::processNextCustomer()
{
    if (!customerQueue.empty()) {
        CustomerMsg *customer = customerQueue.front();
        customerQueue.pop();
        
        // Record queue length change
        emit(queueLengthSignal, (long)customerQueue.size());
        
        startService(customer);
    } else {
        isBusy = false;
        // Start measuring idle time
        lastServiceEndTime = simTime();
    }
}

void Cashier::startService(CustomerMsg *customer)
{
    // Calculate idle time if we were idle
    if (!isBusy) {
        simtime_t idleTime = simTime() - lastServiceEndTime;
        totalIdleTime += idleTime;
        emit(idleTimeSignal, SIMTIME_DBL(idleTime));
    }
    
    isBusy = true;
    currentCustomer = customer;  // Store reference to current customer
    customer->setServiceStartTime(simTime());
    
    // Calculate service time: 0.5s to 2s per item
    int items = customer->getNumberOfItems();
    double serviceTime = 0.0;
    
    for (int i = 0; i < items; i++) {
        serviceTime += uniform(0.5, 2.0);  // Random time per item
    }
    
    EV << "Cashier " << cashierIndex << " starts serving customer " 
       << customer->getCustomerId() << " (service time: " << serviceTime << "s)\n";
    
    // Show popup bubble when starting to serve customer
    char bubbleText[200];
    sprintf(bubbleText, "Serving Customer #%d\n%d items (%.1fs)", 
            customer->getCustomerId(), 
            items,
            serviceTime);
    bubble(bubbleText);
    
    // Calculate and record waiting time
    double waitingTime = SIMTIME_DBL(simTime() - customer->getArrivalTime());
    customer->setTotalWaitingTime(waitingTime);
    emit(waitingTimeSignal, waitingTime);
    
    // Record service time
    emit(serviceTimeSignal, serviceTime);
    
    // Update statistics
    customersServed++;
    totalServiceTime += serviceTime;
    totalItemsProcessed += items;
    
    scheduleAt(simTime() + serviceTime, processCustomerTimer);
}

void Cashier::finishService()
{
    if (currentCustomer) {
        EV << "Cashier " << cashierIndex << " finished serving customer " 
           << currentCustomer->getCustomerId() << " (total waiting time: " 
           << currentCustomer->getTotalWaitingTime() << "s)\n";
        
        // Show popup bubble when customer is finished
        char bubbleText[200];
        sprintf(bubbleText, "Finished Customer #%d\n%d items, %.2fs wait time", 
                currentCustomer->getCustomerId(), 
                currentCustomer->getNumberOfItems(),
                currentCustomer->getTotalWaitingTime());
        bubble(bubbleText);
        
        // Record service end time for idle time calculation
        lastServiceEndTime = simTime();
        
        delete currentCustomer;
        currentCustomer = nullptr;
    }
}

void Cashier::finish()
{
    // Add final idle time if cashier is idle at end
    if (!isBusy) {
        simtime_t finalIdleTime = simTime() - lastServiceEndTime;
        totalIdleTime += finalIdleTime;
    }
    
    // Calculate utilization
    double simulationTime = SIMTIME_DBL(simTime());
    double utilizationRate = simulationTime > 0 ? (totalServiceTime / simulationTime) * 100 : 0;
    double idleRate = simulationTime > 0 ? (SIMTIME_DBL(totalIdleTime) / simulationTime) * 100 : 0;
    
    EV << "Cashier " << cashierIndex << " Statistics:\n";
    EV << "  Customers served: " << customersServed << "\n";
    EV << "  Total items processed: " << totalItemsProcessed << "\n";
    EV << "  Total service time: " << totalServiceTime << "s\n";
    EV << "  Total idle time: " << totalIdleTime << "s\n";
    EV << "  Utilization rate: " << utilizationRate << "%\n";
    EV << "  Idle rate: " << idleRate << "%\n";
    EV << "  Average service time: " << (customersServed > 0 ? totalServiceTime / customersServed : 0) << "s\n";
    EV << "  Queue length at end: " << customerQueue.size() << "\n";
    
    // Record scalar statistics
    recordScalar("customersServed", customersServed);
    recordScalar("totalServiceTime", totalServiceTime);
    recordScalar("totalIdleTime", SIMTIME_DBL(totalIdleTime));
    recordScalar("utilizationRate", utilizationRate);
    recordScalar("idleRate", idleRate);
    recordScalar("averageServiceTime", customersServed > 0 ? totalServiceTime / customersServed : 0);
    recordScalar("queueLengthAtEnd", (double)customerQueue.size());
    recordScalar("totalItemsProcessed", totalItemsProcessed);
    
    cancelAndDelete(processCustomerTimer);
}

//==============================================================================
// BALANCER CLASS
//==============================================================================
class Balancer : public cSimpleModule
{
  private:
    enum BalancingStrategy {
        ROUND_ROBIN = 0,
        SHORTEST_QUEUE = 1,
        RANDOM = 2
    };
    
    BalancingStrategy strategy;
    int roundRobinCounter;
    std::vector<int> cashierQueueLengths;
    int numCashiers;
    
    // Statistics
    int customersForwarded;
    std::vector<int> cashierAssignments; // Track assignments per cashier
    
    // Statistics signals  
    simsignal_t loadBalancingSignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    int selectCashier();
};

Define_Module(Balancer);

void Balancer::initialize()
{
    // Get balancing strategy from parameter (default: round robin)
    strategy = static_cast<BalancingStrategy>(par("strategy").intValue());
    roundRobinCounter = 0;
    
    // Get number of cashiers from gate size
    numCashiers = gateSize("out");
    cashierQueueLengths.resize(numCashiers, 0);
    cashierAssignments.resize(numCashiers, 0);
    customersForwarded = 0;
    
    // Register statistics signals
    loadBalancingSignal = registerSignal("loadBalancing");
    
    EV << "Balancer initialized with " << numCashiers << " cashiers and strategy: ";
    switch(strategy) {
        case ROUND_ROBIN: EV << "Round Robin\n"; break;
        case SHORTEST_QUEUE: EV << "Shortest Queue First\n"; break;
        case RANDOM: EV << "Random\n"; break;
    }
}

void Balancer::handleMessage(cMessage *msg)
{
    if (CustomerMsg *customer = dynamic_cast<CustomerMsg*>(msg)) {
        int selectedCashier = selectCashier();
        
        EV << "Balancer forwards customer " << customer->getCustomerId() 
           << " to cashier " << selectedCashier << " (strategy: ";
        
        const char* strategyName;
        switch(strategy) {
            case ROUND_ROBIN: 
                EV << "Round Robin"; 
                strategyName = "Round Robin";
                break;
            case SHORTEST_QUEUE: 
                EV << "Shortest Queue"; 
                strategyName = "Shortest Queue";
                break;
            case RANDOM: 
                EV << "Random"; 
                strategyName = "Random";
                break;
        }
        EV << ")\n";
        
        // Show popup bubble for load balancing decision
        char bubbleText[200];
        sprintf(bubbleText, "Customer #%d → Cashier %d\n%s strategy", 
                customer->getCustomerId(), 
                selectedCashier,
                strategyName);
        bubble(bubbleText);
        
        // Update queue length tracking (simplified - in real implementation 
        // we would get feedback from cashiers about queue changes)
        cashierQueueLengths[selectedCashier]++;
        cashierAssignments[selectedCashier]++;
        customersForwarded++;
        
        // Record load balancing decision
        emit(loadBalancingSignal, (long)selectedCashier);
        
        // Forward to selected cashier
        send(customer, "out", selectedCashier);
    }
}

int Balancer::selectCashier()
{
    int selectedCashier = 0;
    
    switch(strategy) {
        case ROUND_ROBIN:
            selectedCashier = roundRobinCounter % numCashiers;
            roundRobinCounter++;
            break;
            
        case SHORTEST_QUEUE:
            {
                auto minIt = std::min_element(cashierQueueLengths.begin(), cashierQueueLengths.end());
                selectedCashier = std::distance(cashierQueueLengths.begin(), minIt);
            }
            break;
            
        case RANDOM:
            selectedCashier = intuniform(0, numCashiers - 1);
            break;
    }
    
    return selectedCashier;
}

void Balancer::finish()
{
    EV << "Balancer Statistics:\n";
    EV << "  Customers forwarded: " << customersForwarded << "\n";
    EV << "  Assignments per cashier: ";
    for (int i = 0; i < numCashiers; i++) {
        EV << "C" << i << ":" << cashierAssignments[i] << " ";
    }
    EV << "\n";
    EV << "  Final queue lengths: ";
    for (int i = 0; i < numCashiers; i++) {
        EV << "C" << i << ":" << cashierQueueLengths[i] << " ";
    }
    EV << "\n";
    
    // Calculate load balancing efficiency
    double maxAssignments = *std::max_element(cashierAssignments.begin(), cashierAssignments.end());
    double minAssignments = *std::min_element(cashierAssignments.begin(), cashierAssignments.end());
    double balancingEfficiency = maxAssignments > 0 ? (minAssignments / maxAssignments) * 100 : 100;
    
    recordScalar("customersForwarded", customersForwarded);
    recordScalar("balancingEfficiency", balancingEfficiency);
    
    // Record individual cashier assignments
    for (int i = 0; i < numCashiers; i++) {
        char scalarName[50];
        sprintf(scalarName, "cashier%d_assignments", i);
        recordScalar(scalarName, cashierAssignments[i]);
    }
}

//==============================================================================
// SHOP CLASS (Customer Generator)
//==============================================================================
class Shop : public cSimpleModule
{
  private:
    cMessage *generateCustomerTimer;
    int customerCounter;
    double arrivalInterval;
    
    // Statistics
    int customersGenerated;
    
    // Statistics signals
    simsignal_t customerGeneratedSignal;
    simsignal_t interArrivalTimeSignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void generateCustomer();
};

Define_Module(Shop);

void Shop::initialize()
{
    generateCustomerTimer = new cMessage("generateCustomer");
    customerCounter = 1;
    arrivalInterval = par("arrivalInterval").doubleValue();
    customersGenerated = 0;
    
    // Register statistics signals
    customerGeneratedSignal = registerSignal("customerGenerated");
    interArrivalTimeSignal = registerSignal("interArrivalTime");
    
    EV << "Shop initialized with mean arrival interval: " << arrivalInterval << "s (exponential distribution)\n";
    EV << "Current simulation time: " << simTime() << "\n";
    EV << "Scheduling first customer at time: " << (simTime() + 0.1) << "\n";
    
    // Schedule first customer immediately to start the simulation
    scheduleAt(simTime() + 0.1, generateCustomerTimer);
}

void Shop::handleMessage(cMessage *msg)
{
    if (msg == generateCustomerTimer) {
        generateCustomer();
        
        // Schedule next customer arrival using exponential distribution
        double nextArrival = exponential(arrivalInterval);
        emit(interArrivalTimeSignal, nextArrival);
        EV << "Next customer scheduled in " << nextArrival << " seconds (exponential)\n";
        scheduleAt(simTime() + nextArrival, generateCustomerTimer);
    }
}

void Shop::generateCustomer()
{
    EV << "generateCustomer() called at time: " << simTime() << "\n";
    
    // Create new customer
    CustomerMsg *customer = new CustomerMsg("customer");
    customer->setCustomerId(customerCounter++);
    customer->setNumberOfItems(intuniform(1, 25));  // 1 to 25 items
    customer->setArrivalTime(simTime());
    
    EV << "Shop generates customer " << customer->getCustomerId() 
       << " with " << customer->getNumberOfItems() << " items at time " << simTime() << "\n";
    
    // Show popup bubble for new customer generation
    char bubbleText[200];
    sprintf(bubbleText, "New Customer #%d\n%d items in basket", 
            customer->getCustomerId(), 
            customer->getNumberOfItems());
    bubble(bubbleText);
    
    customersGenerated++;
    emit(customerGeneratedSignal, (long)customersGenerated);
    
    // Send to balancer
    EV << "Sending customer to balancer via 'out' gate\n";
    send(customer, "out");
}

void Shop::finish()
{
    EV << "Shop Statistics:\n";
    EV << "  Customers generated: " << customersGenerated << "\n";
    
    recordScalar("customersGenerated", customersGenerated);
    cancelAndDelete(generateCustomerTimer);
}

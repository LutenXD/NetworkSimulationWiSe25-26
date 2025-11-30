//
// M/M/1 Queue System Implementation
// Implements Producer, Queue, Service Unit, and Sink modules
//

#include <omnetpp.h>
#include <queue>
#include "queue_sim_m.h"

using namespace omnetpp;

//==============================================================================
// PRODUCER CLASS
//==============================================================================
class Producer : public cSimpleModule
{
  private:
    cMessage *generateJobTimer;
    int jobCounter;
    double meanInterArrivalTime;
    simtime_t warmupPeriod;
    
    // Statistics
    int jobsGenerated;
    int jobsGeneratedAfterWarmup;
    double sumInterArrivalTimes;
    int interArrivalCount;
    
    // Statistics signals
    simsignal_t jobGeneratedSignal;
    simsignal_t interArrivalTimeSignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void generateJob();
};

Define_Module(Producer);

void Producer::initialize()
{
    generateJobTimer = new cMessage("generateJob");
    jobCounter = 1;
    meanInterArrivalTime = par("meanInterArrivalTime").doubleValue();
    warmupPeriod = par("warmupPeriod").doubleValue();
    jobsGenerated = 0;
    jobsGeneratedAfterWarmup = 0;
    sumInterArrivalTimes = 0.0;
    interArrivalCount = 0;
    
    // Register statistics signals
    jobGeneratedSignal = registerSignal("jobGenerated");
    interArrivalTimeSignal = registerSignal("interArrivalTime");
    
    EV << "Producer initialized with mean inter-arrival time: " << meanInterArrivalTime << "s\n";
    EV << "Warmup period: " << warmupPeriod << "s\n";
    
    // Schedule first job immediately
    scheduleAt(simTime() + 0.1, generateJobTimer);
}

void Producer::handleMessage(cMessage *msg)
{
    if (msg == generateJobTimer) {
        generateJob();
        
        // Schedule next job arrival using exponential distribution
        double nextArrival = exponential(meanInterArrivalTime);
        emit(interArrivalTimeSignal, nextArrival);
        
        // Track inter-arrival times after warmup
        if (simTime() >= warmupPeriod) {
            sumInterArrivalTimes += nextArrival;
            interArrivalCount++;
        }
        
        EV << "Next job scheduled in " << nextArrival << " seconds\n";
        scheduleAt(simTime() + nextArrival, generateJobTimer);
    }
}

void Producer::generateJob()
{
    // Create new job
    JobMsg *job = new JobMsg("job");
    job->setJobId(jobCounter++);
    job->setArrivalTime(simTime());
    
    EV << "Producer generates job " << job->getJobId() << " at time " << simTime() << "\n";
    
    jobsGenerated++;
    if (simTime() >= warmupPeriod) {
        jobsGeneratedAfterWarmup++;
    }
    emit(jobGeneratedSignal, (long)jobsGenerated);
    
    // Send to queue
    send(job, "out");
}

void Producer::finish()
{
    double avgInterArrivalTime = interArrivalCount > 0 ? sumInterArrivalTimes / interArrivalCount : 0.0;
    
    EV << "Producer Statistics:\n";
    EV << "  Jobs generated: " << jobsGenerated << "\n";
    EV << "  Jobs generated after warmup: " << jobsGeneratedAfterWarmup << "\n";
    EV << "  Configured mean inter-arrival time: " << meanInterArrivalTime << "s\n";
    EV << "  Observed average inter-arrival time: " << avgInterArrivalTime << "s\n";
    
    recordScalar("jobsGenerated", jobsGenerated);
    recordScalar("jobsGeneratedAfterWarmup", jobsGeneratedAfterWarmup);
    recordScalar("meanInterArrivalTime", meanInterArrivalTime);
    recordScalar("avgInterArrivalTime", avgInterArrivalTime);
    cancelAndDelete(generateJobTimer);
}

//==============================================================================
// QUEUE CLASS
//==============================================================================
class Queue : public cSimpleModule
{
  private:
    std::queue<JobMsg*> jobQueue;
    bool serviceUnitBusy;
    simtime_t warmupPeriod;
    
    // Statistics
    int maxQueueLength;
    double totalQueueingDelay;
    int jobsQueued;
    int jobsQueuedAfterWarmup;
    
    // Queue utilization tracking
    simtime_t lastQueueChangeTime;
    simtime_t totalNonEmptyTime;
    simtime_t measurementStartTime;
    
    // System statistics (queue + service)
    int currentSystemSize;  // Jobs in queue + service unit
    double systemSizeSum;
    int systemSizeSamples;
    
    // Statistics signals
    simsignal_t queueLengthSignal;
    simsignal_t queueingDelaySignal;
    simsignal_t queueUtilizationSignal;
    simsignal_t systemSizeSignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void tryToSendJob();
    void updateQueueUtilization();
    void updateSystemSize();
};

Define_Module(Queue);

void Queue::initialize()
{
    serviceUnitBusy = false;
    warmupPeriod = par("warmupPeriod").doubleValue();
    maxQueueLength = 0;
    totalQueueingDelay = 0.0;
    jobsQueued = 0;
    jobsQueuedAfterWarmup = 0;
    
    // Queue utilization tracking
    lastQueueChangeTime = simTime();
    totalNonEmptyTime = 0;
    measurementStartTime = warmupPeriod;
    
    // System size tracking
    currentSystemSize = 0;
    systemSizeSum = 0.0;
    systemSizeSamples = 0;
    
    // Register statistics signals
    queueLengthSignal = registerSignal("queueLength");
    queueingDelaySignal = registerSignal("queueingDelay");
    queueUtilizationSignal = registerSignal("queueUtilization");
    systemSizeSignal = registerSignal("systemSize");
    
    // Record initial queue length and system size
    emit(queueLengthSignal, 0);
    emit(systemSizeSignal, 0);
    
    EV << "Queue initialized\n";
}

void Queue::handleMessage(cMessage *msg)
{
    if (JobMsg *job = dynamic_cast<JobMsg*>(msg)) {
        if (job->getServiceEndTime() > 0) {
            // Job completed from service unit
            EV << "Queue receives completed job " << job->getJobId() << " from service unit\n";
            serviceUnitBusy = false;
            currentSystemSize--;  // Job leaving system
            updateSystemSize();
            
            // Forward completed job to next module (should go to sink)
            send(job, "out");
            
            // Try to send next job from queue
            tryToSendJob();
        } else {
            // New job arrived from producer
            EV << "Queue receives new job " << job->getJobId() << " from producer\n";
            
            job->setQueueEnterTime(simTime());
            
            // Update queue utilization before adding job
            updateQueueUtilization();
            
            jobQueue.push(job);
            jobsQueued++;
            if (simTime() >= warmupPeriod) {
                jobsQueuedAfterWarmup++;
            }
            
            currentSystemSize++;  // Job entering system
            updateSystemSize();
            
            // Update queue length statistics
            int currentQueueLength = jobQueue.size();
            emit(queueLengthSignal, (long)currentQueueLength);
            
            if (currentQueueLength > maxQueueLength) {
                maxQueueLength = currentQueueLength;
            }
            
            EV << "Queue length: " << currentQueueLength << "\n";
            
            // Try to send job if service unit is free
            tryToSendJob();
        }
    }
}

void Queue::tryToSendJob()
{
    if (!serviceUnitBusy && !jobQueue.empty()) {
        // Update queue utilization before removing job
        updateQueueUtilization();
        
        JobMsg *job = jobQueue.front();
        jobQueue.pop();
        
        // Calculate queueing delay
        double queueingDelay = SIMTIME_DBL(simTime() - job->getQueueEnterTime());
        if (simTime() >= warmupPeriod) {
            totalQueueingDelay += queueingDelay;
        }
        emit(queueingDelaySignal, queueingDelay);
        
        EV << "Queue sends job " << job->getJobId() << " to service unit (queueing delay: " 
           << queueingDelay << "s)\n";
        
        serviceUnitBusy = true;
        
        // Update queue length
        emit(queueLengthSignal, (long)jobQueue.size());
        updateSystemSize();
        
        // Send to service unit
        send(job, "out");
    }
}

void Queue::finish()
{
    // Final queue utilization update
    updateQueueUtilization();
    
    double avgQueueingDelay = jobsQueuedAfterWarmup > 0 ? totalQueueingDelay / jobsQueuedAfterWarmup : 0.0;
    double measurementTime = SIMTIME_DBL(simTime() - measurementStartTime);
    double queueUtilization = measurementTime > 0 ? SIMTIME_DBL(totalNonEmptyTime) / measurementTime : 0.0;
    double avgSystemSize = systemSizeSamples > 0 ? systemSizeSum / systemSizeSamples : 0.0;
    
    EV << "Queue Statistics:\n";
    EV << "  Jobs queued: " << jobsQueued << "\n";
    EV << "  Jobs queued after warmup: " << jobsQueuedAfterWarmup << "\n";
    EV << "  Max queue length: " << maxQueueLength << "\n";
    EV << "  Average queueing delay: " << avgQueueingDelay << "s\n";
    EV << "  Queue utilization: " << queueUtilization * 100 << "%\n";
    EV << "  Average system size: " << avgSystemSize << "\n";
    EV << "  Final queue length: " << jobQueue.size() << "\n";
    
    recordScalar("jobsQueued", jobsQueued);
    recordScalar("jobsQueuedAfterWarmup", jobsQueuedAfterWarmup);
    recordScalar("maxQueueLength", maxQueueLength);
    recordScalar("avgQueueingDelay", avgQueueingDelay);
    recordScalar("queueUtilization", queueUtilization);
    recordScalar("avgSystemSize", avgSystemSize);
    recordScalar("finalQueueLength", (double)jobQueue.size());
    
    emit(queueUtilizationSignal, queueUtilization);
}

void Queue::updateQueueUtilization()
{
    if (simTime() >= measurementStartTime) {
        simtime_t timeDelta = simTime() - lastQueueChangeTime;
        if (!jobQueue.empty()) {
            totalNonEmptyTime += timeDelta;
        }
    }
    lastQueueChangeTime = simTime();
}

void Queue::updateSystemSize()
{
    if (simTime() >= warmupPeriod) {
        systemSizeSum += currentSystemSize;
        systemSizeSamples++;
        emit(systemSizeSignal, (long)currentSystemSize);
    }
}

//==============================================================================
// SERVICE UNIT CLASS
//==============================================================================
class ServiceUnit : public cSimpleModule
{
  private:
    cMessage *serviceTimer;
    JobMsg *currentJob;
    double meanServiceTime;
    simtime_t warmupPeriod;
    
    // Statistics
    int jobsServed;
    int jobsServedAfterWarmup;
    double totalServiceTime;
    double totalServiceTimeAfterWarmup;
    simtime_t totalBusyTime;
    simtime_t totalBusyTimeAfterWarmup;
    simtime_t lastServiceEndTime;
    simtime_t measurementStartTime;
    
    // Statistics signals
    simsignal_t serviceTimeSignal;
    simsignal_t utilizationSignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

Define_Module(ServiceUnit);

void ServiceUnit::initialize()
{
    serviceTimer = new cMessage("serviceTimer");
    currentJob = nullptr;
    meanServiceTime = par("meanServiceTime").doubleValue();
    warmupPeriod = par("warmupPeriod").doubleValue();
    
    jobsServed = 0;
    jobsServedAfterWarmup = 0;
    totalServiceTime = 0.0;
    totalServiceTimeAfterWarmup = 0.0;
    totalBusyTime = 0;
    totalBusyTimeAfterWarmup = 0;
    lastServiceEndTime = simTime();
    measurementStartTime = warmupPeriod;
    
    // Register statistics signals
    serviceTimeSignal = registerSignal("serviceTime");
    utilizationSignal = registerSignal("utilization");
    
    EV << "Service Unit initialized with mean service time: " << meanServiceTime << "s\n";
    EV << "Warmup period: " << warmupPeriod << "s\n";
}

void ServiceUnit::handleMessage(cMessage *msg)
{
    if (msg == serviceTimer) {
        // Finish serving current job
        EV << "Service Unit finishes serving job " << currentJob->getJobId() << "\n";
        
        currentJob->setServiceEndTime(simTime());
        
        // Update statistics
        simtime_t serviceDuration = simTime() - currentJob->getServiceStartTime();
        totalBusyTime += serviceDuration;
        
        if (currentJob->getServiceStartTime() >= warmupPeriod) {
            totalBusyTimeAfterWarmup += serviceDuration;
        }
        
        // Send completed job back to queue (which will forward to sink)
        send(currentJob, "out");
        currentJob = nullptr;
        
        lastServiceEndTime = simTime();
        
        // Update utilization
        double totalTime = SIMTIME_DBL(simTime());
        double utilization = totalTime > 0 ? SIMTIME_DBL(totalBusyTime) / totalTime * 100 : 0;
        emit(utilizationSignal, utilization);
    }
    else if (JobMsg *job = dynamic_cast<JobMsg*>(msg)) {
        // New job to serve
        EV << "Service Unit starts serving job " << job->getJobId() << "\n";
        
        currentJob = job;
        currentJob->setServiceStartTime(simTime());
        
        // Generate service time using exponential distribution
        double serviceTime = exponential(meanServiceTime);
        totalServiceTime += serviceTime;
        jobsServed++;
        
        if (simTime() >= warmupPeriod) {
            totalServiceTimeAfterWarmup += serviceTime;
            jobsServedAfterWarmup++;
        }
        
        emit(serviceTimeSignal, serviceTime);
        
        EV << "Service time: " << serviceTime << "s\n";
        
        // Schedule service completion
        scheduleAt(simTime() + serviceTime, serviceTimer);
    }
}

void ServiceUnit::finish()
{
    double avgServiceTime = jobsServed > 0 ? totalServiceTime / jobsServed : 0.0;
    double avgServiceTimeAfterWarmup = jobsServedAfterWarmup > 0 ? totalServiceTimeAfterWarmup / jobsServedAfterWarmup : 0.0;
    double totalTime = SIMTIME_DBL(simTime());
    double utilization = totalTime > 0 ? SIMTIME_DBL(totalBusyTime) / totalTime * 100 : 0.0;
    double measurementTime = SIMTIME_DBL(simTime() - measurementStartTime);
    double utilizationAfterWarmup = measurementTime > 0 ? SIMTIME_DBL(totalBusyTimeAfterWarmup) / measurementTime * 100 : 0.0;
    
    EV << "Service Unit Statistics:\n";
    EV << "  Jobs served: " << jobsServed << "\n";
    EV << "  Jobs served after warmup: " << jobsServedAfterWarmup << "\n";
    EV << "  Configured mean service time: " << meanServiceTime << "s\n";
    EV << "  Average service time: " << avgServiceTime << "s\n";
    EV << "  Average service time after warmup: " << avgServiceTimeAfterWarmup << "s\n";
    EV << "  Total busy time: " << totalBusyTime << "s\n";
    EV << "  Utilization: " << utilization << "%\n";
    EV << "  Utilization after warmup: " << utilizationAfterWarmup << "%\n";
    
    recordScalar("jobsServed", jobsServed);
    recordScalar("jobsServedAfterWarmup", jobsServedAfterWarmup);
    recordScalar("meanServiceTime", meanServiceTime);
    recordScalar("avgServiceTime", avgServiceTime);
    recordScalar("avgServiceTimeAfterWarmup", avgServiceTimeAfterWarmup);
    recordScalar("totalBusyTime", SIMTIME_DBL(totalBusyTime));
    recordScalar("utilization", utilization);
    recordScalar("utilizationAfterWarmup", utilizationAfterWarmup);
    
    cancelAndDelete(serviceTimer);
}

//==============================================================================
// SINK CLASS
//==============================================================================
class Sink : public cSimpleModule
{
  private:
    simtime_t warmupPeriod;
    
    // Statistics
    int jobsCompleted;
    int jobsCompletedAfterWarmup;
    double totalSystemDelay;
    double totalSystemDelayAfterWarmup;
    
    // Statistics signals
    simsignal_t jobCompletedSignal;
    simsignal_t totalDelaySignal;
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

Define_Module(Sink);

void Sink::initialize()
{
    warmupPeriod = par("warmupPeriod").doubleValue();
    jobsCompleted = 0;
    jobsCompletedAfterWarmup = 0;
    totalSystemDelay = 0.0;
    totalSystemDelayAfterWarmup = 0.0;
    
    // Register statistics signals
    jobCompletedSignal = registerSignal("jobCompleted");
    totalDelaySignal = registerSignal("totalDelay");
    
    EV << "Sink initialized\n";
    EV << "Warmup period: " << warmupPeriod << "s\n";
}

void Sink::handleMessage(cMessage *msg)
{
    if (JobMsg *job = dynamic_cast<JobMsg*>(msg)) {
        EV << "Sink receives completed job " << job->getJobId() << "\n";
        
        // Calculate total system delay (arrival to completion)
        double systemDelay = SIMTIME_DBL(simTime() - job->getArrivalTime());
        totalSystemDelay += systemDelay;
        jobsCompleted++;
        
        // Track statistics after warmup
        if (job->getArrivalTime() >= warmupPeriod) {
            totalSystemDelayAfterWarmup += systemDelay;
            jobsCompletedAfterWarmup++;
        }
        
        emit(jobCompletedSignal, (long)jobsCompleted);
        emit(totalDelaySignal, systemDelay);
        
        EV << "Job " << job->getJobId() << " total system delay: " << systemDelay << "s\n";
        
        // Delete the job
        delete job;
    }
}

void Sink::finish()
{
    double avgSystemDelay = jobsCompleted > 0 ? totalSystemDelay / jobsCompleted : 0.0;
    double avgSystemDelayAfterWarmup = jobsCompletedAfterWarmup > 0 ? totalSystemDelayAfterWarmup / jobsCompletedAfterWarmup : 0.0;
    
    EV << "Sink Statistics:\n";
    EV << "  Jobs completed: " << jobsCompleted << "\n";
    EV << "  Jobs completed after warmup: " << jobsCompletedAfterWarmup << "\n";
    EV << "  Average system delay: " << avgSystemDelay << "s\n";
    EV << "  Average system delay after warmup: " << avgSystemDelayAfterWarmup << "s\n";
    EV << "  Total system delay: " << totalSystemDelay << "s\n";
    
    recordScalar("jobsCompleted", jobsCompleted);
    recordScalar("jobsCompletedAfterWarmup", jobsCompletedAfterWarmup);
    recordScalar("avgSystemDelay", avgSystemDelay);
    recordScalar("avgSystemDelayAfterWarmup", avgSystemDelayAfterWarmup);
    recordScalar("totalSystemDelay", totalSystemDelay);
}
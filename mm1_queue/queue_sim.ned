//
// M/M/1 Queue Network Definition
// Simple queue system with producer, queue, service unit, and sink
//

simple Producer
{
    parameters:
        double meanInterArrivalTime @unit(s) = default(1s);  // β_producer = λ^-1 (mean inter-arrival time)
        double warmupPeriod @unit(s) = default(100s);        // Warmup period for statistics
        @display("i=block/source");
        
        // Statistics signals
        @signal[jobGenerated](type=long);
        @signal[interArrivalTime](type=double);
        @statistic[jobGenerated](title="Jobs Generated"; record=vector,last; interpolationmode=sample-hold);
        @statistic[interArrivalTime](title="Inter-arrival Time"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        
    gates:
        output out;
}

simple Queue
{
    parameters:
        double warmupPeriod @unit(s) = default(100s);        // Warmup period for statistics
        @display("i=block/queue");
        
        // Statistics signals
        @signal[queueLength](type=long);
        @signal[queueingDelay](type=double);
        @signal[queueUtilization](type=double);
        @signal[systemSize](type=long);
        @statistic[queueLength](title="Queue Length"; record=vector,timeavg,max; interpolationmode=sample-hold);
        @statistic[queueingDelay](title="Queueing Delay"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        @statistic[queueUtilization](title="Queue Utilization"; record=scalar; interpolationmode=none);
        @statistic[systemSize](title="System Size"; record=vector,timeavg,mean; interpolationmode=sample-hold);
        
    gates:
        input in;
        output out;
}

simple ServiceUnit
{
    parameters:
        double meanServiceTime @unit(s) = default(1s);       // β_service = μ^-1 (mean service time)
        double warmupPeriod @unit(s) = default(100s);        // Warmup period for statistics
        @display("i=block/process");
        
        // Statistics signals
        @signal[serviceTime](type=double);
        @signal[utilization](type=double);
        @statistic[serviceTime](title="Service Time"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        @statistic[utilization](title="Server Utilization"; record=vector,timeavg,last; interpolationmode=sample-hold);
        
    gates:
        input in;
        output out;
}

simple Sink
{
    parameters:
        double warmupPeriod @unit(s) = default(100s);        // Warmup period for statistics
        @display("i=block/sink");
        
        // Statistics signals
        @signal[jobCompleted](type=long);
        @signal[totalDelay](type=double);
        @statistic[jobCompleted](title="Jobs Completed"; record=vector,last; interpolationmode=sample-hold);
        @statistic[totalDelay](title="Total System Delay"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        
    gates:
        input in;
}

network queue_sim
{
    submodules:
        producer: Producer;
        queue: Queue;
        serviceUnit: ServiceUnit;
        sink: Sink;

    connections:
        producer.out --> queue.in;
        queue.out --> serviceUnit.in;
        serviceUnit.out --> sink.in;
}
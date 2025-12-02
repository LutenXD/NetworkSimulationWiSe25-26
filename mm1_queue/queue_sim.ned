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
        @statistic[jobGenerated](title="Jobs Generated"; record=vector);
        @statistic[interArrivalTime](title="Inter-arrival Time"; unit=s; record=vector,mean);
        
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
        @statistic[queueLength](title="Queue Length"; record=vector);
        @statistic[queueingDelay](title="Queueing Delay"; unit=s; record=vector,mean);
        @statistic[queueUtilization](title="Queue Utilization"; record=vector);
        @statistic[systemSize](title="System Size"; record=vector,mean);
        
    gates:
        input in;
        output out;
        input fromService;  // For receiving completion notifications from service unit
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
        @statistic[serviceTime](title="Service Time"; unit=s; record=vector,mean);
        @statistic[utilization](title="Server Utilization"; record=vector);
        
    gates:
        input in;
        output out;
        output toQueue;  // For sending completion notifications back to queue
}

simple Sink
{
    parameters:
        double warmupPeriod @unit(s) = default(100s);        // Warmup period for statistics
        @display("i=block/sink");
        
        // Statistics signals
        @signal[jobCompleted](type=long);
        @signal[totalDelay](type=double);
        @statistic[jobCompleted](title="Jobs Completed"; record=vector);
        @statistic[totalDelay](title="Total System Delay"; unit=s; record=vector,mean);
        
    gates:
        input in;
}

network queue_sim
{
    submodules:
        producer: Producer{
            @display("p=40,40");
        }
        queue: Queue{
            @display("p=160,40");
        }
        serviceUnit: ServiceUnit{
            @display("p=280,40");
        }
        sink: Sink{
            @display("p=400,40");
        }

    connections:
        producer.out --> queue.in;
        queue.out --> serviceUnit.in;
        serviceUnit.out --> sink.in;
        serviceUnit.toQueue --> queue.fromService;
}
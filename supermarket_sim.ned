simple Shop
{
    parameters:
        double arrivalInterval @unit(s) = default(5s);  // Mean time between customer arrivals (exponential distribution)
        @display("i=block/source");
        
        // Statistics signals
        @signal[customerGenerated](type=long);
        @signal[interArrivalTime](type=double);
        @statistic[customerGenerated](title="Customers Generated"; record=vector,last; interpolationmode=sample-hold);
        @statistic[interArrivalTime](title="Inter-arrival Time"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        
    gates:
        output out;
}

simple Balancer
{
    parameters:
        int strategy = default(0);  // 0=Round Robin, 1=Shortest Queue, 2=Random
        @display("i=block/dispatch");
        
        // Statistics signals
        @signal[loadBalancing](type=long);
        @statistic[loadBalancing](title="Load Balancing Decisions"; record=vector,histogram; interpolationmode=sample-hold);
        
    gates:
        input in;
        output out[];
}

simple Cashier
{
    parameters:
        @display("i=block/sink");
        
        // Statistics signals
        @signal[queueLength](type=long);
        @signal[waitingTime](type=double);
        @signal[serviceTime](type=double);
        @signal[idleTime](type=double);
        
        @statistic[queueLength](title="Queue Length"; record=vector,timeavg,max; interpolationmode=sample-hold);
        @statistic[waitingTime](title="Customer Waiting Time"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        @statistic[serviceTime](title="Service Time"; unit=s; record=vector,histogram,mean,max; interpolationmode=none);
        @statistic[idleTime](title="Cashier Idle Time"; unit=s; record=vector,histogram,mean,sum; interpolationmode=none);
        
    gates:
        input in;
}

network supermarket_sim
{
    parameters:
        int numCashiers = default(4);
        
    submodules:
        shop: Shop {
            parameters:
                arrivalInterval = 1s;  // Mean inter-arrival time (exponentially distributed)
        }
        balancer: Balancer {
            parameters:
                strategy = 0;  // 0=Round Robin, 1=Shortest Queue, 2=Random
        }
        cashier[numCashiers]: Cashier;

    connections allowunconnected:
        shop.out --> balancer.in;
        for i=0..numCashiers-1 {
            balancer.out++ --> cashier[i].in;
        }
}


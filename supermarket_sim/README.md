# Supermarket Simulation

## Features

### Customer Model
- **Basket Size**: Each customer has 1-25 items (uniformly distributed)
- **Arrival Pattern**: Exponentially distributed inter-arrival times (realistic queueing behavior)
- **Comprehensive Tracking**: Arrival time, service start time, waiting time, and service completion

### Processing Model
- **Realistic Service Times**: 0.5-2.0 seconds per item (uniformly distributed)
- **Sequential Processing**: Cashiers handle one customer at a time with proper queueing
- **Dynamic Queue Management**: Real-time queue length tracking and statistics

### Load Balancing Strategies

1. **Round Robin (strategy = 0)**: Cyclic assignment ensuring equal distribution
2. **Shortest Queue First (strategy = 1)**: Optimal assignment to minimize waiting times
3. **Random (strategy = 2)**: Random distribution for comparison baseline

### Visual Feedback
- **Real-time Bubbles**: Interactive popup messages showing simulation events
- **Customer Generation**: Shows new arrivals with basket sizes
- **Load Balancing**: Displays assignment decisions and strategies
- **Service Events**: Cashier start/finish notifications with timing details

## Configuration

### Available Simulation Scenarios in omnetpp.ini:

- **Default**: Round Robin balancing, 4 cashiers, 1s mean arrival interval
- **ShortestQueue**: Optimal shortest queue first strategy
- **Random**: Random assignment for baseline comparison
- **HighLoad**: High-frequency arrivals (0.5s mean interval) - stress testing
- **LowLoad**: Low-frequency arrivals (5s mean interval) - light load analysis

### Key Parameters:

- **`*.shop.arrivalInterval`**: Mean time between customer arrivals (exponential distribution)
- **`*.balancer.strategy`**: Load balancing algorithm (0=Round Robin, 1=Shortest Queue, 2=Random)
- **`sim-time-limit`**: Total simulation duration (default: 10000s)

## Statistics & Analytics

### Performance Metrics Collected:

#### 1. **Customer Waiting Times**
- **Per Customer**: Individual waiting time from arrival to service start
- **Overall System**: Aggregated waiting time statistics across all customers
- **Distributions**: Histogram analysis of waiting time patterns
- **Signals**: `waitingTime` (vector + scalar statistics)

#### 2. **Queue Management**
- **Queue Size Over Time**: Real-time queue length monitoring
- **Peak Queue Lengths**: Maximum queue sizes reached per cashier
- **Queue Utilization**: Time-weighted average queue occupancy
- **Signals**: `queueLength` (vector with timeavg, max statistics)

#### 3. **Service Time Analysis**
- **Per Cashier**: Individual service times for each customer transaction
- **Service Rate**: Customers served per unit time
- **Service Distribution**: Statistical analysis of service time patterns
- **Signals**: `serviceTime` (vector + histogram + mean/max)

#### 4. **Cashier Utilization & Efficiency**
- **Active Service Time**: Total time spent actively serving customers
- **Idle Time Periods**: Time spent waiting for customers
- **Utilization Rate**: Percentage of time actively working (service time / total time)
- **Idle Rate**: Percentage of time waiting for customers
- **Signals**: `idleTime` (vector + histogram + sum statistics)

#### 5. **System Throughput**
- **Customer Generation Rate**: Inter-arrival time analysis
- **System Capacity**: Total customers processed
- **Load Balancing Effectiveness**: Distribution fairness across cashiers
- **Signals**: `customerGenerated`, `interArrivalTime`, `loadBalancing`

### Advanced Analytics:

### Core Components

#### `CustomerMsg` (Message Definition)
```cpp
- customerId: Unique customer identifier
- numberOfItems: Basket size (1-25 items)
- arrivalTime: Timestamp when customer entered system
- serviceStartTime: When service began
- totalWaitingTime: Complete waiting duration
```

#### `Shop` (Customer Generator)
- Exponential inter-arrival time generation
- Random basket size assignment
- Real-time customer generation tracking
- Visual bubble notifications

#### `Balancer` (Load Balancer)
- **Round Robin**: Ensures equal distribution across cashiers
- **Shortest Queue**: Minimizes individual waiting times
- **Random**: Baseline comparison strategy
- Load balancing efficiency tracking

#### `Cashier` (Service Processor)
- Individual customer queues with FIFO processing
- Realistic service time calculation (0.5-2.0s per item)
- Comprehensive idle time and utilization tracking
- Real-time performance monitoring

### Statistics Infrastructure
- **Signal-based Data Collection**: Type-safe statistics using OMNeT++ signals
- **Vector Recording**: Time-series data for detailed analysis
- **Scalar Statistics**: Summary metrics for quick comparison
- **Histogram Generation**: Distribution analysis for all key metrics

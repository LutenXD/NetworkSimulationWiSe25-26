# M/M/1 Queue System - Task 3 Implementation

This implements a comprehensive M/M/1 queuing system with detailed statistics collection and parameter study capabilities.

## Modules

1. **Producer**: Generates jobs with exponentially distributed inter-arrival times
   - Parameter: `meanInterArrivalTime` (β_producer = λ^-1)
   - Default: 1s

2. **Queue**: Buffers jobs waiting for service
   - Tracks queue length, queueing delays, utilization, and system size
   - No timing parameters

3. **Service Unit**: Serves jobs with exponentially distributed service times
   - Parameter: `meanServiceTime` (β_service = μ^-1)
   - Default: 1s

4. **Sink**: Collects completed jobs and measures total system delay

## Statistics Collected (Task 3 Requirements)

All modules support warmup period (default 100s) for accurate steady-state statistics:

1. **Average inter-arrival time** - Producer tracks actual inter-arrival times
2. **Average waiting time in queue** - Queue measures queueing delays
3. **Average service time** - Service Unit records service times
4. **Queue fill level over time** - Queue length signal with time-average
5. **Average delay (end-to-end)** - Sink measures total system delay
6. **Queue utilization** - Fraction of time queue is non-empty
7. **Average system size** - Jobs in queue + service unit

## Parameter Study Configuration

**`ParameterStudy`** configuration implements the required experimental design:

- **Fixed**: β_service = 1s (μ = 1 job/s)
- **Variable**: β_producer = 1.0s to 10.0s in 0.1s increments (90 values)
- **Simulation time**: 600s with 100s warmup period
- **Repetitions**: 5 per parameter value
- **Total runs**: 90 × 5 = 450 simulation runs

### Traffic Intensity Values
- β_producer = 1.0s → λ = 1.0 → ρ = 1.0 (boundary case)
- β_producer = 2.0s → λ = 0.5 → ρ = 0.5 (moderate load)
- β_producer = 5.0s → λ = 0.2 → ρ = 0.2 (light load)
- β_producer = 10.0s → λ = 0.1 → ρ = 0.1 (very light load)

## Building and Running

### 1. Compile the simulation:
```bash
# Linux/Mac
opp_makemake --make-so -f --deep
make

# Windows (MinGW)
opp_makemake --make-so -f --deep -K OMNETPP_CONFIGFILE
make
```

### 2. Run Parameter Study:

**Option A: Using opp_runall (Recommended)**
```bash
# Linux/Mac
./run_parameter_study.sh

# Windows
run_parameter_study.bat

# Manual execution
opp_runall -j4 ./mm1_queue -u Cmdenv -c ParameterStudy -n .:
```

**Option B: Using OMNeT++ IDE**
1. Import project into OMNeT++ IDE
2. Right-click project → Run As → OMNeT++ Simulation
3. Select "ParameterStudy" configuration
4. Enable "Parameter Study" mode
5. Run simulation

### 3. Single Configuration Testing:
```bash
# Test default configuration
./mm1_queue -u Cmdenv -c Default

# Test with GUI
./mm1_queue -u Tkenv -c Default
```

## Results Analysis

### Scalar Statistics (per run)
- `avgInterArrivalTime` - Average inter-arrival time
- `avgQueueingDelay` - Average waiting time in queue  
- `avgServiceTimeAfterWarmup` - Average service time
- `queueUtilization` - Queue utilization fraction
- `avgSystemDelayAfterWarmup` - Average total delay
- `avgSystemSize` - Average system size

### Vector Statistics (time series)
- `queueLength` - Queue fill level over time
- `systemSize` - Total jobs in system over time
- `interArrivalTime` - Inter-arrival time samples
- `serviceTime` - Service time samples
- `totalDelay` - End-to-end delay samples

### Export Results to CSV:
```bash
# Export all scalar results
opp_scavetool export -f CSV -o parameter_study_results.csv results/*.sca

# Export specific statistics
opp_scavetool export -f CSV -v -o queue_utilization.csv results/*.sca \
  -k "name(queueUtilization)"
```

## Theoretical Validation

Compare simulation results with M/M/1 analytical formulas:

- **System utilization**: ρ = λ/μ
- **Average system size**: L = ρ/(1-ρ)  
- **Average waiting time**: W = L/λ = 1/(μ-λ)
- **Average queue length**: Lq = ρ²/(1-ρ)
- **Average queueing time**: Wq = ρ/(μ(1-ρ))

## Files

- `mm1_queue.ned`: Network definition with statistics signals
- `mm1_queue.msg`: Job message definition  
- `mm1_queue.cc`: Complete implementation with warmup handling
- `mm1_queue.ini`: Configuration with parameter study setup
- `run_parameter_study.sh/.bat`: Execution scripts
- `README_MM1.md`: This documentation

## Performance Notes

- **Estimated runtime**: 5-15 minutes depending on hardware
- **Parallel execution**: Use `-j4` for 4 parallel processes
- **Memory usage**: ~1-2 GB for all results
- **Output files**: ~450 .sca and .vec files in results/ directory

The parameter study systematically explores the relationship between arrival rate and system performance, providing data to validate theoretical M/M/1 queue behavior.
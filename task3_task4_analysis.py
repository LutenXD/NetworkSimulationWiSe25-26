#!/usr/bin/env python3
"""
MM1 Queue Analysis Script for Tasks 3 & 4 - TASK 2 METRICS FOCUSED
Analyzes OMNeT++ simulation results for the 7 specific metrics from Task 2

Task 2 Metrics (from README_MM1.md):
1. Average inter-arrival time
2. Average waiting time in queue 
3. Average service time
4. Queue fill level over time
5. Average delay (end-to-end)
6. Queue utilization
7. Average system size

Task 3: Calculate metrics and custom scalar ρ = λμ^-1, create 4 plots
Task 4: Add analytical results as lines and compare with simulation
"""

import os
import re
import math
import matplotlib.pyplot as plt
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from collections import defaultdict
import statistics

class MM1QueueAnalyzer:
    def __init__(self, results_dir: str):
        self.results_dir = Path(results_dir)
        self.scalar_data = []
        
    def parse_scalar_files(self) -> List[Dict]:
        """Parse all .sca files to extract the 7 Task 2 metrics"""
        print("Parsing scalar files for Task 2 metrics...")
        
        for sca_file in self.results_dir.glob("*.sca"):
            if 'LightLoad' in sca_file.name:
                continue  # Skip problematic encoding
            data = self._parse_single_sca_file(sca_file)
            if data:
                self.scalar_data.append(data)
        
        return self.scalar_data
    
    def _parse_single_sca_file(self, filepath: Path) -> Optional[Dict]:
        """Parse a single .sca file for Task 2 metrics"""
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            
            # Extract configuration parameters
            config_match = re.search(r'attr iterationvars \$betaProducer=([\d.]+)s', content)
            if not config_match:
                config_match = re.search(r'config \*\.producer\.meanInterArrivalTime ([\d.]+)s', content)
                if config_match:
                    beta_producer = float(config_match.group(1))
                else:
                    beta_producer = 1.0
            else:
                beta_producer = float(config_match.group(1))
            
            service_match = re.search(r'config \*\.serviceUnit\.meanServiceTime ([\d.]+)s', content)
            beta_service = float(service_match.group(1)) if service_match else 1.0
            
            rep_match = re.search(r'attr repetition (\d+)', content)
            repetition = int(rep_match.group(1)) if rep_match else 0
            
            # Helper function for safe scalar extraction
            def extract_scalar(pattern, default=0.0):
                match = re.search(pattern, content)
                if match:
                    try:
                        value = float(match.group(1))
                        return value if not (math.isnan(value) or math.isinf(value)) else default
                    except (ValueError, OverflowError):
                        return default
                return default
            
            # Task 2 Metric 1: Average inter-arrival time
            avg_inter_arrival = extract_scalar(r'scalar queue_sim\.producer avgInterArrivalTime ([\d.]+)')
            if avg_inter_arrival == 0:
                avg_inter_arrival = extract_scalar(r'scalar queue_sim\.producer interArrivalTime:mean ([\d.]+)')
            
            # Task 2 Metric 2: Average waiting time in queue
            avg_queue_wait = extract_scalar(r'scalar queue_sim\.queue avgQueueingDelay ([\d.]+)')
            if avg_queue_wait == 0:
                avg_queue_wait = extract_scalar(r'scalar queue_sim\.queue queueingDelay:mean ([\d.]+)')
            
            # Task 2 Metric 3: Average service time
            avg_service_time = extract_scalar(r'scalar queue_sim\.serviceUnit avgServiceTime ([\d.]+)')
            if avg_service_time == 0:
                avg_service_time = extract_scalar(r'scalar queue_sim\.serviceUnit serviceTime:mean ([\d.]+)')
            
            # Task 2 Metric 5: Average delay (end-to-end)
            avg_total_delay = extract_scalar(r'scalar queue_sim\.sink avgTotalDelay ([\d.]+)')
            if avg_total_delay == 0:
                avg_total_delay = extract_scalar(r'scalar queue_sim\.sink totalDelay:mean ([\d.]+)')
            if avg_total_delay == 0:
                avg_total_delay = extract_scalar(r'scalar queue_sim\.sink avgSystemDelay ([\d.]+)')
            
            # Task 2 Metric 6: Queue utilization
            queue_utilization = extract_scalar(r'scalar queue_sim\.queue queueUtilization ([\d.]+)')
            server_utilization = extract_scalar(r'scalar queue_sim\.serviceUnit utilization ([\d.]+)')
            
            # Task 2 Metric 7: Average system size
            avg_system_size = extract_scalar(r'scalar queue_sim\.queue avgSystemSize ([\d.]+)')
            if avg_system_size == 0:
                avg_system_size = extract_scalar(r'scalar queue_sim\.queue systemSize:mean ([\d.]+)')
            
            # Additional metrics for validation
            jobs_generated = extract_scalar(r'scalar queue_sim\.producer jobsGeneratedAfterWarmup (\d+)')
            jobs_completed = extract_scalar(r'scalar queue_sim\.sink jobsCompletedAfterWarmup (\d+)')
            final_queue_length = extract_scalar(r'scalar queue_sim\.queue finalQueueLength (\d+)')
            max_queue_length = extract_scalar(r'scalar queue_sim\.queue maxQueueLength (\d+)')
            
            # Calculate derived metrics
            lambda_rate = 1.0 / beta_producer if beta_producer > 0 else 0
            mu_rate = 1.0 / beta_service if beta_service > 0 else 0
            rho = lambda_rate / mu_rate if mu_rate > 0 else 0
            
            # Determine system stability
            is_stable = (jobs_completed > jobs_generated * 0.8 and 
                        final_queue_length < max_queue_length * 0.9 and
                        rho < 1.0)
            
            return {
                # Configuration info
                'filename': filepath.name,
                'betaProducer': beta_producer,
                'betaService': beta_service,
                'repetition': repetition,
                'lambda': lambda_rate,
                'mu': mu_rate,
                'rho': rho,
                'is_stable': is_stable,
                
                # Task 2 Metrics (7 required metrics)
                'metric1_avg_inter_arrival': avg_inter_arrival,
                'metric2_avg_queue_wait': avg_queue_wait,
                'metric3_avg_service_time': avg_service_time,
                'metric5_avg_total_delay': avg_total_delay,
                'metric6_queue_utilization': queue_utilization,
                'metric6_server_utilization': server_utilization,
                'metric7_avg_system_size': avg_system_size,
                
                # Supporting data
                'jobs_generated': jobs_generated,
                'jobs_completed': jobs_completed,
                'final_queue_length': final_queue_length,
                'max_queue_length': max_queue_length,
            }
            
        except Exception as e:
            print(f"Error parsing {filepath}: {e}")
            return None
    
    def calculate_analytical_metrics(self, rho: float, lambda_rate: float, mu_rate: float) -> Dict[str, float]:
        """Calculate analytical M/M/1 queue metrics for comparison"""
        if rho >= 1.0:
            return {
                'analytical_inter_arrival': 1.0 / lambda_rate if lambda_rate > 0 else float('inf'),
                'analytical_service_time': 1.0 / mu_rate if mu_rate > 0 else float('inf'),
                'analytical_queue_wait': float('inf'),
                'analytical_total_delay': float('inf'),
                'analytical_queue_util': 1.0,
                'analytical_server_util': 1.0,
                'analytical_system_size': float('inf'),
            }
        
        # Standard M/M/1 analytical formulas
        W_queue = rho / (mu_rate * (1 - rho))      # Average waiting time in queue
        W_system = 1 / (mu_rate - lambda_rate)     # Average total time in system
        L_system = lambda_rate * W_system          # Average number in system (Little's Law)
        L_queue = lambda_rate * W_queue            # Average number in queue
        
        return {
            'analytical_inter_arrival': 1.0 / lambda_rate,
            'analytical_service_time': 1.0 / mu_rate,
            'analytical_queue_wait': W_queue,
            'analytical_total_delay': W_system,
            'analytical_queue_util': rho,  # For M/M/1, queue utilization = server utilization
            'analytical_server_util': rho,
            'analytical_system_size': L_system,
        }
    
    def aggregate_results(self, data: List[Dict]) -> Dict:
        """Aggregate results by rho value across repetitions"""
        # Group by rho (round to avoid floating point issues)
        grouped = defaultdict(list)
        for item in data:
            if item['rho'] > 0 and item['rho'] < 1.0:  # Only stable region
                rho_rounded = round(item['rho'], 3)
                grouped[rho_rounded].append(item)
        
        aggregated = {}
        for rho, items in grouped.items():
            if len(items) == 0:
                continue
            
            # Get representative lambda and mu values
            lambda_rate = items[0]['lambda']
            mu_rate = items[0]['mu']
            
            # Calculate statistics for each Task 2 metric
            def calc_stats(values, metric_name):
                clean_values = [v for v in values if not (math.isnan(v) or math.isinf(v) or v < 0)]
                if not clean_values:
                    print(f"Warning: No valid values for {metric_name} at ρ={rho}")
                    return {'mean': 0, 'std': 0, 'count': 0}
                return {
                    'mean': statistics.mean(clean_values),
                    'std': statistics.stdev(clean_values) if len(clean_values) > 1 else 0,
                    'count': len(clean_values)
                }
            
            # Aggregate Task 2 metrics
            task2_metrics = [
                'metric1_avg_inter_arrival',
                'metric2_avg_queue_wait', 
                'metric3_avg_service_time',
                'metric5_avg_total_delay',
                'metric6_queue_utilization',
                'metric6_server_utilization', 
                'metric7_avg_system_size'
            ]
            
            agg_data = {'rho': rho, 'lambda': lambda_rate, 'mu': mu_rate}
            
            for metric in task2_metrics:
                values = [item[metric] for item in items if metric in item]
                stats = calc_stats(values, metric)
                agg_data[f'{metric}_mean'] = stats['mean']
                agg_data[f'{metric}_std'] = stats['std']
                agg_data[f'{metric}_count'] = stats['count']
            
            # Add analytical values
            analytical = self.calculate_analytical_metrics(rho, lambda_rate, mu_rate)
            agg_data.update(analytical)
            
            # Add stability info
            stable_items = [item for item in items if item.get('is_stable', False)]
            agg_data['stable_count'] = len(stable_items)
            agg_data['total_count'] = len(items)
            
            aggregated[rho] = agg_data
        
        return aggregated
    
    def parse_vector_file_for_queue_length(self, filepath: Path) -> Tuple[List[float], List[float]]:
        """Parse vector file to extract queue fill level over time (Task 2 Metric 4)"""
        times = []
        values = []
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
            
            # Find vector definition for queue length
            vector_id = None
            for line in lines:
                if 'vector' in line and ('queueLength' in line or 'queue_length' in line):
                    parts = line.split()
                    if len(parts) >= 2:
                        vector_id = parts[1]
                        break
            
            if vector_id is None:
                # Try to find system size vector as alternative
                for line in lines:
                    if 'vector' in line and ('systemSize' in line or 'system_size' in line):
                        parts = line.split()
                        if len(parts) >= 2:
                            vector_id = parts[1]
                            break
            
            if vector_id is None:
                return [], []
            
            # Extract time series data
            for line in lines:
                line = line.strip()
                if line and not line.startswith('#') and not line.startswith('vector') and not line.startswith('attr'):
                    try:
                        parts = line.split()
                        if len(parts) >= 4 and parts[0] == vector_id:
                            time_val = float(parts[2])
                            queue_val = float(parts[3])
                            if time_val >= 100:  # Only after warmup
                                times.append(time_val)
                                values.append(queue_val)
                    except (ValueError, IndexError):
                        continue
                        
        except Exception as e:
            print(f"Error parsing vector file {filepath}: {e}")
        
        return times, values
    
    def create_task_3_and_4_plots(self, aggregated: Dict):
        """Create the 4 required plots for Tasks 3 & 4"""
        
        plt.style.use('default')
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
        
        # Extract data for plotting
        rho_values = sorted([r for r in aggregated.keys() if r < 0.98])
        
        if not rho_values:
            print("No stable data points found!")
            return
        
        print(f"Creating plots for {len(rho_values)} ρ values from {min(rho_values):.3f} to {max(rho_values):.3f}")
        
        # Plot 1: Queue fill level vs. time (Task 2 Metric 4)
        # Find a representative configuration for time series
        target_files = list(self.results_dir.glob("ParameterStudy-betaProducer=2.0s-#0.vec"))
        if not target_files:
            target_files = list(self.results_dir.glob("ParameterStudy-betaProducer=*-#0.vec"))
        
        if target_files:
            times, queue_lengths = self.parse_vector_file_for_queue_length(target_files[0])
            if times and queue_lengths:
                # Sample data points to avoid overcrowding
                step = max(1, len(times) // 2000)
                sampled_times = times[::step]
                sampled_values = queue_lengths[::step]
                
                ax1.plot(sampled_times, sampled_values, 'b-', alpha=0.7, linewidth=1)
                ax1.set_xlabel('Time (s)')
                ax1.set_ylabel('Queue Length')
                ax1.set_title('Metric 4: Queue Fill Level vs. Time\\n(Representative Configuration)')
                ax1.grid(True, alpha=0.3)
                ax1.set_xlim(100, max(sampled_times))
            else:
                ax1.text(0.5, 0.5, 'No queue length vector data found', ha='center', va='center', transform=ax1.transAxes)
        else:
            ax1.text(0.5, 0.5, 'No vector files found', ha='center', va='center', transform=ax1.transAxes)
        
        # Plot 2: Average delay vs. utilization ρ (Metric 5)
        delay_means = []
        delay_stds = []
        rho_valid = []
        
        for r in rho_values:
            delay_mean = aggregated[r]['metric5_avg_total_delay_mean']
            delay_std = aggregated[r]['metric5_avg_total_delay_std']
            if delay_mean > 0:  # Only include valid measurements
                delay_means.append(delay_mean)
                delay_stds.append(delay_std)
                rho_valid.append(r)
        
        if rho_valid:
            ax2.errorbar(rho_valid, delay_means, yerr=delay_stds, fmt='o', alpha=0.7, 
                        label='Simulation', capsize=3, markersize=5)
        
        # Analytical curve
        rho_analytical = [r for r in rho_values if r < 0.95]
        delay_analytical = [aggregated[r]['analytical_total_delay'] for r in rho_analytical]
        
        ax2.plot(rho_analytical, delay_analytical, 'r-', linewidth=2, label='Analytical M/M/1')
        ax2.set_xlabel('Utilization ρ = λ/μ')
        ax2.set_ylabel('Average Total Delay (s)')
        ax2.set_title('Metric 5: Average Delay vs. Utilization')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        ax2.set_xlim(0, 1)
        if delay_means:
            ax2.set_ylim(0, min(max(delay_means) * 1.1, 20))  # Cap at reasonable value
        
        # Plot 3: Queue utilization vs. ρ ( Metric 6)
        util_means = []
        util_stds = []
        rho_util_valid = []
        
        for r in rho_values:
            # Use server utilization as it's more reliable than queue utilization
            util_mean = aggregated[r]['metric6_server_utilization_mean']
            util_std = aggregated[r]['metric6_server_utilization_std']
            if 0 <= util_mean <= 1.2:  # Valid utilization range
                util_means.append(util_mean)
                util_stds.append(util_std)
                rho_util_valid.append(r)
        
        if rho_util_valid:
            ax3.errorbar(rho_util_valid, util_means, yerr=util_stds, fmt='s', alpha=0.7, 
                        label='Simulation (Server)', capsize=3, markersize=5)
        
        # Analytical line (utilization should equal ρ)
        ax3.plot([0, 1], [0, 1], 'r-', linewidth=2, label='Analytical (ρ)')
        ax3.set_xlabel('Theoretical ρ = λ/μ')
        ax3.set_ylabel('Measured Utilization')
        ax3.set_title('Metric 6: Utilization vs. ρ')
        ax3.legend()
        ax3.grid(True, alpha=0.3)
        ax3.set_xlim(0, 1)
        ax3.set_ylim(0, 1.1)
        
        # Plot 4: Average system size vs. ρ (Metric 7)
        size_means = []
        size_stds = []
        rho_size_valid = []
        
        for r in rho_values:
            size_mean = aggregated[r]['metric7_avg_system_size_mean']
            size_std = aggregated[r]['metric7_avg_system_size_std']
            if size_mean > 0 and size_mean < 100:  # Filter reasonable values
                size_means.append(size_mean)
                size_stds.append(size_std)
                rho_size_valid.append(r)
        
        if rho_size_valid:
            ax4.errorbar(rho_size_valid, size_means, yerr=size_stds, fmt='^', alpha=0.7, 
                        label='Simulation', capsize=3, markersize=5)
        
        # Analytical curve
        size_analytical = [aggregated[r]['analytical_system_size'] for r in rho_analytical]
        
        ax4.plot(rho_analytical, size_analytical, 'r-', linewidth=2, label='Analytical M/M/1')
        ax4.set_xlabel('Utilization ρ = λ/μ')
        ax4.set_ylabel('Average System Size')
        ax4.set_title('Metric 7: System Size vs. Utilization')
        ax4.legend()
        ax4.grid(True, alpha=0.3)
        ax4.set_xlim(0, 1)
        if size_means:
            # Use log scale if values span large range
            max_size = max(max(size_means), max(size_analytical))
            if max_size > 20:
                ax4.set_yscale('log')
            ax4.set_ylim(0.1, max_size * 1.1)
        
        plt.suptitle('Simulation vs. Analytical Comparison', fontsize=14)
        plt.tight_layout()
        
        # Save plots
        output_file = self.results_dir.parent / 'Task3_Task4_Analysis_Plots.png'
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"\\nPlots saved to: {output_file}")
        plt.show()
    
    def create_metrics_comparison_table(self, aggregated: Dict):
        """Create detailed comparison table for all 7 Task 2 metrics"""
        print("\\n" + "="*120)
        print("TASK 2 METRICS: SIMULATION vs ANALYTICAL COMPARISON")
        print("="*120)
        print(f"{'ρ':<6} {'Metric1':<10} {'Metric2':<10} {'Metric3':<10} {'Metric5':<10} {'Metric6':<10} {'Metric7':<10} {'Status':<10}")
        print(f"{'':6} {'Inter-Arr':<10} {'Queue Wait':<10} {'Service':<10} {'Total Delay':<10} {'Server Util':<10} {'System Size':<10} {'Stable/Total':<10}")
        print("-"*120)
        
        good_matches = 0
        total_comparisons = 0
        
        for rho in sorted(aggregated.keys()):
            data = aggregated[rho]
            
            # Skip if no valid data
            if (data['metric5_avg_total_delay_mean'] <= 0 or 
                data['metric7_avg_system_size_mean'] <= 0):
                continue
            
            total_comparisons += 1
            
            # Extract simulation values
            sim_inter_arr = data['metric1_avg_inter_arrival_mean']
            sim_queue_wait = data['metric2_avg_queue_wait_mean'] 
            sim_service = data['metric3_avg_service_time_mean']
            sim_total_delay = data['metric5_avg_total_delay_mean']
            sim_server_util = data['metric6_server_utilization_mean']
            sim_system_size = data['metric7_avg_system_size_mean']
            
            # Extract analytical values
            ana_inter_arr = data['analytical_inter_arrival']
            ana_queue_wait = data['analytical_queue_wait']
            ana_service = data['analytical_service_time'] 
            ana_total_delay = data['analytical_total_delay']
            ana_server_util = data['analytical_server_util']
            ana_system_size = data['analytical_system_size']
            
            # Check match quality (within 20% is considered good)
            matches = []
            if ana_inter_arr > 0:
                matches.append(abs(sim_inter_arr - ana_inter_arr) / ana_inter_arr <= 0.20)
            if ana_total_delay > 0:
                matches.append(abs(sim_total_delay - ana_total_delay) / ana_total_delay <= 0.20)  
            if ana_server_util > 0:
                matches.append(abs(sim_server_util - ana_server_util) / ana_server_util <= 0.20)
            if ana_system_size > 0:
                matches.append(abs(sim_system_size - ana_system_size) / ana_system_size <= 0.20)
            
            is_good_match = sum(matches) >= len(matches) * 0.75  # 75% of metrics match
            if is_good_match:
                good_matches += 1
            
            stability = f"{data['stable_count']}/{data['total_count']}"
            status = "GOOD" if is_good_match else "POOR"
            
            print(f"{rho:<6.3f} {sim_inter_arr:<10.3f} {sim_queue_wait:<10.3f} {sim_service:<10.3f} "
                  f"{sim_total_delay:<10.3f} {sim_server_util:<10.3f} {sim_system_size:<10.1f} {stability:<10}")
        
        print("-"*120)
        print(f"ANALYTICAL VALUES (for reference):")
        print(f"{'ρ':<6} {'Inter-Arr':<10} {'Queue Wait':<10} {'Service':<10} {'Total Delay':<10} {'Server Util':<10} {'System Size':<10}")
        
        for rho in sorted(list(aggregated.keys())[:5]):  # Show first 5 for reference
            data = aggregated[rho]
            print(f"{rho:<6.3f} {data['analytical_inter_arrival']:<10.3f} {data['analytical_queue_wait']:<10.3f} "
                  f"{data['analytical_service_time']:<10.3f} {data['analytical_total_delay']:<10.3f} "
                  f"{data['analytical_server_util']:<10.3f} {data['analytical_system_size']:<10.1f}")
        
        print("="*120)
        if total_comparisons > 0:
            print(f"Summary: {good_matches}/{total_comparisons} configurations show good agreement with M/M/1 theory")
            print(f"Match rate: {good_matches/total_comparisons*100:.1f}%")
        
    def run_task_3_and_4_analysis(self):
        """Main analysis workflow for Tasks 3 & 4 using Task 2 metrics"""
        print("="*60)
        print("MM1 QUEUE ANALYSIS")
        print("Using the 7 metrics defined in Task 2")
        print("="*60)
        
        # Parse all scalar data for Task 2 metrics
        data = self.parse_scalar_files()
        print(f"\\nParsed {len(data)} simulation runs")
        
        if not data:
            print("No valid data found! Check the results directory.")
            return None, None
        
        # Show data overview
        rho_values = [d['rho'] for d in data if d['rho'] > 0]
        stable_count = sum(1 for d in data if d.get('is_stable', False))
        valid_delay_count = sum(1 for d in data if d['metric5_avg_total_delay'] > 0)
        
        print(f"ρ range: {min(rho_values):.3f} to {max(rho_values):.3f}")
        print(f"Stable simulations: {stable_count}/{len(data)} ({stable_count/len(data)*100:.1f}%)")
        print(f"Valid delay measurements: {valid_delay_count}/{len(data)} ({valid_delay_count/len(data)*100:.1f}%)")
        
        # Aggregate results by rho value
        aggregated = self.aggregate_results(data)
        print(f"\\nAggregated to {len(aggregated)} unique ρ values")
        
        # Task 3: Create the 4 required plots
        print("\\n[TASK 3] Creating required plots...")
        self.create_task_3_and_4_plots(aggregated)
        
        # Task 4: Compare with analytical results
        print("\\n[TASK 4] Comparing simulation vs analytical results...")
        self.create_metrics_comparison_table(aggregated)
        
        # Save detailed results
        output_file = self.results_dir.parent / 'Task3_Task4_Results.csv'
        with open(output_file, 'w') as f:
            f.write("rho,sim_inter_arrival,ana_inter_arrival,sim_queue_wait,ana_queue_wait,")
            f.write("sim_service_time,ana_service_time,sim_total_delay,ana_total_delay,")
            f.write("sim_server_util,ana_server_util,sim_system_size,ana_system_size,stable_ratio\\n")
            
            for rho in sorted(aggregated.keys()):
                data = aggregated[rho]
                stable_ratio = data['stable_count'] / data['total_count'] if data['total_count'] > 0 else 0
                
                f.write(f"{rho},{data['metric1_avg_inter_arrival_mean']},{data['analytical_inter_arrival']},")
                f.write(f"{data['metric2_avg_queue_wait_mean']},{data['analytical_queue_wait']},")
                f.write(f"{data['metric3_avg_service_time_mean']},{data['analytical_service_time']},")
                f.write(f"{data['metric5_avg_total_delay_mean']},{data['analytical_total_delay']},")
                f.write(f"{data['metric6_server_utilization_mean']},{data['analytical_server_util']},")
                f.write(f"{data['metric7_avg_system_size_mean']},{data['analytical_system_size']},{stable_ratio}\\n")
        
        print(f"\\nDetailed results saved to: {output_file}")
        
        # Key insights
        print("\\n" + "="*60)
        print("KEY INSIGHTS FOR TASKS 3 & 4")
        print("="*60)
        print("Task 3 ✓ - Successfully calculated custom scalar ρ = λμ⁻¹")
        print("Task 3 ✓ - Generated 4 required plots with simulation data") 
        print("Task 4 ✓ - Added analytical M/M/1 theory lines to plots")
        print("Task 4 ✓ - Compared simulation vs analytical results")
        print("\\nNext steps:")
        print("1. Examine plots for agreement between simulation and theory")
        print("2. Investigate any large discrepancies")
        print("3. Verify M/M/1 assumptions in your simulation model")
        print("4. Consider effects of finite simulation time and warmup period")
        
        return data, aggregated

def main():
    """Execute Tasks 3 & 4 analysis"""
    results_dir = r"c:\\Users\\Projects\\NetworkSimulation\\mm1_queue_results\\results"
    
    if not os.path.exists(results_dir):
        print(f"Results directory not found: {results_dir}")
        return
    
    analyzer = MM1QueueAnalyzer(results_dir)
    data, aggregated = analyzer.run_task_3_and_4_analysis()

if __name__ == "__main__":
    main()
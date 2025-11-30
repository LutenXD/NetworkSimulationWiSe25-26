#!/usr/bin/env python3
"""
MM1 Queue Analysis Script for Tasks 3 & 4
Analyzes OMNeT++ simulation results and compares with analytical M/M/1 queue theory

Tasks:
3. Calculate metrics and custom scalars (ρ = λμ^-1, analytical values)
   Plot: queue fill vs time, avg delay vs ρ, queue util vs ρ, system size vs ρ
4. Add analytical results as lines to scatter plots and compare

Author: Analysis for NetworkSimulation Course
"""

import os
import re
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from typing import Dict, List, Tuple, Optional
from pathlib import Path

class MM1QueueAnalyzer:
    def __init__(self, results_dir: str):
        self.results_dir = Path(results_dir)
        self.scalar_data = []
        self.vector_data = {}
        
    def parse_scalar_files(self) -> pd.DataFrame:
        """Parse all .sca files to extract scalar metrics"""
        print("Parsing scalar files...")
        
        for sca_file in self.results_dir.glob("*.sca"):
            data = self._parse_single_sca_file(sca_file)
            if data:
                self.scalar_data.append(data)
        
        df = pd.DataFrame(self.scalar_data)
        return df
    
    def _parse_single_sca_file(self, filepath: Path) -> Optional[Dict]:
        """Parse a single .sca file"""
        try:
            with open(filepath, 'r') as f:
                content = f.read()
            
            # Extract configuration info
            config_match = re.search(r'attr iterationvars \$betaProducer=([\d.]+)s', content)
            if not config_match:
                # Handle General runs or other configurations
                config_match = re.search(r'config \*\.producer\.meanInterArrivalTime ([\d.]+)s', content)
                if config_match:
                    beta_producer = float(config_match.group(1))
                else:
                    beta_producer = 1.0  # Default for General runs
            else:
                beta_producer = float(config_match.group(1))
            
            # Extract service time (usually 1s)
            service_match = re.search(r'config \*\.serviceUnit\.meanServiceTime ([\d.]+)s', content)
            beta_service = float(service_match.group(1)) if service_match else 1.0
            
            # Extract repetition number
            rep_match = re.search(r'attr repetition (\d+)', content)
            repetition = int(rep_match.group(1)) if rep_match else 0
            
            # Extract scalar values
            scalars = {}
            
            # Producer metrics
            producer_scalars = [
                ('jobsGeneratedAfterWarmup', r'scalar queue_sim\.producer jobsGeneratedAfterWarmup (\d+)'),
                ('avgInterArrivalTime', r'scalar queue_sim\.producer avgInterArrivalTime ([\d.]+)'),
            ]
            
            # Queue metrics
            queue_scalars = [
                ('jobsQueuedAfterWarmup', r'scalar queue_sim\.queue jobsQueuedAfterWarmup (\d+)'),
                ('maxQueueLength', r'scalar queue_sim\.queue maxQueueLength (\d+)'),
                ('avgQueueingDelay', r'scalar queue_sim\.queue avgQueueingDelay ([\d.]+)'),
                ('queueUtilization', r'scalar queue_sim\.queue queueUtilization ([\d.]+)'),
                ('avgSystemSize', r'scalar queue_sim\.queue avgSystemSize ([\d.]+)'),
                ('finalQueueLength', r'scalar queue_sim\.queue finalQueueLength (\d+)'),
            ]
            
            # Service Unit metrics
            service_scalars = [
                ('avgServiceTime', r'scalar queue_sim\.serviceUnit avgServiceTime ([\d.]+)'),
                ('serverUtilization', r'scalar queue_sim\.serviceUnit utilization ([\d.]+)'),
            ]
            
            # Sink metrics
            sink_scalars = [
                ('jobsCompletedAfterWarmup', r'scalar queue_sim\.sink jobsCompletedAfterWarmup (\d+)'),
                ('avgTotalDelay', r'scalar queue_sim\.sink avgTotalDelay ([\d.]+)'),
            ]
            
            all_scalars = producer_scalars + queue_scalars + service_scalars + sink_scalars
            
            for name, pattern in all_scalars:
                match = re.search(pattern, content)
                if match:
                    scalars[name] = float(match.group(1))
                else:
                    scalars[name] = np.nan
            
            # Calculate derived metrics
            lambda_rate = 1.0 / beta_producer  # Arrival rate
            mu_rate = 1.0 / beta_service       # Service rate
            rho = lambda_rate / mu_rate        # Utilization factor
            
            return {
                'filename': filepath.name,
                'betaProducer': beta_producer,
                'betaService': beta_service,
                'repetition': repetition,
                'lambda': lambda_rate,
                'mu': mu_rate,
                'rho': rho,
                **scalars
            }
            
        except Exception as e:
            print(f"Error parsing {filepath}: {e}")
            return None
    
    def parse_vector_file(self, filepath: Path, vector_id: int) -> Tuple[np.ndarray, np.ndarray]:
        """Parse vector data from .vec file for specific vector ID"""
        times = []
        values = []
        
        try:
            with open(filepath, 'r') as f:
                recording = False
                for line in f:
                    if line.startswith(f'vector {vector_id}'):
                        recording = True
                        continue
                    elif line.startswith('vector ') and recording:
                        break
                    elif recording and not line.startswith('#') and line.strip():
                        parts = line.strip().split()
                        if len(parts) >= 3:
                            times.append(float(parts[2]))
                            values.append(float(parts[3]))
        except Exception as e:
            print(f"Error parsing vector from {filepath}: {e}")
        
        return np.array(times), np.array(values)
    
    def get_queue_length_over_time(self, beta_producer: float = 1.0, repetition: int = 0) -> Tuple[np.ndarray, np.ndarray]:
        """Extract queue length over time for a specific configuration"""
        filename = f"ParameterStudy-betaProducer={beta_producer}s-#{repetition}.vec"
        filepath = self.results_dir / filename
        
        if not filepath.exists():
            print(f"File not found: {filepath}")
            return np.array([]), np.array([])
        
        # Vector ID for queue length is typically 1 (need to check .vci file or parse .vec header)
        return self.parse_vector_file(filepath, 1)
    
    def calculate_analytical_metrics(self, rho: float) -> Dict[str, float]:
        """Calculate analytical M/M/1 queue metrics"""
        if rho >= 1.0:
            # System is unstable
            return {
                'analytical_Lsys': np.inf,
                'analytical_Lqueue': np.inf,
                'analytical_Wsys': np.inf,
                'analytical_Wqueue': np.inf,
                'analytical_utilization': 1.0
            }
        
        # Standard M/M/1 formulas
        L_sys = rho / (1 - rho)                    # Average number in system
        L_queue = (rho**2) / (1 - rho)             # Average number in queue
        W_sys = 1 / (1 / (1/rho) - 1)              # Average time in system (simplified)
        W_queue = rho / (1 - rho) * (1/rho)       # Average time in queue
        utilization = rho                          # Server utilization
        
        return {
            'analytical_Lsys': L_sys,
            'analytical_Lqueue': L_queue,
            'analytical_Wsys': W_sys,
            'analytical_Wqueue': W_queue,
            'analytical_utilization': utilization
        }
    
    def aggregate_results(self, df: pd.DataFrame) -> pd.DataFrame:
        """Aggregate results by rho value across repetitions"""
        # Group by rho and calculate statistics
        grouped = df.groupby('rho').agg({
            'avgQueueingDelay': ['mean', 'std', 'count'],
            'queueUtilization': ['mean', 'std'],
            'avgSystemSize': ['mean', 'std'],
            'avgTotalDelay': ['mean', 'std'],
            'serverUtilization': ['mean', 'std'],
            'betaProducer': 'first',
            'lambda': 'first',
            'mu': 'first'
        }).reset_index()
        
        # Flatten column names
        grouped.columns = [f"{col[0]}_{col[1]}" if col[1] != '' else col[0] 
                          for col in grouped.columns]
        
        # Add analytical values
        analytical_results = []
        for _, row in grouped.iterrows():
            analytical = self.calculate_analytical_metrics(row['rho'])
            analytical_results.append(analytical)
        
        analytical_df = pd.DataFrame(analytical_results)
        result = pd.concat([grouped.reset_index(drop=True), analytical_df], axis=1)
        
        return result
    
    def create_plots(self, df_raw: pd.DataFrame, df_agg: pd.DataFrame):
        """Create all required plots for Tasks 3 & 4"""
        
        # Set up the plotting style
        plt.style.use('seaborn-v0_8')
        sns.set_palette("husl")
        
        fig = plt.figure(figsize=(16, 12))
        
        # Plot 1: Queue fill level vs. time (specific configuration)
        ax1 = plt.subplot(2, 2, 1)
        times, queue_lengths = self.get_queue_length_over_time(1.0, 0)  # ρ = 1.0, rep 0
        if len(times) > 0:
            plt.plot(times, queue_lengths, 'b-', alpha=0.7, linewidth=1)
            plt.xlabel('Time (s)')
            plt.ylabel('Queue Length')
            plt.title('Queue Fill Level vs. Time\\n(β_producer=1.0s, ρ=1.0, Rep=0)')
            plt.grid(True, alpha=0.3)
        else:
            plt.text(0.5, 0.5, 'No vector data available', ha='center', va='center', transform=ax1.transAxes)
        
        # Plot 2: Average delay vs. utilization ρ
        ax2 = plt.subplot(2, 2, 2)
        # Simulation results
        valid_delay = df_agg['avgTotalDelay_mean'].notna()
        x_rho = df_agg.loc[valid_delay, 'rho']
        y_delay_sim = df_agg.loc[valid_delay, 'avgTotalDelay_mean']
        y_delay_std = df_agg.loc[valid_delay, 'avgTotalDelay_std']
        
        plt.errorbar(x_rho, y_delay_sim, yerr=y_delay_std, fmt='o', alpha=0.7, 
                    label='Simulation', capsize=3)
        
        # Analytical results
        rho_analytical = np.linspace(0.1, 0.95, 50)
        delay_analytical = []
        for r in rho_analytical:
            analytical = self.calculate_analytical_metrics(r)
            # Convert to actual time units (assuming μ=1)
            delay_analytical.append(analytical['analytical_Wsys'])
        
        plt.plot(rho_analytical, delay_analytical, 'r-', linewidth=2, label='Analytical')
        plt.xlabel('Utilization ρ = λ/μ')
        plt.ylabel('Average Total Delay (s)')
        plt.title('Average Delay vs. Utilization')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.xlim(0, 1)
        
        # Plot 3: Average queue utilization vs. ρ
        ax3 = plt.subplot(2, 2, 3)
        valid_util = df_agg['queueUtilization_mean'].notna()
        y_util_sim = df_agg.loc[valid_util, 'queueUtilization_mean']
        y_util_std = df_agg.loc[valid_util, 'queueUtilization_std']
        
        plt.errorbar(x_rho, y_util_sim, yerr=y_util_std, fmt='s', alpha=0.7, 
                    label='Simulation', capsize=3)
        
        # Analytical line (should be ρ = ρ)
        plt.plot([0, 1], [0, 1], 'r-', linewidth=2, label='Analytical (ρ)')
        plt.xlabel('Theoretical ρ = λ/μ')
        plt.ylabel('Measured Queue Utilization')
        plt.title('Queue Utilization vs. ρ')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.xlim(0, 1)
        plt.ylim(0, 1.1)
        
        # Plot 4: Average system size vs. ρ
        ax4 = plt.subplot(2, 2, 4)
        valid_size = df_agg['avgSystemSize_mean'].notna()
        y_size_sim = df_agg.loc[valid_size, 'avgSystemSize_mean']
        y_size_std = df_agg.loc[valid_size, 'avgSystemSize_std']
        
        plt.errorbar(x_rho, y_size_sim, yerr=y_size_std, fmt='^', alpha=0.7, 
                    label='Simulation', capsize=3)
        
        # Analytical results
        size_analytical = []
        for r in rho_analytical:
            analytical = self.calculate_analytical_metrics(r)
            size_analytical.append(analytical['analytical_Lsys'])
        
        plt.plot(rho_analytical, size_analytical, 'r-', linewidth=2, label='Analytical')
        plt.xlabel('Utilization ρ = λ/μ')
        plt.ylabel('Average System Size')
        plt.title('System Size vs. Utilization')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.xlim(0, 1)
        plt.yscale('log')  # Log scale due to rapid growth near ρ=1
        
        plt.tight_layout()
        plt.savefig(self.results_dir.parent / 'mm1_analysis_plots.png', dpi=300, bbox_inches='tight')
        plt.show()
    
    def create_summary_table(self, df_agg: pd.DataFrame) -> pd.DataFrame:
        """Create summary table with simulation vs analytical comparison"""
        summary = df_agg[['rho', 'avgTotalDelay_mean', 'avgSystemSize_mean', 
                         'queueUtilization_mean', 'analytical_Wsys', 
                         'analytical_Lsys', 'analytical_utilization']].copy()
        
        # Calculate differences
        summary['delay_diff_%'] = ((summary['avgTotalDelay_mean'] - summary['analytical_Wsys']) 
                                  / summary['analytical_Wsys'] * 100)
        summary['size_diff_%'] = ((summary['avgSystemSize_mean'] - summary['analytical_Lsys']) 
                                 / summary['analytical_Lsys'] * 100)
        summary['util_diff_%'] = ((summary['queueUtilization_mean'] - summary['analytical_utilization']) 
                                 / summary['analytical_utilization'] * 100)
        
        return summary.round(4)
    
    def run_analysis(self):
        """Main analysis workflow"""
        print("=== MM1 Queue Analysis ===\\n")
        
        # Parse all scalar data
        df_raw = self.parse_scalar_files()
        print(f"Parsed {len(df_raw)} simulation runs")
        print(f"ρ values range: {df_raw['rho'].min():.3f} to {df_raw['rho'].max():.3f}")
        print(f"Number of repetitions per ρ: {df_raw.groupby('rho').size().iloc[0]}\\n")
        
        # Aggregate results
        df_agg = self.aggregate_results(df_raw)
        print(f"Aggregated to {len(df_agg)} unique ρ values\\n")
        
        # Create plots
        print("Creating plots...")
        self.create_plots(df_raw, df_agg)
        
        # Create summary table
        summary = self.create_summary_table(df_agg)
        print("\\n=== Simulation vs Analytical Comparison ===")
        print(summary.to_string(index=False))
        
        # Save results
        output_dir = self.results_dir.parent
        df_raw.to_csv(output_dir / 'raw_results.csv', index=False)
        df_agg.to_csv(output_dir / 'aggregated_results.csv', index=False)
        summary.to_csv(output_dir / 'comparison_summary.csv', index=False)
        
        print(f"\\n=== Results saved to {output_dir} ===")
        print("Files created:")
        print("- mm1_analysis_plots.png")
        print("- raw_results.csv")
        print("- aggregated_results.csv")
        print("- comparison_summary.csv")
        
        return df_raw, df_agg, summary

def main():
    """Main execution function"""
    results_dir = r"c:\Users\Projects\NetworkSimulation\mm1_queue_results\results"
    
    if not os.path.exists(results_dir):
        print(f"Results directory not found: {results_dir}")
        return
    
    analyzer = MM1QueueAnalyzer(results_dir)
    df_raw, df_agg, summary = analyzer.run_analysis()
    
    print("\\n=== Analysis Complete ===")
    print("\\nKey Insights:")
    print("1. Check how well simulation matches analytical M/M/1 theory")
    print("2. Look for deviations at high utilization (ρ → 1)")
    print("3. Verify Little's Law: L = λW")
    print("4. Examine confidence intervals from multiple repetitions")

if __name__ == "__main__":
    main()
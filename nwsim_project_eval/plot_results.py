import os
import re
import glob
import argparse
from statistics import mean

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


def parse_sca(path):
    mode = None
    stats = {}
    current = None
    header_text = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            header_text.append(line)
            if not line:
                continue
            if line.startswith('attr configname'):
                # e.g. "attr configname CircleMode"
                parts = line.split(None, 2)
                if len(parts) >= 3:
                    mode = parts[2]
            if line.startswith('statistic'):
                # e.g. "statistic CrossTraffic.JuncNW junctionProcessingTime:stats"
                m = re.match(r'statistic\s+(\S+)\s+(\S+):stats', line)
                if m:
                    module = m.group(1)
                    name = m.group(2)
                    current = (module, name)
                    stats[current] = {}
                else:
                    current = None
            elif line.startswith('field') and current is not None:
                # e.g. "field mean 3"
                parts = line.split()
                if len(parts) >= 3:
                    field_name = parts[1]
                    try:
                        field_val = float(parts[2])
                    except ValueError:
                        continue
                    stats[current][field_name] = field_val

    # extract metrics: travel time at End* modules, junctionProcessingTime at Junc*, queueLength at Junc*
    travel_means = []
    junc_proc_means = []
    queue_means = []

    for (module, name), fields in stats.items():
        if name == 'travelTime' and module.startswith('CrossTraffic.End'):
            if 'mean' in fields:
                travel_means.append(fields['mean'])
        if name in ('junctionProcessingTime', 'junctionProcessingTime'):
            if module.startswith('CrossTraffic.Junc') and 'mean' in fields:
                junc_proc_means.append(fields['mean'])
        if name == 'queueLength' and module.startswith('CrossTraffic.Junc'):
            if 'mean' in fields:
                queue_means.append(fields['mean'])

    result = {
        'file': os.path.basename(path),
        'mode': mode if mode is not None else 'UNKNOWN',
        'travel_mean': mean(travel_means) if travel_means else float('nan'),
        'junction_processing_mean': mean(junc_proc_means) if junc_proc_means else float('nan'),
        'queue_length_mean': mean(queue_means) if queue_means else float('nan'),
        'header': '\n'.join(header_text[:40])
    }
    return result


def collect_results(results_dir, name_contains=None, name_prefix=None):
    files = glob.glob(os.path.join(results_dir, '*.sca'))
    if name_prefix:
        files = [f for f in files if os.path.basename(f).startswith(name_prefix)]
    elif name_contains:
        files = [f for f in files if name_contains in os.path.basename(f)]
    data = []
    for f in files:
        try:
            data.append(parse_sca(f))
        except Exception as e:
            print(f'Warning: failed to parse {f}: {e}')
    df = pd.DataFrame(data)
    return df


def plot_metrics(df, outdir):
    os.makedirs(outdir, exist_ok=True)
    metrics = [
        ('travel_mean', 'Travel Time (mean)'),
        ('junction_processing_mean', 'Junction Processing Time (mean)'),
        ('queue_length_mean', 'Queue Length (mean)')
    ]

    # create short labels: take everything after the last underscore
    df['mode_label'] = df['mode'].apply(lambda m: m.split('_')[-1] if isinstance(m, str) and '_' in m else (m if isinstance(m, str) else 'UNKNOWN'))
    modes = sorted(df['mode_label'].dropna().unique())

    # boxplots for each metric using matplotlib directly
    for col, label in metrics:
        plt.figure(figsize=(6, 4))
        data_to_plot = [df[df['mode_label'] == m][col].dropna().values for m in modes]
        plt.boxplot(data_to_plot, labels=modes, patch_artist=True)
        # overlay individual points jittered
        for i, m in enumerate(modes, start=1):
            y = df[df['mode_label'] == m][col].dropna().values
            x = (np.random.rand(len(y)) - 0.5) * 0.15 + i
            plt.scatter(x, y, color='k', s=6, alpha=0.6)
        plt.title(label)
        plt.tight_layout()
        outpath = os.path.join(outdir, f'{col}.png')
        plt.savefig(outpath, dpi=200)
        plt.close()

    # combined figure with 3 subplots
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    for ax, (col, label) in zip(axes, metrics):
        data_to_plot = [df[df['mode_label'] == m][col].dropna().values for m in modes]
        ax.boxplot(data_to_plot, labels=modes, patch_artist=True)
        for i, m in enumerate(modes, start=1):
            y = df[df['mode_label'] == m][col].dropna().values
            x = (np.random.rand(len(y)) - 0.5) * 0.15 + i
            ax.scatter(x, y, color='k', s=6, alpha=0.6)
        ax.set_title(label)
    plt.tight_layout()
    plt.savefig(os.path.join(outdir, 'combined_metrics.png'), dpi=200)
    plt.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results', '-r', default='../nwsim_project_results', help='results folder')
    parser.add_argument('--out', '-o', default='plots', help='output folder for plots')
    parser.add_argument('--filter-text', '-f', default=None, help='only include runs whose header contains this text')
    parser.add_argument('--name-contains', '-n', default=None, help='only include files whose filename contains this substring')
    parser.add_argument('--name-prefix', '-p', default=None, help='only include files whose filename starts with this prefix')
    args = parser.parse_args()

    results_dir = os.path.abspath(args.results)
    outdir = os.path.abspath(args.out)

    print('Scanning', results_dir)
    df = collect_results(results_dir, name_contains=args.name_contains, name_prefix=args.name_prefix)
    if args.filter_text:
        mask = df['header'].str.contains(args.filter_text, na=False)
        df = df[mask].copy()
    if df.empty:
        print('No .sca files found or parsing failed.')
        return

    # save summary csv
    os.makedirs(outdir, exist_ok=True)
    df.to_csv(os.path.join(outdir, 'summary.csv'), index=False)
    print('Collected', len(df), 'runs. Saving plots to', outdir)
    plot_metrics(df, outdir)
    print('Done.')


if __name__ == '__main__':
    main()

import os
import glob
import re
import argparse
import csv
from collections import defaultdict


def parse_sca_for_max(path):
    configname = None
    stats = {}
    current = None
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith('attr configname'):
                parts = line.split(None, 2)
                if len(parts) >= 3:
                    configname = parts[2]
            if line.startswith('statistic'):
                m = re.match(r'statistic\s+(\S+)\s+(\S+):stats', line)
                if m:
                    module = m.group(1)
                    name = m.group(2)
                    current = (module, name)
                    stats[current] = {}
                else:
                    current = None
            elif line.startswith('field') and current is not None:
                parts = line.split()
                if len(parts) >= 3:
                    field_name = parts[1]
                    try:
                        field_val = float(parts[2])
                    except ValueError:
                        continue
                    stats[current][field_name] = field_val

    # compute run-level maxima
    max_queue = None
    max_junction = None
    for (module, name), fields in stats.items():
        if name == 'queueLength' and module.startswith('CrossTraffic.Junc'):
            if 'max' in fields:
                v = fields['max']
                if max_queue is None or v > max_queue:
                    max_queue = v
        if name == 'junctionProcessingTime' and module.startswith('CrossTraffic.Junc'):
            if 'max' in fields:
                v = fields['max']
                if max_junction is None or v > max_junction:
                    max_junction = v

    return {
        'file': os.path.basename(path),
        'configname': configname or 'UNKNOWN',
        'max_queue_length': max_queue if max_queue is not None else '',
        'max_junction_time': max_junction if max_junction is not None else ''
    }


def collect(results_dir, name_prefix=None):
    files = glob.glob(os.path.join(results_dir, '*.sca'))
    if name_prefix:
        files = [f for f in files if os.path.basename(f).startswith(name_prefix)]
    rows = []
    for f in sorted(files):
        try:
            rows.append(parse_sca_for_max(f))
        except Exception as e:
            print('Warning parsing', f, e)
    return rows


def aggregate_per_config(rows):
    agg = defaultdict(lambda: {'max_queue_length': None, 'max_junction_time': None, 'count': 0})
    for r in rows:
        cfg = r['configname']
        agg[cfg]['count'] += 1
        mq = r['max_queue_length']
        mj = r['max_junction_time']
        if mq != '':
            mq = float(mq)
            if agg[cfg]['max_queue_length'] is None or mq > agg[cfg]['max_queue_length']:
                agg[cfg]['max_queue_length'] = mq
        if mj != '':
            mj = float(mj)
            if agg[cfg]['max_junction_time'] is None or mj > agg[cfg]['max_junction_time']:
                agg[cfg]['max_junction_time'] = mj
    # to list
    out = []
    for cfg, v in agg.items():
        out.append({'configname': cfg, 'runs': v['count'], 'max_queue_length': v['max_queue_length'] or '', 'max_junction_time': v['max_junction_time'] or ''})
    return out


def write_csv(rows, path, fieldnames):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for r in rows:
            writer.writerow(r)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--results', '-r', default='../nwsim_project_results/results', help='results folder')
    parser.add_argument('--out', '-o', default='max_metrics', help='output folder')
    parser.add_argument('--name-prefix', '-p', default=None, help='optional filename prefix filter')
    args = parser.parse_args()

    results_dir = os.path.abspath(args.results)
    outdir = os.path.abspath(args.out)

    rows = collect(results_dir, name_prefix=args.name_prefix)
    if not rows:
        print('No .sca files found.')
        return

    write_csv(rows, os.path.join(outdir, 'per_run_max.csv'), ['file', 'configname', 'max_queue_length', 'max_junction_time'])

    agg = aggregate_per_config(rows)
    write_csv(agg, os.path.join(outdir, 'per_config_max.csv'), ['configname', 'runs', 'max_queue_length', 'max_junction_time'])

    print('Wrote', os.path.join(outdir, 'per_run_max.csv'), 'and', os.path.join(outdir, 'per_config_max.csv'))


if __name__ == '__main__':
    main()

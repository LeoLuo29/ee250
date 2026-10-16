"""Plot the iperf results logged by iperf_logger.sh.

For every distance with both iperf_tcp_<d>m.csv and iperf_udp_<d>m.csv in this
folder, writes iperf_<d>m.png: Run 1-5 on the x-axis, TCP and UDP receiver
throughput as two lines.

Also writes iperf_summary.png (average throughput and packet loss vs. distance)
and one graph per data set: iperf_tcp_vs_distance.png, iperf_udp_vs_distance.png,
iperf_loss_vs_distance.png.

Put the CSV files in the same folder as this script; the PNGs are written there too.
Usage:
    python iperf_plot.py
"""
import glob
import os
import re

import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots

RUNS = ['Run1', 'Run2', 'Run3', 'Run4', 'Run5']


def find_distances():
    """Return the distances (as strings, e.g. '2' or '2.5') that have both CSV files, sorted."""
    distances = []
    for path in glob.glob("iperf_tcp_*m.csv"):
        d = re.match(r"iperf_tcp_(.+)m\.csv", path).group(1)
        if glob.glob(f"iperf_udp_{d}m.csv"):
            distances.append(d)
    return sorted(distances, key=float)


def load_row(path):
    """Read one logger CSV. If the logger was run more than once at this
    distance, the file has several rows; use the most recent (last) one."""
    return pd.read_csv(path).iloc[-1]


def plot_distance(d):
    tcp = load_row(f"iperf_tcp_{d}m.csv")
    udp = load_row(f"iperf_udp_{d}m.csv")

    # Run 1-5 on the x-axis, TCP and UDP throughput as two lines (as the lab handout asks)
    tcp_mbps = [tcp[r] for r in RUNS]
    udp_mbps = [udp[r] for r in RUNS]

    fig = go.Figure()
    fig.add_trace(go.Scatter(x=RUNS, y=tcp_mbps, name='TCP Throughput (Mbps)',
                             mode='lines+markers'))
    fig.add_trace(go.Scatter(x=RUNS, y=udp_mbps, name='UDP Throughput (Mbps)',
                             mode='lines+markers', line=dict(dash='dash')))

    fig.update_layout(title=f"TCP & UDP Throughput at {d}m Distance",
                      xaxis_title='Test Runs', yaxis_title='Throughput (Mbps)')
    fig.update_yaxes(rangemode='tozero')

    out = f"iperf_{d}m.png"
    fig.write_image(out)
    print(f"wrote {out}")
    return float(d), tcp['Avg'], udp['Avg'], udp['Avg_PacketLoss']


def plot_summary(rows):
    df = pd.DataFrame(rows, columns=['distance', 'tcp_avg', 'udp_avg', 'udp_loss'])

    fig = make_subplots(specs=[[{"secondary_y": True}]])
    fig.add_trace(go.Scatter(x=df['distance'], y=df['tcp_avg'], name='TCP throughput',
                             mode='lines+markers'), secondary_y=False)
    fig.add_trace(go.Scatter(x=df['distance'], y=df['udp_avg'], name='UDP throughput',
                             mode='lines+markers'), secondary_y=False)
    fig.add_trace(go.Scatter(x=df['distance'], y=df['udp_loss'], name='UDP packet loss',
                             mode='lines+markers', line=dict(dash='dash')), secondary_y=True)

    fig.update_layout(title='Average iperf throughput vs. distance', xaxis_title='Distance (m)')
    fig.update_yaxes(title_text='Receiver throughput (Mbit/s)', rangemode='tozero', secondary_y=False)
    fig.update_yaxes(title_text='UDP packet loss (%)', rangemode='tozero', secondary_y=True)

    fig.write_image("iperf_summary.png")
    print("wrote iperf_summary.png")


def plot_separate(rows):
    """One graph per data set, in the same style (and colours) as iperf_summary.png."""
    df = pd.DataFrame(rows, columns=['distance', 'tcp_avg', 'udp_avg', 'udp_loss'])
    colors = px.colors.qualitative.Plotly  # plotly's default trace colours, as in the summary

    datasets = [
        # (column, legend name, title, y-axis title, line style, output file)
        ('tcp_avg', 'TCP throughput', 'Average TCP throughput vs. distance',
         'Receiver throughput (Mbit/s)', dict(color=colors[0]), 'iperf_tcp_vs_distance.png'),
        ('udp_avg', 'UDP throughput', 'Average UDP throughput vs. distance',
         'Receiver throughput (Mbit/s)', dict(color=colors[1]), 'iperf_udp_vs_distance.png'),
        ('udp_loss', 'UDP packet loss', 'Average UDP packet loss vs. distance',
         'UDP packet loss (%)', dict(color=colors[2], dash='dash'), 'iperf_loss_vs_distance.png'),
    ]
    for col, name, title, y_title, line, out in datasets:
        fig = go.Figure()
        fig.add_trace(go.Scatter(x=df['distance'], y=df[col], name=name,
                                 mode='lines+markers', line=line, showlegend=True))
        fig.update_layout(title=title, xaxis_title='Distance (m)', yaxis_title=y_title)
        fig.update_yaxes(rangemode='tozero')

        fig.write_image(out)
        print(f"wrote {out}")


def main():
    # work in this script's folder, so it finds the CSVs no matter where it is run from
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    distances = find_distances()
    if not distances:
        print(f"No iperf_tcp_<d>m.csv / iperf_udp_<d>m.csv pairs found in {os.getcwd()}")
        return

    rows = [plot_distance(d) for d in distances]
    plot_summary(rows)
    plot_separate(rows)


if __name__ == "__main__":
    main()

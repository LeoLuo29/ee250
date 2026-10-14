#!/bin/bash
# EE 250L Lab 05 — Fall 2026
#
# iperf_logger.sh — run this on the Ubuntu VM (the iperf SERVER, fixed near the
# router). Last year this script ran on the Raspberry Pi *client*; the ESP32-S3
# cannot write CSV files, so the logging moved to the server side. The ESP32-S3
# is the client: when this script says it is ready, type `measure` in the
# board's serial console and it sends 5 TCP runs and then 5 UDP runs.
#
# Output (identical format to last year):
#   iperf_tcp_<distance>m.csv : Distance,Run1..Run5,Avg                (Mbps, receiver side)
#   iperf_udp_<distance>m.csv : Distance,Run1..Run5,Avg,PacketLoss1..5,Avg_PacketLoss
#
# Requires iperf **2** (`sudo apt install iperf`). iperf3 speaks a different
# protocol and will NOT talk to the ESP32-S3.

if [ -z "$1" ]; then
    echo "Usage: ./iperf_logger.sh <distance_in_meters>"
    exit 1
fi

DISTANCE=$1
PORT=5001
RUNS=5
TIMEOUT_S=900          # give up after 15 minutes without all runs arriving

TCP_LOGFILE="iperf_tcp_${DISTANCE}m.csv"
UDP_LOGFILE="iperf_udp_${DISTANCE}m.csv"

if ! command -v iperf >/dev/null 2>&1; then
    echo "iperf (version 2) is not installed. Run: sudo apt install iperf"
    exit 1
fi
if iperf -v 2>&1 | grep -q "iperf 3"; then
    echo "This is iperf3. Lab 5 needs iperf 2: sudo apt install iperf"
    exit 1
fi

# Write CSV headers (if files don't exist)
if [ ! -f "$TCP_LOGFILE" ]; then
    echo "Distance,Run1,Run2,Run3,Run4,Run5,Avg" > "$TCP_LOGFILE"
fi
if [ ! -f "$UDP_LOGFILE" ]; then
    echo "Distance,Run1,Run2,Run3,Run4,Run5,Avg,PacketLoss1,PacketLoss2,PacketLoss3,PacketLoss4,PacketLoss5,Avg_PacketLoss" > "$UDP_LOGFILE"
fi

TCP_RAW=$(mktemp)
UDP_RAW=$(mktemp)

# -y C makes iperf2 print one machine-readable CSV line per finished client run:
#   TCP: timestamp,server_ip,server_port,client_ip,client_port,id,interval,bytes,bits_per_sec
#   UDP: ...same 9 fields...,jitter_ms,lost,total,loss_percent,out_of_order
# Both servers listen on the same port number; TCP 5001 and UDP 5001 are
# different sockets, so this is allowed.
iperf -s -p "$PORT" -y C    > "$TCP_RAW" 2>/dev/null &
TCP_PID=$!
iperf -s -u -p "$PORT" -y C > "$UDP_RAW" 2>/dev/null &
UDP_PID=$!

cleanup() { kill "$TCP_PID" "$UDP_PID" 2>/dev/null; rm -f "$TCP_RAW" "$UDP_RAW"; }
trap cleanup EXIT

sleep 1
# If an iperf server you started by hand is still running, ours could not bind
# port 5001 and has already exited. Refuse to continue rather than wait forever.
if ! kill -0 "$TCP_PID" 2>/dev/null || ! kill -0 "$UDP_PID" 2>/dev/null; then
    echo "ERROR: could not start the iperf servers on port $PORT."
    echo "       Another 'iperf -s' is probably still running in a different terminal."
    echo "       Stop it with Ctrl-C (or: pkill iperf) and run this script again."
    exit 1
fi
# Linux prints all addresses with `hostname -I`; macOS has no -I, so fall
# back to the Wi-Fi interface's address there.
VM_IP=$(hostname -I 2>/dev/null | awk '{print $1}')
[ -z "$VM_IP" ] && VM_IP=$(ipconfig getifaddr en0 2>/dev/null)

echo "============================================="
echo " Logging iPerf tests at $DISTANCE meters"
echo " iperf2 TCP + UDP servers listening on $VM_IP:$PORT"
echo " Now type   measure   in the ESP32-S3 serial console."
echo " Waiting for $RUNS TCP runs, then $RUNS UDP runs..."
echo "============================================="

TCP_RESULTS=()
UDP_RESULTS=()
PACKET_LOSS=()
tcp_seen=0
udp_seen=0
start=$(date +%s)

while [ $tcp_seen -lt $RUNS ] || [ $udp_seen -lt $RUNS ]; do
    # Only count lines from a real client (id >= 1); iperf2 prints id -1 for sums.
    tcp_lines=$(grep -c "" "$TCP_RAW")
    if [ "$tcp_lines" -gt "$tcp_seen" ] && [ $tcp_seen -lt $RUNS ]; then
        line=$(sed -n "$((tcp_seen + 1))p" "$TCP_RAW")
        bps=$(echo "$line" | awk -F, '{print $9}')
        mbps=$(awk -v b="$bps" 'BEGIN{printf "%.3f", b/1000000}')
        tcp_seen=$((tcp_seen + 1))
        TCP_RESULTS+=("$mbps")
        echo "TCP Receiver Throughput (Run $tcp_seen): $mbps Mbps"
    fi

    udp_lines=$(grep -c "" "$UDP_RAW")
    if [ "$udp_lines" -gt "$udp_seen" ] && [ $udp_seen -lt $RUNS ]; then
        line=$(sed -n "$((udp_seen + 1))p" "$UDP_RAW")
        bps=$(echo "$line"  | awk -F, '{print $9}')
        loss=$(echo "$line" | awk -F, '{print $13}')
        mbps=$(awk -v b="$bps" 'BEGIN{printf "%.3f", b/1000000}')
        loss=$(awk -v l="$loss" 'BEGIN{printf "%.3f", l+0}')
        udp_seen=$((udp_seen + 1))
        UDP_RESULTS+=("$mbps")
        PACKET_LOSS+=("$loss")
        echo "UDP Receiver Throughput (Run $udp_seen): $mbps Mbps   Packet Loss: $loss %"
    fi

    if [ $(( $(date +%s) - start )) -gt $TIMEOUT_S ]; then
        echo "Timed out waiting for runs (got $tcp_seen TCP, $udp_seen UDP). Nothing logged."
        exit 1
    fi
    sleep 0.5
done

TCP_AVG=$(echo "${TCP_RESULTS[@]}" | awk '{sum=0; for (i=1; i<=NF; i++) sum+=$i; printf "%.3f", sum/NF}')
UDP_AVG=$(echo "${UDP_RESULTS[@]}" | awk '{sum=0; for (i=1; i<=NF; i++) sum+=$i; printf "%.3f", sum/NF}')
PKT_LOSS_AVG=$(echo "${PACKET_LOSS[@]}" | awk '{sum=0; for (i=1; i<=NF; i++) sum+=$i; printf "%.3f", sum/NF}')

tcp_csv=$(IFS=','; echo "${TCP_RESULTS[*]}")
udp_csv=$(IFS=','; echo "${UDP_RESULTS[*]}")
loss_csv=$(IFS=','; echo "${PACKET_LOSS[*]}")
echo "$DISTANCE,$tcp_csv,$TCP_AVG" >> "$TCP_LOGFILE"
echo "$DISTANCE,$udp_csv,$UDP_AVG,$loss_csv,$PKT_LOSS_AVG" >> "$UDP_LOGFILE"

echo "---------------------------------------------"
echo "TCP Results (Mbps): ${TCP_RESULTS[@]}   Average: $TCP_AVG Mbps"
echo "UDP Results (Mbps): ${UDP_RESULTS[@]}   Average: $UDP_AVG Mbps"
echo "Packet Loss (%):    ${PACKET_LOSS[@]}   Average: $PKT_LOSS_AVG %"
echo "Logging completed. Data saved in $TCP_LOGFILE and $UDP_LOGFILE."
echo "---------------------------------------------"

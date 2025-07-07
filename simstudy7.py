import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt
import pandas as pd

def parse_flow_monitor(xml_file):
    tree = ET.parse(xml_file)
    root = tree.getroot()
    flows = root.find("FlowStats")
    flow_data = []
    for flow in flows.findall("Flow"):
        flow_id = int(flow.get("flowId"))
        tx_packets = int(flow.get("txPackets"))
        rx_packets = int(flow.get("rxPackets"))
        lost_packets = int(flow.get("lostPackets"))
        delay_sum_ns = float(flow.get("delaySum").replace("+", "").replace("ns", ""))
        jitter_sum_ns = float(flow.get("jitterSum").replace("+", "").replace("ns", ""))
        flow_data.append({
            "flowId": flow_id,
            "txPackets": tx_packets,
            "rxPackets": rx_packets,
            "lostPackets": lost_packets,
            "delaySum_ns": delay_sum_ns,
            "jitterSum_ns": jitter_sum_ns
        })
    return flow_data

def parse_rtt_data(tsv_file):
    rtt_data = pd.read_csv(tsv_file, sep="\t", header=None, names=["Time", "Bytes", "RTT_ms"])
    return rtt_data

def compute_metrics(flow_data):
    total_flows = len(flow_data)
    total_dropped_packets = sum(flow["lostPackets"] for flow in flow_data)
    total_received_packets = sum(flow["rxPackets"] for flow in flow_data)
    avg_latency_ns = sum(flow["delaySum_ns"] for flow in flow_data) / total_received_packets
    avg_jitter_ns = sum(flow["jitterSum_ns"] for flow in flow_data) / total_received_packets
    return total_flows, total_dropped_packets, total_received_packets, avg_latency_ns, avg_jitter_ns

def plot_dropped_vs_received(flow_data):
    dropped = [flow["lostPackets"] for flow in flow_data]
    received = [flow["rxPackets"] for flow in flow_data]
    plt.figure()
    plt.scatter(received, dropped, color="red")
    plt.xlabel("Received Packets")
    plt.ylabel("Dropped Packets")
    plt.title("Dropped Packets vs. Received Packets")
    plt.grid()
    plt.show()

def plot_rtt_over_time(rtt_data):
    plt.figure()
    plt.plot(rtt_data["Time"], rtt_data["RTT_ms"], color="blue")
    plt.xlabel("Time (s)")
    plt.ylabel("RTT (ms)")
    plt.title("RTT Over Time")
    plt.grid()
    plt.show()


def task_7_3():
    xml_file = "output/dgr/FLOW-MONITOR-OUTPUT.XML"
    tsv_file = "output/dgr/SRCDST.RTT"

    flow_data = parse_flow_monitor(xml_file)
    rtt_data = parse_rtt_data(tsv_file)

    total_flows, total_dropped_packets, total_received_packets, avg_latency_ns, avg_jitter_ns = compute_metrics(flow_data)

    print(f"Total Flows: {total_flows}")
    print(f"Total Dropped Packets: {total_dropped_packets}")
    print(f"Total Received Packets: {total_received_packets}")
    print(f"Average Latency (ms): {avg_latency_ns / 1e6:.3f}")
    print(f"Average Jitter (ms): {avg_jitter_ns / 1e6:.3f}")

    plot_dropped_vs_received(flow_data)
    plot_rtt_over_time(rtt_data)

if __name__ == "__main__":
    task_7_1_5()
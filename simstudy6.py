import os
import pandas as pd
import matplotlib.pyplot as plt

output_dir = "output"

tcp_variants = ["TcpNewReno", "TcpVegas"]
error_rates = ["1e-05", "1e-06"]
delays = ["10ms", "20ms"]

def read_cwnd(filename):
    df = pd.read_csv(filename, sep="\t", names=["time", "old_cwnd", "new_cwnd"])
    return df

def read_rxdrop(filename):
    if not os.path.isfile(filename):
        return pd.DataFrame(columns=["time"])
    df = pd.read_csv(filename, names=["time"])
    return df

def task_6_1_4():
    plt.figure(figsize=(10, 6))
    for tcp in tcp_variants:
        for ber in error_rates:
            cwnd_file = f"{output_dir}/single_tcp_task_614_{tcp}_ber_{ber}_delay_10ms.cwnd"
            rxdrop_file = f"{output_dir}/single_tcp_task_614_{tcp}_ber_{ber}_delay_10ms.rxdrop"
            if not os.path.isfile(cwnd_file):
                continue
            df_cwnd = read_cwnd(cwnd_file)
            df_rxdrop = read_rxdrop(rxdrop_file)
            label = f"{tcp} (Error Rate={ber})"
            plt.plot(df_cwnd["time"], df_cwnd["new_cwnd"], label=label)

            for drop_time in df_rxdrop["time"]:

                idx = df_cwnd[df_cwnd["time"] <= drop_time].index.max()
                if pd.isna(idx):
                    continue
                plt.scatter(df_cwnd.loc[idx, "time"], df_cwnd.loc[idx, "new_cwnd"], color="red", s=30, zorder=5)

    plt.title("TCP cWnd Evolution (Delay=10ms)")
    plt.xlabel("Time (s)")
    plt.ylabel("Congestion Window Size (bytes)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

    plt.figure(figsize=(10, 6))
    for tcp in tcp_variants:
        for d in delays:
            cwnd_file = f"{output_dir}/single_tcp_task_614_{tcp}_ber_1e-05_delay_{d}.cwnd"
            rxdrop_file = f"{output_dir}/single_tcp_task_614_{tcp}_ber_1e-05_delay_{d}.rxdrop"
            if not os.path.isfile(cwnd_file):
                continue
            df_cwnd = read_cwnd(cwnd_file)
            df_rxdrop = read_rxdrop(rxdrop_file)
            label = f"{tcp} (Delay={d})"
            plt.plot(df_cwnd["time"], df_cwnd["new_cwnd"], label=label)

            for drop_time in df_rxdrop["time"]:
                idx = df_cwnd[df_cwnd["time"] <= drop_time].index.max()
                if pd.isna(idx):
                    continue
                plt.scatter(df_cwnd.loc[idx, "time"], df_cwnd.loc[idx, "new_cwnd"], color="red", s=30, zorder=5)

    plt.title("TCP cWnd Evolution (Error Rate =1e-5)")
    plt.xlabel("Time (s)")
    plt.ylabel("Congestion Window Size (bytes)")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

def task_6_2_3():

    tcp_variants = ["TcpNewReno", "TcpBic"]
    flow_labels = ["Flow 1", "Flow 2"]

    for tcp in tcp_variants:
        plt.figure(figsize=(10, 6))
        for flow in range(1, 3): 
            cwnd_file = f"{output_dir}/multi_tcp_flow{flow}_task_623_{tcp}.cwnd"
            if not os.path.isfile(cwnd_file):
                continue
            df_cwnd = read_cwnd(cwnd_file)
            label = f"{tcp} ({flow_labels[flow - 1]})"
            plt.plot(df_cwnd["time"], df_cwnd["new_cwnd"], label=label)

        plt.title("TCP cWnd Evolution for Multiple Flows (Shared Bottleneck)")
        plt.xlabel("Time (s)")
        plt.ylabel("Congestion Window Size (bytes)")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()

def task_6_2_5():

    tcp_variants = ["TcpNewReno", "TcpBic"]
    flow_labels = ["Flow 1", "Flow 2"]

    for tcp in tcp_variants:
        plt.figure(figsize=(10, 6))
        for flow in range(1, 3): 
            cwnd_file = f"{output_dir}/multi_tcp_flow{flow}_task_625_{tcp}.cwnd"
            if not os.path.isfile(cwnd_file):
                continue
            df_cwnd = read_cwnd(cwnd_file)
            label = f"{tcp} ({flow_labels[flow - 1]})"
            plt.plot(df_cwnd["time"], df_cwnd["new_cwnd"], label=label)

        plt.title("TCP cWnd Evolution for Multiple Flows (Shared Bottleneck)")
        plt.xlabel("Time (s)")
        plt.ylabel("Congestion Window Size (bytes)")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()

        
def task_6_2_6():

    tcp_variants = ["TcpNewReno", "TcpBic"]
    flow_labels = ["Flow 1", "Flow 2"]

    for tcp in tcp_variants:
        plt.figure(figsize=(10, 6))
        for flow in range(1, 3): 
            cwnd_file = f"{output_dir}/multi_tcp_flow{flow}_task_626_{tcp}.cwnd"
            if not os.path.isfile(cwnd_file):
                continue
            df_cwnd = read_cwnd(cwnd_file)
            label = f"{tcp} ({flow_labels[flow - 1]})"
            plt.plot(df_cwnd["time"], df_cwnd["new_cwnd"], label=label)

        plt.title("TCP cWnd Evolution for Multiple Flows (Shared Bottleneck)")
        plt.xlabel("Time (s)")
        plt.ylabel("Congestion Window Size (bytes)")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()


if __name__ == "__main__":
    # task_6_1_4()
    task_6_2_3()
    
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import subprocess

N = [10**3, 10**4, 10**5, 10**6, 10**7]
P = 16
REPEATS = 10

def run_experiments():
    for i in range(3):
        for n in N:
            for p in range(P, 0, -1):
                print(f"======== Run {i} for n = {n} on {p} processors")

                subprocess.call(['./bspsieve', '-n', str(n), '-p', str(p)])

def general(df):
    plt.plot(df['p'], df['duration'])
    plt.title('Speedup Improvement Plot')
    plt.xlabel('# of Processors')
    plt.ylabel('Duration (ms)')
    plt.show()

def processor_load(df):
    plt.bar(df['pid'], df['flops'])
    plt.xticks(df['pid'])
    plt.title('Processor Load Distribution')
    plt.xlabel('Processor ID')
    plt.ylabel('FLOPS')
    plt.show()

# df = pd.read_csv('results_old.csv', names = ['n', 'p', 'duration', 'pid', 'flops'])
# df_duration = df.groupby(['p'])
# df_duration = df_duration['duration'].mean().reset_index()


# for i in range(1, P):
#     df_flops = df[df['p'] == i]
#     df_flops = df_flops.groupby(['pid'])
#     df_flops = df_flops['flops'].mean().reset_index()
#     processor_load(df_flops)

run_experiments()

# general(df_duration)
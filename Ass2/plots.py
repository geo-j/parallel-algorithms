import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import subprocess

N = [10**3, 10**4, 10**5, 10**6, 10**7]
P = 16
REPEATS = 10

def run_experiments():
    for n in N:
        for p in range(1, P):
            for i in range(REPEATS):
                print(f"======== Run {i} for n = {n} on {p} processors")

                subprocess.call(['./BSPSieve', '-n', str(n), '-p', str(p)])

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

df = pd.read_csv('results.csv', names = ['n', 'p', 'duration', 'pid', 'flops'])
# df = df.groupby(['p'])
# print(df['flops'])
for i in range(1, P):
    processor_load(df[df['p'] == i])

# run_experiments()


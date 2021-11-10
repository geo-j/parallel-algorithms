import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import subprocess

N = [10 ** 3, 10 ** 4, 10 ** 5, 10 ** 6, 10 ** 7]
# N = 10 ** 6
P = range(1, 17)
REPEATS = 10
bs = []
# B = range()

def run_experiments():
    for i in range(REPEATS):
        # for b in range(16, 100):
        for n in N:
            for p in P:
                print(f"======== Run {i} for n = {n}, b = {bs[p - 1]} on {p} processors")

                subprocess.call(['./bspsieve', '-n', str(n), '-p', str(p), '-b', str(bs[p - 1])])

def b_time(df1):
    plt.title(f'Speedup Improvement Plot Based on the Choice of $b$ \n $n = {N}, p = {P}$')
    plt.xlabel('$b$')
    plt.ylabel('Duration (ms)')
    plt.xticks(B)
    color = np.where(df1['duration'] == np.min(df1['duration']), 'red', 'blue')
    plt.grid()
    plt.scatter(df1['b'], df1['duration'], color=color)
    plt.show()

def general(df1):
    plt.plot(df1['p'], df1['duration'])
    # plt.plot(df2['p'], df2['duration'])
    plt.title('Speedup Improvement Plot')
    plt.xlabel('# of Processors')
    plt.ylabel('Duration (ms)')
    plt.show()

def processor_load(df):
    plt.bar(df['pid'], df['flops'])
    plt.xticks(df['pid'])
    plt.title(f'Processor Load Distribution p = {7}, b = {30}')
    plt.xlabel('Processor ID')
    plt.ylabel('FLOPS')
    plt.show()

df = pd.read_csv('results_best_b.csv', names = ['n', 'p', 'b', 'duration', 'pid', 'flops', 'test'])
# df = df[df['n'] == 10 ** 6]
# df_b = df_b[df_b['b'] == 101]
df_b = df[df['p'] == 7]
# df_b = df_b.groupby(['b'])
# df_b = df_b['duration'].mean().reset_index()
# df_o = df[df['test'] == 'o']
# print(df_o)
# df_i = df[df['test'] == 'p']

# print(df_b)
# df_duration_o = df_o.groupby(['p'])
# df_duration_o = df_duration_o['duration'].mean().reset_index()
# df_duration_i = df_i.groupby(['p'])
# df_duration_i = df_duration_i['duration'].mean().reset_index()

# for i in range(1, 17):
    # df_b = df[df['p'] == i]
    # print(df_b)
    # df_b = df_b['duration'].mean()
    # bs.append(df_b.iloc[df_b['duration'].argmin()]['b'])
    # df_flops = df_b[df_b['pid'] == i]
# print(bs)
df_flops = df_b.groupby(['pid'])
df_flops = df_flops['flops'].mean().reset_index()
processor_load(df_flops)

# run_experiments()
# print(df_duration_o)
# general(df_duration_o)

# b_time(df_b)
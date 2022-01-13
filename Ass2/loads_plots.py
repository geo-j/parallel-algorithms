import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# df = pd.read_csv('loads_4.csv', names = ['index', 't', 'd', 'N', 'p', 'pid', 'load', 'duration'])
df = pd.read_csv('runtime.csv', names = ['d', 'N', 'p', 'duration', 'pid', 'flops', 'syncs', 'n_paths', 'sync_factor'])
df = df.dropna(axis = 0)
df = df.astype(int)
print(df)

# times = list(map(int, df['t'].unique()))


def all_loads():
    _, ax = plt.subplots()
    for p in range(4):
        # print(p, df[df['pid'] == str(p)])
        load = df[df['pid'] == p]['load']
        if p > 0:
            bottom = df[df['pid'] == 0]['load'].to_numpy()
            for prev_p in range(1, p):
                bottom += df[df['pid'] == prev_p]['load'].to_numpy()
            ax.bar(range(len(load)), load, bottom = bottom)
        else:
            ax.bar(range(len(load)), load)


    plt.legend(['$p_1$', '$p_2$', '$p_3$', '$p_4$'])
    plt.grid()
    plt.xlabel('time steps')
    plt.ylabel('load')
    plt.title('Load Distribution In Time for $d = 2$, $N = 10$, $p = 4$')
    plt.show()

def min_max_load():
    _, ax = plt.subplots()

    # print(len(load0), len(load1), len(load2), len(load3))
    matrix = []
    for p in range(4):
        # print(p, df[df['pid'] == str(p)])
        load = df[df['pid'] == p]['load']
        matrix.append(load)

    matrix = np.array(matrix)
    # print(matrix.min(axis = 0))
    ax.bar(range(len(matrix[0])), matrix.min(axis = 0))
    ax.bar(range(len(matrix[0])), matrix.max(axis = 0), bottom = matrix.min(axis = 0))


    plt.legend(['min load', 'max load'])
    plt.grid()
    plt.xlabel('time steps')
    plt.ylabel('load')
    plt.title('Load Distribution In Time for $d = 2$, $N = 10$, $p = 4$')
    plt.show()

def runtime():
    global df
    df = df[df['d'] == 2]
    df = df[df['pid'] == 0]
    df = df[df['n_paths'] != 2]
    df = df.drop_duplicates()
    # print(df.groupby('p')['N'])
    # df = df[df['sync'] == 0]

    # print(df)
    for p in list(map(int, df['p'].unique())):
    # for p in [1]:
    # print(df['duration'])
        # print(df[df['p'] == p]['N'], df[df['p'] == p]['duration'])
        df_p = df[df['p'] == p]
        df_p['avg_duration'] = df_p.groupby('N')['duration'].transform('mean')
        df_p = df_p[['N', 'avg_duration']]
        df_p = df_p.drop_duplicates()
        # N = list(map(int, df_p['N'].unique()))
        print(df_p)
        # plt.plot(df[df['p'] == p]['N'], df[df['p'] == p]['avg_duration'])
        plt.plot(df_p['N'], df_p['avg_duration'])
    plt.xlabel('path length $n$')
    plt.ylabel('duration (ms)')
    plt.legend(['$p = 1$', '$p = 2$', '$p = 4$', '$p = 8$', '$p = 16$', '$ p = 32$', '$p = 64$', '$p = 128$'])
    plt.grid()
    plt.title('Runtime ')
    plt.show()

runtime()
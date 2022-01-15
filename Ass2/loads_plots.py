import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

df = pd.read_csv('loads.csv', names = ['t', 'd', 'N', 'p', 'pid', 'load', 'duration', 'n_syncs', 'sync_factor'])
# df = pd.read_csv('runtime_syncs.csv', names = ['d', 'N', 'p', 'duration', 'pid', 'flops', 'syncs', 'n_paths', 'sync_factor'])
# print(df['t'])
df['t'] = df['t'].str[13:]
df = df.dropna(axis = 0)
df = df.astype(int)
print(df)

# times = list(map(int, df['t'].unique()))


def all_loads():
    global df
    _, ax = plt.subplots()
    df = df[df['n_syncs'] <= 50]
    df = df[df['sync_factor'] == 1]
    df = df[df['N'] == 15]
    df = df[df['d'] == 2]
    df = df[df['p'] == 8]
    for p in sorted(list(map(int, df['pid'].unique()))):
        # print(p, df[df['pid'] == str(p)])
        load = df[df['pid'] == p]['load']
        if p > 0:
            bottom = df[df['pid'] == 0]['load'].to_numpy()
            for prev_p in range(1, p):
                bottom += df[df['pid'] == prev_p]['load'].to_numpy()
            ax.bar(range(len(load)), load, bottom = bottom)
        else:
            ax.bar(range(len(load)), load)
        # ax.plot(load)


    plt.legend(['$p_1$', '$p_2$', '$p_3$', '$p_4$', '$p_5$', '$p_6$', '$p_7$', '$p_8$'])
    plt.grid()
    plt.xlabel('# syncs')
    plt.ylabel('load')
    plt.ylim(0, 250)
    plt.title('Bad Load Distribution In Time for the First 50 Syncs for $d = 2$, $N = 15$, $p = 8$')
    plt.show()

def min_max_load():
    global df
    _, ax = plt.subplots()
    df = df[df['d'] == 2]
    df = df[df['N'] == 14]
    df = df[df['sync_factor'] == 16]
    # df = df[df['load'] == 0]

    # print(len(load0), len(load1), len(load2), len(load3))
    matrix = []
    for p in range(8):
        # print(p, df[df['pid'] == str(p)])
        load = df[df['pid'] == p]['load'].tolist()
        matrix.append(load)

    # print(matrix)
    matrix = np.array(matrix)
    max_matrix = matrix.max(axis = 0)
    min_matrix = matrix.min(axis = 0)
    matrix_0 = np.where(min_matrix == 0)
    ax.plot(min_matrix[matrix_0])
    ax.plot(max_matrix[matrix_0])
    # print(matrix.min(axis = 0))
    # ax.bar(range(len(matrix[0])), matrix.min(axis = 0))
    # ax.bar(range(len(matrix[0])), matrix.max(axis = 0), bottom = matrix.min(axis = 0))
    # ax.plot(matrix.min(axis = 0))
    # ax.plot(matrix.max(axis = 0))


    plt.legend(['min load', 'max load'])
    plt.grid()
    plt.xlabel('# syncs')
    plt.ylabel('load')
    plt.title('Load Distribution In Time for $d = 2$, $N = 13$, $p = 8$')
    plt.show()

def loads_zero_sync():
    global df
    df = df[df['d'] == 2]
    df = df[df['pid'] == 0]
    # df = df[df['n_paths'] != 2]
    # df = df[df['sync_factor'] == 10]
    # df = df[df['N'] >= 14]
    df = df.drop_duplicates()
    # print(df.groupby('p')['N'])
    # df = df[df['sync'] == 0]

    print(df)
    n_syncs = []
    for s in sorted(list(map(int, df['sync_factor'].unique()))):
    # # for p in [1]:
    # # print(df['duration'])
    #     # print(df[df['p'] == p]['N'], df[df['p'] == p]['duration'])
        df_p = df[df['sync_factor'] == s]
        zeros = df_p[df_p['load'] == 0]['N'].count()
        # avg_duration = df_p['duration'].mean()
        # print(s, zeros)
        n_syncs.append(zeros)
    plt.plot(n_syncs)
        # durations.append(avg_duration)

    plt.xlabel('sync factor')
    plt.ylabel('# loads 0')
    # plt.legend(['$p = 1$', '$p = 2$', '$p = 4$', '$p = 8$', '$p = 16$', '$ p = 32$', '$p = 64$', '$p = 128$', '$p = 256$'])
    print(sorted(list(map(int, df['sync_factor'].unique()))))
    plt.xticks(range(len(n_syncs)), sorted(list(map(int, df['sync_factor'].unique()))))
    plt.grid()
    plt.title('Number of 0 Loads at Sync Time for $p = 8$, $N = 15$')
    plt.show()

def runtime():
    global df
    df = df[df['d'] == 2]
    df = df[df['pid'] == 0]
    df = df[df['n_paths'] != 2]
    # df = df[df['p'] != 0]
    # df = df[df['sync_factor'] == 10]
    df = df[df['N'] >= 20]
    df = df[df['N'] <= 23]

    df = df.drop_duplicates()
    # print(df.groupby('p')['N'])
    # df = df[df['sync'] == 0]

    # print(df)
    for p in sorted(list(map(int, df['p'].unique()))):
        print(p)

    # for p in [1]:
    # print(df['duration'])
        # print(df[df['p'] == p]['N'], df[df['p'] == p]['duration'])
        df_p = df[df['p'] == p]
        if p > 0:
            df_p = df_p[df_p['sync_factor'] == p]
        df_p['avg_duration'] = df_p.groupby('N')['duration'].transform('mean')
        df_p = df_p[['d', 'N', 'p', 'sync_factor', 'avg_duration']]
        df_p = df_p.drop_duplicates()
        # N = list(map(int, df_p['N'].unique()))
        # print(df_p)
        # plt.plot(df[df['p'] == p]['N'], df[df['p'] == p]['avg_duration'])
        plt.semilogy(df_p['N'], df_p['avg_duration'], base = 2)
        # df_p.to_csv('runtime_p.csv', mode = 'a', index = False, header = False)
        # plt.plot(df_p['N'], df_p['avg_duration'])
    # print(list(map(int, np.logspace(5, 18, 15, base = 2, dtype = int))))
    # plt.yticks(np.logspace(5, 18, 12, base = 2, dtype = int))
    # plt.ylim(2 ** 5, 2 ** 21)
    plt.xlabel('path length $N$')
    plt.ylabel('duration (ms)')
    # plt.legend(['seq', '$p = 1$', '$p = 2$', '$p = 4$', '$p = 8$', '$p = 16$', '$ p = 32$', '$p = 64$', '$p = 128$', '$p = 256$'])
    plt.legend(['$p = 64$', '$p = 128$', '$p = 256$'])
    plt.grid()
    plt.title('Runtime Plot for a Sync Factor of $p$')
    plt.show()

def speed_runtime():
    global df
    df = df[df['d'] == 2]
    df = df[df['pid'] == 0]
    df = df[df['n_paths'] != 2]
    df = df[df['N'] == 15]
    # df = df[df['sync_factor'] == 10]
    df = df.drop_duplicates()
    # print(df[df['p'] == 256])
    # print(df.groupby('p')['N'])
    # df = df[df['sync'] == 0]

    # print(df)
    for p in [8, 256]:
        df_p = df[df['p'] == p]
        print(df_p['sync_factor'])
        durations = []
        for s in sorted(list(map(int, df['sync_factor'].unique()))):
        # # for p in [1]:
        # # print(df['duration'])
        #     # print(df[df['p'] == p]['N'], df[df['p'] == p]['duration'])
            df_s = df_p[df_p['sync_factor'] == s]
            avg_duration = df_s['duration'].mean()
            print(s)
            durations.append(avg_duration)
            # print(durations)
        #     df_p = df_p[['N', 'avg_duration']]
        #     df_p = df_p.drop_duplicates()
        #     # N = list(map(int, df_p['N'].unique()))
        #     print(df_p)
            # plt.plot(df[df['p'] == p]['N'], df[df['p'] == p]['avg_duration'])
        for i in range(len(durations)):
            if pd.isna(durations[i]) and i > 0:
                durations[i] = durations[i - 1]
        plt.plot(durations)
        # plt.show()
    plt.xlabel('sync factor')
    plt.ylabel('duration (ms)')
    plt.xticks(range(len(sorted(list(map(int, df['sync_factor'].unique()))))), sorted(list(map(int, df['sync_factor'].unique()))))
        # plt.show()
    # plt.legend(['$p = 1$', '$p = 2$', '$p = 4$', '$p = 8$', '$p = 16$', '$ p = 32$', '$p = 64$', '$p = 128$', '$p = 256$'])
    plt.legend(['$p = 8$', '$p = 256$'])
    plt.grid()
    plt.title('Runtime for $N = 15$ and $p = 256$ Based on the Sync Factor')
    plt.show()
# speed_runtime()
# min_max_load()
# runtime()
loads_zero_sync()
# all_loads()
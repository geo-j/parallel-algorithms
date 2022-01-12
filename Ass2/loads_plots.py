import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

df = pd.read_csv('loads_4.csv', names = ['index', 't', 'd', 'N', 'p', 'pid', 'load', 'duration'])
df = df.dropna(axis = 0)
df = df.astype()
print(df)
# print(df[df['pid'] == 0]
# df['t'] = df['t'].str[13:]
# df = df[df['d'] == 2]
# df = df[df['N'] == 10]
# print(df)
# df.to_csv('loads_4.csv')
# df.plot(x = 'load', kind = 'bar', stacked = True)
# plt.show()
times = list(map(int, df['t'].unique()))
# for i in range(len(times)):
#     times[i] = times[i][13:]
# print(times)
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


#     print(f"p = {p}: {df[df['p'] == p]['load']}")
# color = ['cornflowerblue' if t % 2 == 0 else 'b' for t in times]
# load = df[df['p'] == 0]['load']
# ax.bar(times, load, color = color)
# color = ['moccasin' if t % 2 == 0 else 'orange' for t in times]
# load = df[df['p'] == 1]['load']
# ax.bar(times, load, bottom = df[df['p'] == 0]['load'], color = color)
# color = ['palegreen' if t % 2 == 0 else 'g' for t in times]
# load = df[df['p'] == 2]['load']
# ax.bar(times, load, bottom = (df[df['p'] == 1]['load'].to_numpy() + df[df['p'] == 0]['load'].to_numpy()), color = color)
# color = ['lightcoral' if t % 2 == 0 else 'r' for t in times]
# load = df[df['p'] == 3]['load']
# ax.bar(times, load, bottom = (df[df['p'] == 1]['load'].to_numpy() + df[df['p'] == 0]['load'].to_numpy() + df[df['p'] == 2]['load'].to_numpy()), color = color)
# p1a = mpatches.Patch(color='cornflowerblue', label='$p_1$ before redistribution')
# p1b = mpatches.Patch(color='b', label='$p_1$ after redistribution')
# p2a = mpatches.Patch(color='moccasin', label='$p_2$ before redistribution')
# p2b = mpatches.Patch(color='orange', label='$p_3$ after redistribution')
# p3a = mpatches.Patch(color='palegreen', label='$p_3$ before redistribution')
# p3b = mpatches.Patch(color='g', label='$p_3$ after redistribution')
# p4a = mpatches.Patch(color='lightcoral', label='$p_4$ before redistribution')
# p4b = mpatches.Patch(color='r', label='$p_4$ after redistribution')
# plt.legend(handles=[p1a, p1b, p2a, p2b, p3a, p3b, p4a, p4b])
# plt.legend(['$p_1$', '$p_2$', '$p_3$', '$p_4$', '$p_5$', '$p_6$', '$p_7$', '$p_8$'])
plt.legend(['$p_1$', '$p_2$', '$p_3$', '$p_4$'])
plt.grid()
plt.xlabel('time steps')
plt.ylabel('load')
plt.title('Load Distribution In Time for $d = 2$, $N = 10$, $p = 4$')
plt.show()
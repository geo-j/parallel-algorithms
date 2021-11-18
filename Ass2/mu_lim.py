import matplotlib.pyplot as plt

c_n = [1, 4, 12, 36, 100, 284, 780, 2172, 5916, 16268, 44100, 120292, 324932, 881500, 2374444, 6416596, 17245332, 46466676, 124658732, 335116620, 897697164, 2408806028, 6444560484, 17266613812, 46146397316, 123481354908, 329712786220, 881317491628]
mu_n = [pow(c_n[n], 1 / (n + 1)) for n in range(len(c_n))]
print(len(c_n))

plt.plot(mu_n)
plt.xlabel('$n$')
plt.ylabel('$\mu_n$')
plt.title('Plot of $\mu_n$ for SAW Length $n \in [1, 28]$')
plt.show()
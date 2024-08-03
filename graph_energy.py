from matplotlib import pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np

figure, ax = plt.subplots()
m = 0
lastm = 0
def animate(i):
    global m, lastm
    qwe = np.loadtxt("energy.txt", skiprows=1).T
    if (len(qwe) == 0):
        return
    m = qwe.shape[1]
    if m==lastm:
        return
        lastm = m
    n = qwe.shape[0] - 1
    figure.clf()
    colors = "bgrcmyk"
    for i in range(n):
        plt.plot(qwe[0], qwe[i+1], c=colors[i])
    lastm = m

ani = FuncAnimation(figure, animate, interval=2000)
plt.show()

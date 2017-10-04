import numpy as np
import struct
import time
import sys, os
def Akm(m,n):  
    a = np.zeros((m+1, n+1))
    #print(a.shape)
    for j in range(n):
        a[0][j]=j+1
    for i in range(1, m):
        a[i][0]=a[i-1][1] 
        for j in range(1, n):  
            a[i][j]=a[i-1][a[i][j-1]]  
    return a[m][n]

totaltime = 0
numloop = int(sys.argv[1])
m = int(sys.argv[2])
n = int(sys.argv[3])
for i in range(numloop):
    startsec = time.time()
    Akm(m, n)
    #print(arr)
    endsec = time.time()
    totaltime += endsec - startsec
print(totaltime)
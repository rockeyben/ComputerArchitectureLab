import numpy as np
import struct
import time

p = open('numPool.txt','rb')
arr = []
for i in range(10000000):
    t, = struct.unpack('i',p.read(4))
    arr.append(t)
#print(arr)

pool = np.array(arr)
Mat1 = pool[0:1000000].reshape(1000, 1000)
Mat2 = pool[1000:1001000].reshape(1000, 1000)


totaltime = 0
for i in range(10):
    startsec = time.time()
    np.dot(Mat1, Mat2)
    #print(arr)
    endsec = time.time()
    totaltime += endsec - startsec
print(totaltime)

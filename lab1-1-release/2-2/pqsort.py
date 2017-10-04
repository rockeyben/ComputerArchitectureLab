import struct
import time
import copy

def QuickSort(arr,firstIndex,lastIndex):
    if firstIndex<lastIndex:
        divIndex=Partition(arr,firstIndex,lastIndex)
 
        QuickSort(arr,firstIndex,divIndex)       
        QuickSort(arr,divIndex+1,lastIndex)
    else:
        return
 
 
def Partition(arr,firstIndex,lastIndex):
    i=firstIndex-1
    for j in range(firstIndex,lastIndex):
        if arr[j]<=arr[lastIndex]:
            i=i+1
            arr[i],arr[j]=arr[j],arr[i]
    arr[i+1],arr[lastIndex]=arr[lastIndex],arr[i+1]
    return i

p = open('array.txt','rb')
arr = []
for i in range(100000):
    t, = struct.unpack('i',p.read(4))
    arr.append(t)
#print(arr)

totaltime = 0
for i in range(100):
    arr_sort = copy.deepcopy(arr)
    startsec = time.time()
    QuickSort(arr_sort, 0, 100000-1)
    #print(arr)
    endsec = time.time()
    totaltime += endsec - startsec
print(totaltime)
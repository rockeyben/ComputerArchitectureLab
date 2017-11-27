
## compile

```
make clean
make
```

## test trace file

${cache_size}: how many 32KBs
${block_size}: how many Bytes
${associativity}: associativity
${write through}: 1|0 for write through|back
${write allocate}: 1|0 for writ allocate|non-allocate

```
./sim -t [file name] ${cache_size} ${block_size} ${associativity} ${write through} ${write allocate}
```

for example: if you want to test 1.trace at cache size 64KB, block size 64Bytes, associativity 4,
write through and non-allocate, you just type in cmd:

```
./sim -t 1.trace 2 64 4 1 0
```





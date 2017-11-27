
## complie

```
make clean
make
```

## test file

normal mode: it will run the prog directly and print final result

```
./sim [file name]
```

debug mode: single step debug

```
./sim -d [file name]
```

ps: you should notice that the test prog is under sub-directory 'test', so if you want to test prog 'test1',
you should type in cmd:

```
./sim test/test1
```


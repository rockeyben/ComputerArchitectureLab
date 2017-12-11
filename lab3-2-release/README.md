# ComputerArchitectureLab
Lab3-2 for Computer Organization and Architecture
PKU, 2017 Autumn

## Compile

```bash
make clean
make
```

## Test

If you want to simulate one trace file, just type in:

```bash
./sim [Filename]
```

Multi modes are provided for ablation study, try

```bash
./sim -h
```

for details.

### Replace mode

3 replace mode:

- 0 : LRU (default)
- 1 : NRU
- 2 : Tree-PLRU

If you want to use one of the mode, type in the corresponding number after '--replace-mode'

Example:

```bash
./sim [Filename] --replace-mode 2
```

for Tree-PLRU. 


### Prefetch

use '--prefetch'

Example:

```bash

./sim [Filename] --prefetch

```


### Bypass

use '--bypass'

Example:

```bash

./sim [Filename] --bypass
```


### Together

For my final strategy:

- L1 cache: prefetch + NRU

- L2 cache: prefetch + NRU + bypass

type in

```bash
./sim [Filename] --bypass --prefetch --replace-mode 1
``` 


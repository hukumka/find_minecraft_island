# find_minecraft_island

Tool that helps finding "good" minecraft island to play on.

## Building

### Pre requirements

#### Linux
Install those using you favorite package manager.
+ git
+ make
+ gcc

#### Windows

+ git
+ msys2

Make sure msys dir `usr/bin` is in path.

install `gcc` and `make` using command:
```
pacman -S make gcc
```

### Compiling project

Get source code:
```
git clone https://github.com/hukumka/find_minecraft_island
cd find_minecraft_island
git submodules init
```

Build it:
```
make all
```

Run the result:
```
./find_island
```

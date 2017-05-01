# pentomino
pentomino solver

## what is pentomino?
- [pentomino  wikipedia](https://en.wikipedia.org/wiki/Pentomino)

- [ペントミノ　wikipedia](https://ja.wikipedia.org/wiki/%E3%83%9A%E3%83%B3%E3%83%88%E3%83%9F%E3%83%8E)

## build
### os & dependency
- ubuntu / windows10
- clang
### build
```
make
```

## usage
```
usage:  pentomino [-r rows] [-c cols] [-fpm]
-r rows of the board to place the pentomino pieces.
-c cols of the board to place the pentomino pieces.
-f find all solutions.
-p print all solutions.(otherwise, print only first founded solution.)
-m find solutions usinhg openmp.
```

## sample

```
.\pentomino -r 3 -c 20
UUXPPPZYYYYWTFNNNVVV
UXXXPPZZZYWWTFFFNNLV
UUXIIIIIZWWTTTFLLLLV
```

```
.\pentomino -r 6 -c 10
FNNNTTTVVV
FFFNNTLVZZ
UFUWWTLVZY
UUUXWWLZZY
PPXXXWLLYY
PPPXIIIIIY
```
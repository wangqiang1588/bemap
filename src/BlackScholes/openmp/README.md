BlackScholes, OpenMP Implementation
===================================

* Read LICENSE first

* Prerequisites
CMake - Cross Platform Make
- http://www.cmake.org/
 
* Compilation (all projects) in build/
```
$ cd /path/to/bemap
$ cd build/
$ cmake ../
$ make
```

* Compilation (only this project) in TestRun
```
$ cd /path/to/BlackScholes/openmp/
$ cd TestRun
$ cmake ../
$ make
```

* Usage:
```
./BlackScholes_omp -h
./BlackScholes_omp [--verbose|-v] [--help|-h] [--output|-o FILENAME]
     [--optnum|-O NUMBER] [--riskfree|-R NUMBER] [--volatility|-V NUMBER]
     [--prep-time]

* Options *
 --verbose             Be verbose
 --help                Print this message
 --output=NAME         Write to this file
 --optnum=NUMBER       Number of elements in the data array -- default = 50 * 1024 * 1024
 --riskfree=NUMBER     The annualized risk-free interest rate, continuously compounded -- default = 0.02
 --volatility=NUMBER   The volatility of stock's returns -- default = 0.30
 --prep-time           Show initialization, memory preparation and copyback time

 * Examples *
./BlackScholes_omp [OPTS...] -v
./BlackScholes_omp [OPTS...] -v -O 4194304
```

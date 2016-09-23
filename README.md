## Summary
* P1 (problem 1):
  * To run, use `make test`
  * To measure with NOP's inside the measurement loops, add `#define INCLUDE_NOP`
  * To measure the baseline of `clock_gettime()` or `RDTSC`, comment out `#define INCLUDE_NOP`

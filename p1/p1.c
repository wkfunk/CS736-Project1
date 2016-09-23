#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <malloc.h>
#include <time.h>
#include <sched.h>

#define RDTSC(hi, lo)			\
  {					\
    asm volatile (	       		\
      "CPUID\n\t"			\
      "RDTSC\n\t"			\
      :"=a" (lo), "=d" (hi)		\
      :					\
      : "%rbx", "%rcx"			\
    );                          	\
  }

#define RDTSCP(hi, lo)			\
  {					\
    asm volatile (	       		\
      "RDTSCP\n\t"			\
      :"=a" (lo), "=d" (hi)		\
      :					\
      : "%rbx", "%rcx"			\
    );                          	\
    asm volatile ("CPUID");		\
  }

#define JOIN(hi, lo) ((uint64_t) hi << 32) | lo

// make it easier to do nop's
#define NOP asm volatile("nop");
#define NOP_2 NOP; NOP
#define NOP_4 NOP_2; NOP_2
#define NOP_8 NOP_4; NOP_4
#define NOP_16 NOP_8; NOP_8

// number of experiments
#define SAMPLES 1000000
#define LOOPS 25


uint64_t delta(time_t s1, long ns1, time_t s2, long ns2) {
  uint64_t s_diff = (uint64_t) (s2-s1) * 1e9;
  uint64_t ns_diff = ns2-ns1;
  return s_diff + ns_diff;
}

uint64_t delta_struct(struct timespec *t1, struct timespec *t2) {
  return delta(t1->tv_sec, t1->tv_nsec, t2->tv_sec, t2->tv_nsec);
}

void take_gettime_measurements(uint64_t **results)
{
  // see what the timer has to say about resolution
  struct timespec *res = malloc(sizeof(struct timespec));
  assert(clock_getres(CLOCK_REALTIME, res) == 0);

  struct timespec *t1 = malloc(sizeof(struct timespec));
  struct timespec *t2 = malloc(sizeof(struct timespec));

  int i, j;
  for(i = 0; i < LOOPS; i++) {
    for(j = 0; j < SAMPLES; j++) {
      // take a baseline measurement
      clock_gettime(CLOCK_REALTIME, t1);
      //NOP_16
      clock_gettime(CLOCK_REALTIME, t2);
      
      results[i][j] = delta_struct(t1, t2);
    }
  }
}

void take_tsc_measurements(uint64_t **results)
{
  // store the lo and hi registers from RDTSC(P)
  uint32_t lo, hi, lo2, hi2;
  
  // store the total # of cycles by combining lo and hi
  uint64_t pre, post;

  // cache warmup
  RDTSC(hi, lo);
  RDTSCP(hi2, lo2);
  RDTSC(hi, lo);
  RDTSCP(hi2, lo2);

  // take measurements
  int i, j;
  for(i = 0; i < LOOPS; i++) {
    for(j = 0; j < SAMPLES; j++) {
      RDTSC(hi, lo);
      //NOP_16
      RDTSCP(hi2, lo2);

      pre = JOIN(hi, lo);
      post = JOIN(hi2, lo2);
    
      results[i][j] = post - pre;
    }
  }

  return;
}

uint64_t find_min(uint64_t *results, int n)
{
  uint64_t minimum = results[0];
  int i;
  for(i = 0; i < n; i++) {
    if(minimum > results[i]) {
      minimum = results[i];
    }
  }
  return minimum;
}

double find_variance(uint64_t *results, int n) 
{
  uint64_t sum = 0;
  uint64_t sq_sum = 0;
  int i;
  for(i = 0; i < n; i++) {
    sum += results[i];
    sq_sum += results[i] * results[i];
  }
  double mean = sum / n;
  return (double) sq_sum / n - (mean * mean);
}

void fill_mins(uint64_t **results, uint64_t *mins)
{
  int i;
  for(i = 0; i < LOOPS; i++) {
    mins[i] = find_min(results[i], SAMPLES);
  }
  return;
}

void fill_vars(uint64_t **results, double *vars)
{
  int i;
  for(i = 0; i < LOOPS; i++) {
    vars[i] = find_variance(results[i], SAMPLES);
  }
  return;
}

void print_results(uint64_t *mins, double *vars)
{
  int i;
  for(i = 0; i < LOOPS; i++) {
    printf("min: %llu; var: %f\n", mins[i], vars[i]);
  }
}

void print_vals(uint64_t **results)
{
  int i, j;
  for(i = 0; i < LOOPS; i++) {
    printf("loop %d\n----------------\n", i);
    for(j = 0; j< SAMPLES; j++) {
      printf("%d: %llu\n", j, results[i][j]);
    }
  }
}

int main()
{
  // set up cpu priority
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(0, &mask);
  sched_setaffinity(0, sizeof(mask), &mask);

  // initialize results matrix
  uint64_t **results = malloc(sizeof(uint64_t *) * LOOPS);
  int i;
  for(i = 0; i < LOOPS; i++) {
    results[i] = malloc(sizeof(uint64_t) * SAMPLES);
  }
  uint64_t *mins = malloc(sizeof(uint64_t) * LOOPS);
  double *vars = malloc(sizeof(double) * LOOPS);
  
  // run measurements for RDTSC measurement
  take_tsc_measurements(results);
  fill_mins(results, mins);
  fill_vars(results, vars);
  printf("RDTSC:\n----------------------\n");
  print_results(mins, vars);

  // run measurements for gettime()
  take_gettime_measurements(results);
  fill_mins(results, mins);
  fill_vars(results, vars);
  printf("clock_gettime():\n----------------------\n");
  print_results(mins, vars);

  // print out all vals
  //print_vals(results);

  return 0;
}
  

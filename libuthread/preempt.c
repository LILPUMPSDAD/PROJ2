#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"

/* https://stackoverflow.com/questions/3705436/c-error-checking-function */
/* Useful function for error handling instead of reusing the same if then
 * exit statements for every API call or function call */
/* USAGE: EXEC_AND_HANDLE(function, success return value, args) */
#define EXEC_AND_HANDLE(f, r, ...)                                             \
  do {                                                                         \
    if (f(__VA_ARGS__) != r) {                                                 \
      fprintf(stderr, "function: " #f "() failed in %s\n", __FILE__);          \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100

/*
 * itimer only allows seconds, microseconds, and nanoseconds. Converted
 * interval to microseconds.
 */
#define HZ_PER_USEC 1000000 / HZ

void preempt_disable(void) {
  sigset_t ss;
  EXEC_AND_HANDLE(sigemptyset, 0, &ss);
  EXEC_AND_HANDLE(sigaddset, 0, &ss, SIGVTALRM);
  EXEC_AND_HANDLE(sigprocmask, 0, SIG_BLOCK, &ss, NULL);
}

void preempt_enable(void) {
  sigset_t ss;
  EXEC_AND_HANDLE(sigemptyset, 0, &ss);
  EXEC_AND_HANDLE(sigaddset, 0, &ss, SIGVTALRM);
  EXEC_AND_HANDLE(sigprocmask, 0, SIG_UNBLOCK, &ss, NULL);
}

void signal_handler() { uthread_yield(); }

void preempt_start(bool preempt) {
  if (!preempt)
    return;
  struct sigaction sa;

  sa.sa_handler = signal_handler;

  /* Initialize signals by setting all flags to false */
  EXEC_AND_HANDLE(sigemptyset, 0, &sa.sa_mask);
  sa.sa_flags = 0;

  /* Set the signal to execute signal_handler() when alarm is detected */
  EXEC_AND_HANDLE(sigaction, 0, SIGVTALRM, &sa, NULL);

  /* Configure alarm to go off at 100 times per second */
  struct itimerval timer;
  timer.it_interval.tv_usec = (long int)HZ_PER_USEC;
  timer.it_interval.tv_sec = 0;
  timer.it_value.tv_usec = (long int)HZ_PER_USEC;
  timer.it_value.tv_sec = 0;

  EXEC_AND_HANDLE(setitimer, 0, ITIMER_VIRTUAL, &timer, NULL);
}

void preempt_stop(void) {
  struct sigaction sa;
  EXEC_AND_HANDLE(setitimer, 0, ITIMER_VIRTUAL, NULL, NULL);
  sa.sa_handler = SIG_DFL;
  EXEC_AND_HANDLE(sigemptyset, 0, &sa.sa_mask);
  sa.sa_flags = 0;
  EXEC_AND_HANDLE(sigaction, 0, SIGVTALRM, &sa, NULL);
}

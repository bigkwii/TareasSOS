#define _XOPEN_SOURCE 500

#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

#include "nthread-impl.h"

static void nth_RTimerHandler(int sig, siginfo_t *si, void *uc);
static void nth_wakeThreads(void);

static NthTimeQueue *nth_timeQueue;
static long long nth_iniTime;
static timer_t nth_realTimer;

/*************************************************************
 * Prolog and epilog
 *************************************************************/

void nth_timeInit(void) {
  nth_timeQueue= nth_makeTimeQueue();
  nth_iniTime= nGetTimeNanos();

  struct sigaction sigact;
  sigact.sa_flags= SA_SIGINFO;
  sigact.sa_sigaction= nth_RTimerHandler;
  sigact.sa_mask= nth_sigsetCritical;

  if (sigaction(SIGALRM, &sigact, NULL)!=0) {
    perror("sigaction");
    nFatalError("nth_timeInit", "Failed to register signal, cannot continue\n");
  }
    
  struct sigevent sigev;
  sigev.sigev_notify= SIGEV_SIGNAL;
  sigev.sigev_signo= SIGALRM;
  sigev.sigev_value.sival_ptr= &nth_realTimer;
  if ( timer_create(CLOCK_REALTIME,
                    &sigev, &nth_realTimer)!=0 ) {
    perror("timer_create");
    nFatalError("nth_timeInit", "Failed to create timer, cannot continue\n");
  }
}

void nth_timeEnd(void) {
  if (!nth_emptyTimeQueue(nth_timeQueue))
    nFPrintf(stderr, "*** There are pending threads in the time queue\n");
}

/*************************************************************
 * Timeout management
 *************************************************************/

long long nGetTimeNanos(void) {
  struct timespec ts;
  int rc= clock_gettime(CLOCK_REALTIME, &ts);
  if (rc!=0) {
    if (rc==-1)
      perror("clock_gettime");
    nFatalError("clock_gettime", "Failed to get time, cannot continue\n");
  }
  return (long long)ts.tv_sec*1000000000+ts.tv_nsec-nth_iniTime;
}

int nGetTime(void) {
  return nGetTimeNanos()/1000000LL;
}

static void nth_setRealTimerAlarm(long long nanos) {
  struct itimerspec spec;
  spec.it_value.tv_sec= nanos/1000000000;
  spec.it_value.tv_nsec= nanos%1000000000;
  spec.it_interval.tv_sec= 0;
  spec.it_interval.tv_nsec= 0;
  int rc= timer_settime(nth_realTimer, 0, &spec, NULL);
  if (rc!=0) {
    if (rc==-1)
      perror("timer_settime");
    nFatalError("timer_settime", "Failed to set time alarm, cannot continue\n");
  }
}

void nth_programTimer(long long nanos, void (*wakeUpFun)(nThread th)) {
  CHECK_CRITICAL("nth_programTimer")
  nThread thisTh= nSelf();
  if (nanos<=0)
    setReady(thisTh);
  else {
    long long currTime= nGetTimeNanos();
    long long wakeTime= currTime+nanos;
    if ( nth_emptyTimeQueue(nth_timeQueue) ||
         wakeTime-nth_nextTime(nth_timeQueue)<0 ) {
      nth_setRealTimerAlarm(wakeTime-currTime);
    }
    thisTh->wakeUpFun= wakeUpFun;
    nth_putTimed(nth_timeQueue, thisTh, wakeTime);
  }
}

void nth_cancelThread(nThread th) {
  CHECK_CRITICAL("nth_cancelTask")
  nth_delTimed(nth_timeQueue, th);
  nth_wakeThreads();
}

static void nth_wakeThreads(void) {
  long long currTime= nGetTimeNanos();
  // Wake up all threads with wake time <= currTime
  while ( !nth_emptyTimeQueue(nth_timeQueue) &&
          nth_nextTime(nth_timeQueue)<=currTime ) {
    nThread th= nth_getTimed(nth_timeQueue);
    if (th->wakeUpFun!=NULL)
      (*th->wakeUpFun)(th);
    setReady(th);
  }
  
  nth_setRealTimerAlarm( nth_emptyTimeQueue(nth_timeQueue) ?
                           0 : nth_nextTime(nth_timeQueue)-currTime );
}

static void nth_RTimerHandler(int sig, siginfo_t *si, void *uc) {
  // If this core is waiting in nth_rrSchedule, a return to nth_rrSchedule
  // is mandatory because calling recursively nth_rrSchedule is unsafe
    
  START_HANDLER
  
  nth_wakeThreads();
  nThread th= nth_selfCritical();
  // Buggy: if (! nth_coreIsIdle[nth_coreId()] && th!=NULL)
  if (! nth_coreIsIdle[nth_coreId()])
    schedule();
  
  END_HANDLER
}

/*************************************************************
 * nSleepNanos
 *************************************************************/

int nSleepNanos(long long nanos) {
  START_CRITICAL

  nThread thisTh= nSelf();
  suspend(WAIT_SLEEP);
  nth_programTimer(nanos, NULL);
  schedule();

  END_CRITICAL
  return 0;
}

int nSleepMillis(long long millis) {
  nSleepNanos(millis*1000000LL);
  return 0;
}

int nSleepMicros(useconds_t micros) {
  nSleepNanos(micros*1000LL);
  return 0;
}

int nSleepSeconds(unsigned int seconds) {
  nSleepNanos(seconds*1000000000LL);
  return 0;
}

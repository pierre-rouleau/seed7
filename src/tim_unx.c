/********************************************************************/
/*                                                                  */
/*  tim_unx.c     Time access using the unix capabilitys.           */
/*  Copyright (C) 1989 - 2005  Thomas Mertes                        */
/*                                                                  */
/*  This file is part of the Seed7 Runtime Library.                 */
/*                                                                  */
/*  The Seed7 Runtime Library is free software; you can             */
/*  redistribute it and/or modify it under the terms of the GNU     */
/*  Lesser General Public License as published by the Free Software */
/*  Foundation; either version 2.1 of the License, or (at your      */
/*  option) any later version.                                      */
/*                                                                  */
/*  The Seed7 Runtime Library is distributed in the hope that it    */
/*  will be useful, but WITHOUT ANY WARRANTY; without even the      */
/*  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR */
/*  PURPOSE.  See the GNU Lesser General Public License for more    */
/*  details.                                                        */
/*                                                                  */
/*  You should have received a copy of the GNU Lesser General       */
/*  Public License along with this program; if not, write to the    */
/*  Free Software Foundation, Inc., 59 Temple Place, Suite 330,     */
/*  Boston, MA 02111-1307 USA                                       */
/*                                                                  */
/*  Module: Seed7 Runtime Library                                   */
/*  File: seed7/src/tim_unx.c                                       */
/*  Changes: 1992, 1993, 1994  Thomas Mertes                        */
/*  Content: Time access using the unix capabilitys.                */
/*                                                                  */
/********************************************************************/

#include "stdio.h"
#include "time.h"
#include "sys/time.h"
#include "signal.h"
#include "setjmp.h"

#include "version.h"
#include "common.h"

#undef EXTERN
#define EXTERN
#include "tim_drv.h"


#undef TRACE_TIM_UNX
#define USE_SIGACTION


#ifdef ANSI_C
#ifdef C_PLUS_PLUS

extern "C" unsigned int sleep (unsigned int);
extern "C" int pause (void);

#else

unsigned int sleep (unsigned int);
int pause (void);

#endif
#else

unsigned int sleep ();
int pause ();

#endif


#ifdef USE_SIGACTION
sigjmp_buf wait_finished;
#else
jmp_buf wait_finished;
#endif



#ifdef ANSI_C

void get_time (time_t *calendar_time, long *mycro_secs, long *time_zone)
#else

void get_time (calendar_time, mycro_secs, time_zone)
time_t *calendar_time;
long *mycro_secs;
long *time_zone;
#endif

  {
    struct timeval time_val;
    struct timezone this_time_zone;

  /* get_time */
#ifdef TRACE_TIM_UNX
    printf("BEGIN get_time\n");
#endif
    gettimeofday(&time_val, &this_time_zone);
    *calendar_time = time_val.tv_sec;
    *mycro_secs = time_val.tv_usec;
    *time_zone = this_time_zone.tz_minuteswest;
#ifdef TRACE_TIM_UNX
    printf("END get_time\n");
#endif
  } /* get_time */



#ifdef USE_SIGACTION



#ifdef ANSI_C

static void alarm_signal_handler (int sig_num)
#else

static void alarm_signal_handler (sig_num)
int sig_num;
#endif

  { /* alarm_signal_handler */
#ifdef TRACE_TIM_UNX
    printf("BEGIN alarm_signal_handler\n");
#endif
    siglongjmp(wait_finished, 1);
#ifdef TRACE_TIM_UNX
    printf("END alarm_signal_handler\n");
#endif
  } /* alarm_signal_handler */



#ifdef ANSI_C

void await_time (time_t calendar_time, long mycro_secs)
#else

void await_time (calendar_time, mycro_secs)
time_t calendar_time;
long mycro_secs;
#endif

  {
#ifdef OUT_OF_ORDER
    time_t now;
#endif
    struct timeval time_val;
    struct timezone this_time_zone;
    struct itimerval timer_value;
    struct sigaction action;

  /* await_time */
#ifdef TRACE_TIM_UNX
    printf("BEGIN await_time\n");
#endif
    gettimeofday(&time_val, &this_time_zone);
    timer_value.it_value.tv_sec = calendar_time - time_val.tv_sec;
    if (mycro_secs >= time_val.tv_usec) {
      timer_value.it_value.tv_usec = mycro_secs - time_val.tv_usec;
    } else {
      timer_value.it_value.tv_usec = 1000000 - time_val.tv_usec + mycro_secs;
      timer_value.it_value.tv_sec--;
    } /* if */
#ifdef TRACE_TIM_UNX
    fprintf(stderr, "%d %ld %ld %ld %ld %ld %ld %ld %ld\n",
        this_time_zone.tz_minuteswest,
        time_val.tv_sec,
        calendar_time,
        calendar_time - time_val.tv_sec,
        time_val.tv_usec,
        mycro_secs,
        mycro_secs - time_val.tv_usec,
        timer_value.it_value.tv_sec,
        timer_value.it_value.tv_usec);
#endif
    if (timer_value.it_value.tv_sec > 0 ||
        (timer_value.it_value.tv_sec == 0 &&
        timer_value.it_value.tv_usec > 0)) {
      timer_value.it_interval.tv_sec = 0;
      timer_value.it_interval.tv_usec = 0;
      action.sa_handler = &alarm_signal_handler;
      sigemptyset(&action.sa_mask);
      action.sa_flags = 0;
      if (sigaction(SIGALRM, &action, NULL) != SIG_ERR) {
        if (sigsetjmp(wait_finished, 1) == 0) {
          if (setitimer(ITIMER_REAL, &timer_value, NULL) == 0) {
            pause();
          } /* if */
        } /* if */
      } /* if */
    } /* if */
#ifdef OUT_OF_ORDER
    now = time_val.tv_sec;
    if (now < calendar_time) {
      if (calendar_time - now > 1) {
        sleep(calendar_time - now - 1);
      } /* if */
      do {
        ;
      } while (time(NULL) < calendar_time);
    } /* if */
    if (mycro_secs != 0) {
      do {
        gettimeofday(&time_val, &this_time_zone);
      } while (time_val.tv_sec <= calendar_time &&
          time_val.tv_usec < mycro_secs);
    } /* if */
#endif
#ifdef TRACE_TIM_UNX
    printf("END await_time\n");
#endif
  } /* await_time */



#else



#ifdef ANSI_C

static void alarm_signal_handler (int sig_num)
#else

static void alarm_signal_handler (sig_num)
int sig_num;
#endif

  { /* alarm_signal_handler */
#ifdef TRACE_TIM_UNX
    printf("BEGIN alarm_signal_handler\n");
#endif
    longjmp(wait_finished, 1);
#ifdef TRACE_TIM_UNX
    printf("END alarm_signal_handler\n");
#endif
  } /* alarm_signal_handler */



#ifdef ANSI_C

void await_time (time_t calendar_time, long mycro_secs)
#else

void await_time (calendar_time, mycro_secs)
time_t calendar_time;
long mycro_secs;
#endif

  {
#ifdef OUT_OF_ORDER
    time_t now;
#endif
    struct timeval time_val;
    struct timezone this_time_zone;
    struct itimerval timer_value;

  /* await_time */
#ifdef TRACE_TIM_UNX
    printf("BEGIN await_time\n");
#endif
    gettimeofday(&time_val, &this_time_zone);
    timer_value.it_value.tv_sec = calendar_time - time_val.tv_sec;
    if (mycro_secs >= time_val.tv_usec) {
      timer_value.it_value.tv_usec = mycro_secs - time_val.tv_usec;
    } else {
      timer_value.it_value.tv_usec = 1000000 - time_val.tv_usec + mycro_secs;
      timer_value.it_value.tv_sec--;
    } /* if */
#ifdef TRACE_TIM_UNX
    fprintf(stderr, "%d %ld %ld %ld %ld %ld %ld %ld %ld\n",
        this_time_zone.tz_minuteswest,
        time_val.tv_sec,
        calendar_time,
        calendar_time - time_val.tv_sec,
        time_val.tv_usec,
        mycro_secs,
        mycro_secs - time_val.tv_usec,
        timer_value.it_value.tv_sec,
        timer_value.it_value.tv_usec);
#endif
    if (timer_value.it_value.tv_sec > 0 ||
        (timer_value.it_value.tv_sec == 0 &&
        timer_value.it_value.tv_usec > 0)) {
      timer_value.it_interval.tv_sec = 0;
      timer_value.it_interval.tv_usec = 0;
      if (signal(SIGALRM, alarm_signal_handler) != SIG_ERR) {
        if (setjmp(wait_finished) == 0) {
          if (setitimer(ITIMER_REAL, &timer_value, NULL) == 0) {
            pause();
          } /* if */
        } /* if */
      } /* if */
    } /* if */
#ifdef OUT_OF_ORDER
    now = time_val.tv_sec;
    if (now < calendar_time) {
      if (calendar_time - now > 1) {
        sleep(calendar_time - now - 1);
      } /* if */
      do {
        ;
      } while (time(NULL) < calendar_time);
    } /* if */
    if (mycro_secs != 0) {
      do {
        gettimeofday(&time_val, &this_time_zone);
      } while (time_val.tv_sec <= calendar_time &&
          time_val.tv_usec < mycro_secs);
    } /* if */
#endif
#ifdef TRACE_TIM_UNX
    printf("END await_time\n");
#endif
  } /* await_time */



#endif

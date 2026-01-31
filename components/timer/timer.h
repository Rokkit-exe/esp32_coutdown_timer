#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MIN_TIME_MIN = 0
#define MAX_TIME_MIN = 120 

/* ===================== Mode Enum =====================
 * Enum for different timer modes
 * SETTING: Timer is being set
 * RUNNING: Timer is actively counting down
 * PAUSED: Timer is paused
 * */
enum Mode {
    SETTING,
    RUNNING,
    PAUSED,
};

typedef struct Timer Timer;

Timer* init_timer(int8_t duration_min);
void timer_set_duration(Timer* t, int64_t duration_us);
enum Mode timer_get_mode(Timer* t);
void timer_set_mode(Timer* t, enum Mode mode);
void timer_start(Timer* t, int64_t duration_us);
void timer_pause(Timer* t);
void timer_resume(Timer* t);
void timer_stop(Timer* t);
bool timer_expired(Timer* t);
int64_t timer_remaining(Timer* t);
int64_t min_to_us(int8_t minutes);

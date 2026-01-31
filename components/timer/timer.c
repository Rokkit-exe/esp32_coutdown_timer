#include "esp_timer.h"
#include <stdint.h>
#include "timer.h"

/* ===================== TIMER STRUCT =====================
 * Holds the state and configuration of the countdown timer
 * mode: current mode of the timer (SETTING, RUNNING, PAUSED)
 * start_time_us: timestamp when the timer was started or resumed
 * duration_us: total duration set for the timer in microseconds
 * */
struct Timer {
    enum Mode mode;
    int64_t start_time_us;   // microsecond timestamp
    int64_t duration_us;     // duration in microseconds
};

/* ===================== MIN TO US =====================
 * Converts minutes to microseconds
 * minutes: duration in minutes
 * returns: duration in microseconds
 * */
int64_t min_to_us(int8_t minutes) {
  return (int64_t)minutes * 60 * 1000000;
}

/* ===================== INIT TIMER =====================
 * Initializes a Timer struct with given duration in minutes
 * duration_min: initial duration in minutes
 * returns: pointer to Timer struct
 * */
Timer* init_timer(int8_t duration_min) {
  Timer* t = calloc(1, sizeof(Timer));
  if (!t) return NULL;

  t->mode = SETTING;
  t->duration_us = min_to_us(duration_min);

  return t;
}

/* ===================== SET DURATION =====================
 * Sets the timer duration in microseconds
 * t: pointer to Timer struct
 * duration_us: duration in microseconds
 * */
void timer_set_duration(Timer* t, int64_t duration_us) {
  if (!t) return;
  t->duration_us = duration_us;
}

/* ===================== GET MODE =====================
 * Gets the current mode of the timer
 * t: pointer to Timer struct
 * returns: current mode of the timer
 * */
enum Mode timer_get_mode(Timer* t) {
    if (!t) return SETTING; // default
    return t->mode;
}

/* ===================== SET MODE =====================
 * Sets the current mode of the timer
 * t: pointer to Timer struct
 * mode: mode to set
 * */
void timer_set_mode(Timer* t, enum Mode mode) {
    if (!t) return;
    t->mode = mode;
}

/* ===================== START TIMER =====================
 * Starts the timer countdown with given duration
 * t: pointer to Timer struct
 * duration_us: duration in microseconds
 * */
void timer_start(Timer* t, int64_t duration_us) {
    if (!t) return;
    t->duration_us = duration_us;
    t->start_time_us = esp_timer_get_time(); // ESP-IDF microsecond timer
    t->mode = RUNNING;
}

/* ===================== PAUSE TIMER =====================
 * Pauses the timer countdown
 * t: pointer to Timer struct
 * */
void timer_pause(Timer* t) {
    if (!t) return;
    int64_t now = esp_timer_get_time();
    int64_t elapsed_us = now - t->start_time_us;
    t->duration_us -= elapsed_us; // reduce remaining duration
    t->mode = PAUSED;
}

/* ===================== RESUME TIMER =====================
 * Resumes the timer countdown from paused state
 * t: pointer to Timer struct
 * */
void timer_resume(Timer* t) {
    if (!t) return;
    t->start_time_us = esp_timer_get_time(); // restart from now
    t->mode = RUNNING;
}

/* ===================== STOP TIMER =====================
 * Stops the timer and resets to SETTING mode
 * t: pointer to Timer struct
 * */
void timer_stop(Timer* t) {
    if (!t) return;
    t->mode = SETTING;
}

/* ===================== TIMER EXPIRED =====================
 * Checks if the timer has expired
 * t: pointer to Timer struct
 * returns: true if expired, false otherwise
 * */
bool timer_expired(Timer* t) {
    if (!t) return false;
    if (t->mode != RUNNING) return false; // Can't expire if not running

    int64_t now = esp_timer_get_time();
    if ((now - t->start_time_us) >= t->duration_us) {
        t->duration_us = 0; 
        t->mode = SETTING;
        return true;
    }
    return false;
}

/* ===================== TIMER REMAINING =====================
 * Gets the remaining time on the timer in microseconds
 * t: pointer to Timer struct
 * returns: remaining time in microseconds
 * */
int64_t timer_remaining(Timer* t) {
    if (!t) return 0;
    if (t->mode == SETTING) return t->duration_us; // Show the target time
    if (t->mode == PAUSED)  return t->duration_us; // Show the frozen time
    
    // If RUNNING, calculate actual remaining
    int64_t now = esp_timer_get_time();
    int64_t elapsed = now - t->start_time_us;
    int64_t remaining = t->duration_us - elapsed;
    
    return (remaining > 0) ? remaining : 0;
}




#include "wavesets.h"
#include "classes.h"
#include "utils.h"

/*
  wavesetjitter~:
     - does granular synthesis but with wavesets as grains
     - single voice
     - synchronicity as a parameter
       -> interpolation between the sound as it is and a jittered in time Version of it
     - filterfunction from wavesetstepper~
     - parameter determining how randomly a waveset is picked from the buffer
       -> 0 means wavesets are picked in Order of the buffer
     - density (random omission) from 0 to 1
       -> 0 means no wavesets are played, 1 means all are played
     
     Implementation:
     - an event-struct determining when a waveset starts and ends being played determined by a jitter factor
     - list of events in the object corresponding to all currently played or cued to be played wavesets
     - function generating a new wavesets once one is removed from the list
 */


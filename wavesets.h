/*
  File for the necessary Datastructures 
 */

#include "tabread.h"

typedef struct {
  int start_index;
  int end_index;
  int size;
  
  // a value between 0 and 1 with 0 being the
  // smallest size in the wavesetarray and 1 the biggest
  t_float filt;
} t_waveset;


typedef struct _wavesetplayer_tilde
{
  t_object x_obj;
  t_arrayvec x_v;
  t_float x_f;
  int num_wavesets;
  t_waveset* waveset_array;
  // index of the waveset being played in the waveset_array
  int current_waveset;
  // index of the currently played sample
  int current_index;
  t_outlet* x_out, *f_out, *trig_out;
  
} t_wavesetplayer_tilde;


typedef struct _wavesetstepper_tilde
{
  t_object x_obj;
  t_arrayvec x_v;
  // dummy for the first inlet
  t_float x_f;

  // the increment of reading inside the waveset array
  t_float step;
  t_float step_c;
  /*
    how many wavesets are playes until the current
    waveset is reset to the value in the first inlet
  */
  t_float delta;
  t_float delta_c;
  
  // data for omission
  t_float o_fac;
  t_float o_fac_c;
  int is_omitted;

  // for wavesetfiltering
  t_float filt_1;
  t_float filt_2;
  
  int num_wavesets;
  t_waveset* waveset_array;
  // index of the waveset being played in the waveset_array
  int current_waveset;
  // index of the currently played sample
  int current_index;
  
  t_inlet* step_in, *delta_in, *o_fac_in, *filt1_in, *filt2_in;
  t_outlet* x_out, *f_out, *trig_out;

} t_wavesetstepper_tilde;

void free_wavesets_player(t_wavesetplayer_tilde* x)
{
  freebytes(x->waveset_array, (x->num_wavesets) * sizeof(t_waveset));
}

void free_wavesets_stepper(t_wavesetstepper_tilde* x)
{
  freebytes(x->waveset_array, (x->num_wavesets) * sizeof(t_waveset));
}

/*
  File for the necessary Datastructures 
 */

#include "tabread.h"

typedef struct {
  int start_index;
  int end_index;
  int size;
} t_waveset;


typedef struct _wavesetplayer_tilde
{
  t_object x_obj;
  t_arrayvec x_v;
  t_float x_f;
  int num_wavesets;
  t_waveset* waveset_array;
  int current_waveset;
  int current_index;

  t_outlet* x_out, *f_out, *trig_out;

} t_wavesetplayer_tilde;


typedef struct _wavesetstepper_tilde
{
  t_object x_obj;
  t_arrayvec x_v;
  
  t_float x_f;
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
  
  int num_wavesets;
  t_waveset* waveset_array;
  int current_waveset;
  int current_index;

  t_inlet* step_in, *delta_in, *o_fac_in;
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


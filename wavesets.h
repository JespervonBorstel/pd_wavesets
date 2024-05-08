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

  t_outlet* x_out, *f_out, *b_out;

} t_wavesetplayer_tilde;

void free_wavesets(t_wavesetplayer_tilde* x)
{
  freebytes(x->waveset_array, (x->num_wavesets) * sizeof(t_waveset));
}

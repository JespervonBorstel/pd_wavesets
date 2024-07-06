/*
  File for the necessary Datastructures 
 */
#include <m_pd.h>
#include <puredata/g_canvas.h>

typedef struct _node t_node;
typedef struct _ref_list t_ref_list;

typedef struct {
  int start_index;
  int end_index;
  int size;
  
  // a value between 0 and 1 with 0 being the
  // smallest size in the wavesetarray and 1 the biggest
  t_float filt;
} t_waveset;


typedef struct _wavesetbuffer
{
  t_object x_obj;
  
  t_symbol* buffer_name;
  t_symbol* array_name;

  int sorted;
  int* sorted_lookup;
  
  int num_wavesets;
  t_waveset* waveset_array;

  int a_vec_length;
  t_word* a_vec;

  // a pointer to the list of all dsp-objects referencing this object
  t_ref_list *reference_listp;
  
  t_outlet* f_out;
  
} t_wavesetbuffer;

typedef struct _wavesetplayer_tilde
{
  t_object x_obj;
  t_float x_f;
  // index of the waveset being played in the waveset_array
  int current_waveset;
  // index of the currently played sample
  int current_index;
  t_outlet* x_out, *f_out, *trig_out;
  
} t_wavesetplayer_tilde;

typedef struct _wavesetstepper_tilde
{
  t_object x_obj;
  t_symbol *buffer_name;
  t_wavesetbuffer *bufp;
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

  t_float filt_1;
  t_float filt_2;
  int* filter_lookup;
  
  // index of the waveset being played in the waveset_array
  int current_waveset;
  // index of the currently played sample
  int current_index;
  
  t_inlet* step_in, *delta_in, *o_fac_in, *filt1_in, *filt2_in;
  t_outlet* x_out, *freq_out, *trig_out;

} t_wavesetstepper_tilde;

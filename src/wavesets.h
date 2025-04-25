/*
  File for the necessary Datastructures 
 */

#ifndef WAVESETS_H
#define WAVESETS_H
#include <m_pd.h>
#include <puredata/g_canvas.h>
#include <math.h>
#include <stdlib.h>

/* Enum for the different objects */
typedef enum {
  wavesetstepper,
  wavesetplayer,
  wavesetclouds,
  wavesetjitter
} object_type;

typedef struct _wavesetstepper_tilde t_wavesetstepper_tilde;
typedef struct _wavesetplayer_tilde t_wavesetplayer_tilde;
typedef struct _wavesetclouds_tilde t_wavesetclouds_tilde;
typedef struct _wavesetjitter_tilde t_wavesetjitter_tilde;

/* Union for different data types */
typedef union {
  t_wavesetstepper_tilde* wavesetstepper;
  t_wavesetplayer_tilde* wavesetplayer;
  t_wavesetclouds_tilde* wavesetclouds;
  t_wavesetjitter_tilde* wavesetjitter;
} reference_pointer;

/* Node structure */
typedef struct _node {
  reference_pointer object_pointer;
  object_type type;
  struct _node *next;
} t_node;

typedef struct _ref_list {
  t_node* head;
} t_ref_list;

typedef struct _waveset {
  int start_index;
  int end_index;
  int size;
  
  // a value between 0 and 1 with 0 being the
  // smallest size in the wavesetarray and 1 the biggest
  t_float filt;
  // value of the loudest sample in the waveset
  t_sample loudest;
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
  t_symbol* buffer_name;
  t_wavesetbuffer* bufp;
  // index of the waveset being played in the waveset_array
  int current_waveset;
  void (*update_fun_pointer)(struct _wavesetplayer_tilde*);
  // index of the currently played sample
  int current_index;
  t_outlet* x_out, *trig_out;
  
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

  // the mix factor for normalizing the level
  t_float normalise;
  // value of the pitch to be forced on the played wavesets as a frequency
  t_float forced_pitch;
  t_float pitch_mix;
  
  // filter lookup is now saved here 
  int* filter_lookup;
  int lookup_size;
  
  // store a function that is called by wavesetbuffer, when it changes
  void (*update_fun_pointer)(struct _wavesetstepper_tilde*);
  // index of the waveset being played in the waveset_array
  int current_waveset;
  // index of the currently played sample
  t_sample current_index;
  
  t_inlet* step_in, *plb_in, *delta_in, *o_fac_in;
  t_outlet* x_out, *freq_out, *trig_out;

} t_wavesetstepper_tilde;

/* Data types for wavesetclouds~ */

typedef struct _event t_event;
typedef struct _event_list t_event_list;

typedef struct _wavesetclouds_tilde
{
  t_object x_obj;
  t_symbol* buffer_name;
  t_wavesetbuffer* bufp;

  // likelyhood of a waveset being spawned in a voice in the first default inlet
  t_float density;
  // array of events currently played
  t_float voices;
  int max_voices;
  t_event** event_array;
  void (*update_fun_pointer)(struct _wavesetclouds_tilde*);

  t_inlet *voices_in;
  t_outlet* x_out;
  
} t_wavesetclouds_tilde;

typedef struct _wavesetjitter_tilde
{
  t_object x_obj;
  t_symbol* buffer_name;
  t_wavesetbuffer* bufp;

  void (*update_fun_pointer)(struct _wavesetjitter_tilde*);
} t_wavesetjitter_tilde;

#endif

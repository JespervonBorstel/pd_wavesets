/*
  File for the necessary Datastructures 
 */

#ifndef WAVESETS_H
#define WAVESETS_H
#include <m_pd.h>
#include <puredata/g_canvas.h>
#include <math.h>

/* implementation of the reference list in the buffer object
 * if new dsp object are added everything here needs to be updated:
 *  - type of the object added to the enum and union
 *  - nodecreation und remove function
 *  - add case to add_to_reference-list
 */

/* Enum for the different objects */
typedef enum {
  wavesetstepper,
  wavesetplayer
} object_type;

typedef struct _wavesetstepper_tilde t_wavesetstepper_tilde;
typedef struct _wavesetplayer_tilde t_wavesetplayer_tilde;

/* Union for different data types */
typedef union {
  t_wavesetstepper_tilde* wavesetstepper;
  t_wavesetplayer_tilde* wavesetplayer;
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
  int* filter_lookup;
  int lookup_size;

  // store a function that is called by wavesetbuffer, when it changes
  void (*update_fun_pointer)(struct _wavesetstepper_tilde*);
  // index of the waveset being played in the waveset_array
  int current_waveset;
  // index of the currently played sample
  int current_index;
  
  t_inlet* step_in, *delta_in, *o_fac_in;
  t_outlet* x_out, *freq_out, *trig_out;

} t_wavesetstepper_tilde;

#endif

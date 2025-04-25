#include "wavesets.h"
#include "classes.h"
#include "utils.h"

/*
  does granular synthesis with wavesets as grains
     - parameters:
        - spawn-rate of new events
	- area in the buffer from where to pick new wavesets
	- filter method


 */

inline t_event* generate_new_event(t_waveset* waveset_array, int num_wavesets, t_float density)
{
  if(num_wavesets > 0 && waveset_array) {
    t_event* event = (t_event*)getbytes(sizeof(t_event));
    int waveset_index = rand() % num_wavesets;
    
    event->waveset = &(waveset_array[waveset_index]);  
    event->play_index = 0;
    event->skip = ((t_float)rand() / (t_float)RAND_MAX) > density ? 1 : 0;
    return event;
  }
  else
    return NULL;
}

void initialize_event_array(t_wavesetclouds_tilde* x)
{
  x->event_array = (t_event**)getbytes(x->max_voices * sizeof(t_event*));
  for(int i = 0; i < x->max_voices; i++) {
    x->event_array[i] = (t_event*)getbytes(sizeof(t_event));
  }
}

void fill_event_array(t_wavesetclouds_tilde* x)
{
  if(x->bufp) {
    t_waveset* waveset_array = x->bufp->waveset_array;
    int num_wavesets = x->bufp->num_wavesets;
    t_float density = x->density;
    x->event_array = (t_event**)getbytes(x->max_voices * sizeof(t_event*));
    t_event** event_array = x->event_array;
    
    for(int i = 0; i < x->max_voices; i++) {
      event_array[i] = generate_new_event(waveset_array, num_wavesets, density);
    }
  }
  else
    pd_error(x, "wavesetclouds~: no bufferobject found while in init function");
}

void update_wavesetclouds_tilde(t_wavesetclouds_tilde* x)
{
  if(x->bufp) {
    t_waveset* waveset_array = x->bufp->waveset_array;
    int num_wavesets = x->bufp->num_wavesets;
    t_float density = x->density;
    t_event** event_array = x->event_array;
    
    for(int i = 0; i < x->max_voices; i++) {
      freebytes(event_array[i], sizeof(t_event));
      event_array[i] = generate_new_event(waveset_array, num_wavesets, density);
    }
  }
  else
    pd_error(x, "wavesetclouds~: no bufferobject found while in update function");
}

void *wavesetclouds_tilde_new(t_symbol *buffer, t_floatarg max_voices)
{
  t_wavesetclouds_tilde *x = (t_wavesetclouds_tilde *)pd_new(wavesetclouds_tilde_class);

  x->density = 1;
  x->voices = 10;

  if(max_voices) {
    // hardcoded limit of 4096 voices. This value is debatable.
    x->max_voices = (max_voices <= 0 || max_voices > 4096) ? 10 : (int)max_voices;
  }
  else {
    post("wavesetclouds~: no voice maximum provided. Default to 10 voices");
    x->max_voices = 10;
  }

  x->event_array = (t_event**)getbytes(x->max_voices * sizeof(t_event*));
  
  if (!buffer)
    pd_error(x, "wavesetclouds~: no buffer name provided");
  t_wavesetbuffer *a;
  a = (t_wavesetbuffer*)pd_findbyclass((x->buffer_name = buffer), wavesetbuffer_class);
  
  if(a) {
    x->bufp = a;
    
    /* adding the object to the reference list in the bufferobject */
    reference_pointer rp;
    rp.wavesetclouds = x;
    add_to_reference_list(rp, wavesetclouds, a);
    fill_event_array(x);
  }
  else {
    pd_error(x, "wavesetclouds~: no bufferobject found");
    x->bufp = NULL;
    x->event_array = NULL;
  }

  x->density = 1;
  x->voices = 10;

  if(max_voices) {
    // hardcoded limit of 4096 voices. This value is debatable.
    x->max_voices = (max_voices <= 0 || max_voices > 4096) ? 10 : (int)max_voices;
  }
  else {
    post("wavesetclouds~: no voice maximum provided. Default to 10 voices");
    x->max_voices = 10;
  }
  
  x->update_fun_pointer = &update_wavesetclouds_tilde;
  x->voices_in = floatinlet_new(&x->x_obj, &x->voices);
  x->x_out = outlet_new(&x->x_obj, &s_signal);

  return (x);
}

void wavesetclouds_tilde_set(t_wavesetclouds_tilde *x, t_symbol *s)
{
  if(s != x->buffer_name) {
    /* if a new buffer was specified remove the reference
       in the old buffer and add it to the new */
    reference_pointer rp;
    rp.wavesetclouds = x;
    remove_from_reference_list(rp, wavesetclouds, x->bufp);
    
    t_wavesetbuffer *a = NULL;
    a = (t_wavesetbuffer *)pd_findbyclass((x->buffer_name = s), wavesetbuffer_class);
    if(a) {
      x->bufp = a;
      /* adding the object to the reference list in the bufferobject */
      add_to_reference_list(rp, wavesetclouds, a);
      update_wavesetclouds_tilde(x);
    }
    else {
      pd_error(x, "wavesetclouds ~: no bufferobject found");
      x->bufp = NULL;
    }
  }
}

t_int *wavesetclouds_tilde_perform(t_int *w)
{
  t_wavesetclouds_tilde *x = (t_wavesetclouds_tilde *)(w[1]);
  const t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  int n = (int)(w[4]), i;
  const int max_voices = x->max_voices;
  // voices defaults to 0 if out of bounds
  const int voices = (int)((int)x->voices > max_voices || (int)x->voices < 0 ? 0 : x->voices);
  t_event** event_array = x->event_array;

  if(!x->bufp)
    goto zero;

  t_word *buf = x->bufp->a_vec;
  t_waveset *waveset_array = x->bufp->waveset_array;
  
  int num_wavesets = x->bufp->num_wavesets;
  
  if (!buf
      || !waveset_array || !event_array)
    goto zero;
  
  for (i = 0; i < n; i++) {
    t_sample accum_samp = 0;
    // looping over every voice for every sample
    for (int j = 0; j < voices; j++) {
      t_event* event = event_array[j];
      // in case a waveset finishes playing a new one is generated
      if(event->play_index > event->waveset->size) {
	freebytes(event, sizeof(t_event));
	event = generate_new_event(waveset_array, num_wavesets, in[j]);
	event_array[j] = event;
      }
      if(!event->skip)
	accum_samp += buf[event->play_index + event->waveset->start_index].w_float;
      
      (event->play_index)++;
    }
    // divide by the number of voices to have a reasonable output level
    *out++ = (voices > 0) ? (accum_samp / voices) : 0;
  }
  return (w+5);
  
 zero:
  while (n--) {
    *out++ = 0;
  } 
  return (w+5);
}

void wavesetclouds_tilde_dsp(t_wavesetclouds_tilde *x, t_signal **sp)
{
  dsp_add(wavesetclouds_tilde_perform, 4, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void free_event_array(t_event** event_array, int max_voices)
{
  for(int i = 0; i < max_voices; i++) {
    freebytes(event_array[i], sizeof(t_event));
  }
}

void wavesetclouds_tilde_free(t_wavesetclouds_tilde *x)
{
  if(x->bufp) {
    reference_pointer rp;
    rp.wavesetclouds = x;
    remove_from_reference_list(rp, wavesetclouds, x->bufp);
  }
  free_event_array(x->event_array, x->max_voices);
  freebytes(x->event_array, x->max_voices * sizeof(t_event));

  x->event_array = NULL;
  x->bufp = NULL;
  x->update_fun_pointer = NULL;
  
  inlet_free(x->voices_in);
  outlet_free(x->x_out);
}

void wavesetclouds_tilde_setup(void)
{
  wavesetclouds_tilde_class = class_new(gensym("wavesetclouds~"),
					(t_newmethod)wavesetclouds_tilde_new,
					(t_method)wavesetclouds_tilde_free,
					sizeof(t_wavesetclouds_tilde),
					CLASS_DEFAULT,
					A_DEFSYMBOL,
					A_DEFFLOAT,
					0);
  CLASS_MAINSIGNALIN(wavesetclouds_tilde_class, t_wavesetclouds_tilde, density);
  class_addmethod(wavesetclouds_tilde_class, (t_method)wavesetclouds_tilde_dsp,
		  gensym("dsp"), A_CANT, 0);
  class_addmethod(wavesetclouds_tilde_class, (t_method)wavesetclouds_tilde_set,
		  gensym("set"), A_DEFSYMBOL, 0);
}

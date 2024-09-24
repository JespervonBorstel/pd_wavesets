#include "wavesets.h"
#include "classes.h"
#include "utils.h"

/*
  wavesetstepper: wavesetplayer with additional Features
     - reads a certain amount of wavesets from an array by increment starting from the waveset
       specified by the index given in the first inlet.
     - waveset omission
 */

/* functions to clean up the perform routine */

inline t_sample four_point_interpolate(t_word *buf, t_word *wp, const t_sample idx,
				       const int maxindex, const t_sample one_over_six)
{
  t_sample out_samp;
  double findex = (double)idx;
  int index = findex;
  t_sample frac,  a,  b,  c,  d, cminusb;
  if (index < 1)
    index = 1, frac = 0;
  else if (index > maxindex)
    index = maxindex, frac = 1;
  else frac = findex - index;
  wp = buf + index;
  a = wp[-1].w_float;
  b = wp[0].w_float;
  c = wp[1].w_float;
  d = wp[2].w_float;
  cminusb = c-b;
  return out_samp = b + frac * (cminusb - one_over_six * ((t_sample)1.-frac)
				* ((d - a - (t_sample)3.0 * cminusb)
				   * frac + (d + a*(t_sample)2.0 - b*(t_sample)3.0)));
}

/* writes all the changed parameters back into the object at the end of a perform routine */
inline void perform_update_wavesetstepper(t_wavesetstepper_tilde *x, t_float step_c, t_float delta_c,
					  t_float o_fac_c, int current_waveset,
					  t_sample current_index, int is_omitted)
{
  x->step_c = step_c;
  x->delta_c = delta_c;
  x->o_fac_c = o_fac_c;
  x->current_waveset = current_waveset;
  x->current_index = current_index;
  x->is_omitted = is_omitted;
}

/* updates all counter variables when a new waveset starts playing */
inline void perform_update_counters(const t_float* step, t_float* step_c, const t_float* delta,
				    t_float* delta_c, const t_float* o_fac, t_float* o_fac_c,
				    int* is_omitted, t_sample* trig_out, int i)
{
  (*step_c) += (*step);
  (*delta_c) += 1;
  
  if((*delta_c) >= (*delta)) {
    // trigger signal if waveset index is reset
    trig_out[i] = 1;
    (*step_c) = 0;
    (*delta_c) = 0;
  }
  
  // omission algorithm
  (*is_omitted) = 0;
  (*o_fac_c) += (*o_fac);
  if (*o_fac_c > 1) {
    (*is_omitted) = 1;
    (*o_fac_c) = (*o_fac_c) - 1;
  }
}

t_int *wavesetstepper_tilde_perform(t_int *w)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)(w[1]);
  const t_sample *in = (t_sample *)(w[2]);
  const t_sample *plb_in = (t_sample *)(w[3]);
  t_sample *out = (t_sample *)(w[4]);
  t_sample *freq_out = (t_sample *)(w[5]);
  t_sample *trig_out = (t_sample *)(w[6]);
  int n = (int)(w[7]), i, maxindex;
  
  if(!x->bufp)
    goto zero;

  t_word *buf = x->bufp->a_vec;
  const t_waveset *waveset_array = x->bufp->waveset_array;
  const int* filter_lookup = x->filter_lookup;
  int lookup_size = x->lookup_size;
  
  if (!buf
      || !waveset_array
      || !lookup_size)
    goto zero;
  
  t_word *wp;
  maxindex = x->bufp->a_vec_length;
  const t_sample one_over_six = 1./6.;
  
  const int* sorted_lookup = x->bufp->sorted_lookup;
    
  int waveset_index = x->current_waveset;
  t_waveset cur_waveset;
  cur_waveset = waveset_array[waveset_index];

  /* getting all values from the wavesetstepper struct */
  t_sample index = x->current_index;
  int is_omitted = x->is_omitted;
  t_float o_fac = (x->o_fac < 0) ? 0 : x->o_fac, o_fac_c = x->o_fac_c,
    delta = x->delta, delta_c = x->delta_c, step = x->step,
    step_c = x->step_c, sr = sys_getsr(), forced_pitch = x->forced_pitch,
    pitch_mix = x->pitch_mix;
  t_sample freq = (1 / (t_sample)cur_waveset.size) * sr;
  t_sample cur_waveset_loudness = cur_waveset.loudest;
  t_sample normalise = (t_sample)x->normalise;
  // flag for the dsp loop to indicate that a new waveset is to be played
  int waveset_finished = 0;
  t_sample index_delta = 0;
  maxindex -= 1;
  
  for (i = 0; i < n; i++) {
    trig_out[i] = 0;
    // in case playing a waveset is finished, a new waveset starts playing
    if(index > cur_waveset.end_index) {
      index_delta = index - (t_sample)cur_waveset.end_index;
      waveset_finished = 1;
	}
    
    if(index < cur_waveset.start_index) {
      index_delta = (t_sample)cur_waveset.start_index - index;
      waveset_finished = 1;
    }
    if(waveset_finished) {
      perform_update_counters(&step, &step_c, &delta, &delta_c, &o_fac, &o_fac_c, &is_omitted, trig_out, i);
      // filtering
      waveset_index = sorted_lookup[filter_lookup[mod((in[i] + step_c), lookup_size)]];
      
      cur_waveset = waveset_array[waveset_index];
      freq = (1 / (t_sample)cur_waveset.size) * sr;
      cur_waveset_loudness = cur_waveset.loudest;
      
      if(plb_in[i] > 0)
	index = cur_waveset.start_index + index_delta;
      else
	index = cur_waveset.end_index - index_delta;
      
      waveset_finished = 0;
    }
    *out++ = four_point_interpolate(buf, wp, index, maxindex, one_over_six)
      * is_omitted
      * ((normalise / cur_waveset_loudness) + (1.0 - normalise));
    *freq_out++ = freq;
    
    index += ((forced_pitch / freq) * copysign(1.0, plb_in[i])) * pitch_mix + plb_in[i] * (1 - pitch_mix);

  }
  perform_update_wavesetstepper(x, step_c, delta_c, o_fac_c, waveset_index, index, is_omitted);
  return (w+8);
  
 zero:
  while (n--) {
    *out++ = 0;
    *trig_out++ = 0;
    *freq_out++ = 0;
  } 
  return (w+8);
}

void wavesetstepper_tilde_bang(t_wavesetstepper_tilde* x)
{
  x->step_c = 0;
  x->delta_c = 0;
}

void wavesetstepper_tilde_set(t_wavesetstepper_tilde *x, t_symbol *s)
{
  /* if a new buffer was specified remove the reference
     in the old buffer and add it to the new */
  reference_pointer rp;
  rp.wavesetstepper = x;
  remove_from_reference_list(rp, wavesetstepper, x->bufp);
  
  t_wavesetbuffer *a = NULL;
  a = (t_wavesetbuffer *)pd_findbyclass((x->buffer_name = s), wavesetbuffer_class);
  if(a) {
    x->bufp = a;
    /* adding the object to the reference list in the bufferobject */
    add_to_reference_list(rp, wavesetstepper, a);
  }
  else {
      pd_error(x, "wavesetstepper~: no bufferobject found");
      x->bufp = NULL;
  }
}


int is_in_filter_range(t_waveset waveset, t_float lower_filt, t_float upper_filt)
{
  int bool = 0;
  if((waveset.filt >= lower_filt) && (waveset.filt <= upper_filt))
    bool = 1;
  return bool;
}

int count_in_filter_range(t_waveset* waveset_array, int num_wavesets,
			  t_float lower_filt, t_float upper_filt)
{
  int counter = 0;
  for(int i = 0; i < num_wavesets; i++) {
    if(is_in_filter_range(waveset_array[i], lower_filt, upper_filt))
      counter++;
  }
  return counter;
}

/* makes the filter_lookup array and saves its size int num_in_filter_range */
int *make_filter_lookup(t_waveset* waveset_array, int num_wavesets, int* num_in_filter_range,
			t_float lower_filt, t_float upper_filt)
{
  (*num_in_filter_range) = count_in_filter_range(waveset_array, num_wavesets, lower_filt, upper_filt);
  int counter = 0;
  int* filter_lookup = (int*)getbytes((*num_in_filter_range) * sizeof(int));
  for(int i = 0; i < num_wavesets; i++) {
    if(is_in_filter_range(waveset_array[i], lower_filt, upper_filt)) {
      filter_lookup[counter] = i;
      counter++;
    }
  }
  return filter_lookup;
}

void wavesetstepper_tilde_filter(t_wavesetstepper_tilde *x, t_floatarg f1, t_floatarg f2)
{
  if(x->bufp) {
    /* frees the lookup if it exists */
    if(x->filter_lookup) {
      freebytes(x->filter_lookup, x->lookup_size * sizeof(int));
    }
    
    t_waveset* waveset_array = x->bufp->waveset_array;
    int num_wavesets = x->bufp->num_wavesets;
    
    t_float upper_filt = f1 >= f2 ? f1 : f2;
    t_float lower_filt = f1 <= f2 ? f1 : f2;
    
    int num_in_filter_range = 0;
    int* filter_lookup = make_filter_lookup(waveset_array, num_wavesets,
					  &num_in_filter_range, lower_filt, upper_filt);
    
    x->filt_1 = lower_filt;
    x->filt_2 = upper_filt;
    x->filter_lookup = filter_lookup;
    x->lookup_size = num_in_filter_range;
  /*
  post("lower_filt: %f\nupper_filt: %f\n", lower_filt, upper_filt);
  for(int i = 0; i < num_in_filter_range; i++) {
    post("i: %d -> index: %d -> filt %f\n", i, filter_lookup[i], waveset_array[filter_lookup[i]].filt);
  }
  */
  }
  else {
    x->filt_1 = f1;
    x->filt_2 = f2;
    x->filter_lookup = NULL;
    x->lookup_size = 0;
  }
}

/*
 * makes all wavesets play at the specified frequency.
 * 0 means no forced pitch.
 * Overwrites the second inlet
 */

void wavesetstepper_tilde_force_pitch(t_wavesetstepper_tilde *x, t_floatarg f1, t_floatarg f2)
{
  x->forced_pitch = f1;
  // clipping the mix factor
  f2 = f2 > 1 ? 1 : f2;
  f2 = f2 < 0 ? 0 : f2;
  x->pitch_mix = f2;
}

void wavesetstepper_tilde_normalise(t_wavesetstepper_tilde *x, t_floatarg f)
{
  // clipping the argument
  f = f > 1 ? 1 : f;
  f = f < 0 ? 0 : f;
  
  x->normalise = f;
}

void wavesetstepper_tilde_dsp(t_wavesetstepper_tilde *x, t_signal **sp)
{
  dsp_add(wavesetstepper_tilde_perform, 7, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}

/* function that is called by wavesetbuffer if the buffer changes */

void update_wavesetstepper_tilde(t_wavesetstepper_tilde *x)
{
  wavesetstepper_tilde_filter(x, x->filt_1, x->filt_2);
}

void wavesetstepper_tilde_free(t_wavesetstepper_tilde *x)
{
  if(x->bufp) {
    reference_pointer rp;
    rp.wavesetstepper = x;
    remove_from_reference_list(rp, wavesetstepper, x->bufp);
  }
  freebytes(x->filter_lookup, x->lookup_size * sizeof(int));
  inlet_free(x->plb_in);
  inlet_free(x->step_in);
  inlet_free(x->delta_in);
  
  outlet_free(x->x_out);
  outlet_free(x->freq_out);
  outlet_free(x->trig_out);
}

void wavesetstepper_tilde_print(t_wavesetstepper_tilde* x)
{
  post("lookup size: %d", x->lookup_size);
  for(int i = 0; i < x->lookup_size; i++) {
    post("%d: %d", i, x->filter_lookup[i]);
  }
}

void *wavesetstepper_tilde_new(t_symbol *s)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)pd_new(wavesetstepper_tilde_class);

  wavesetstepper_tilde_set(x, s);
  
  x->x_f = 0;
  
  x->step = 0;
  x->step_c = 0;
  
  x->delta = 1;
  x->delta_c = 0;
  
  x->o_fac = 1;
  x->o_fac_c = x->o_fac;
  x->is_omitted = 1;

  x->filt_1 = 0;
  x->filt_2 = 1;
  wavesetstepper_tilde_filter(x, 0, 1);

  x->normalise = 0;
  x->forced_pitch = 0;
  x->pitch_mix = 0;

  x->update_fun_pointer = &update_wavesetstepper_tilde;

  x->plb_in = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  x->step_in = floatinlet_new(&x->x_obj, &x->step);
  x->delta_in = floatinlet_new(&x->x_obj, &x->delta);
  x->o_fac_in = floatinlet_new(&x->x_obj, &x->o_fac);
  
  x->x_out = outlet_new(&x->x_obj, &s_signal);
  x->freq_out = outlet_new(&x->x_obj, &s_signal);
  x->trig_out = outlet_new(&x->x_obj, &s_signal);
  
  return (x);
}


void wavesetstepper_tilde_setup(void)
{
  wavesetstepper_tilde_class = class_new(gensym("wavesetstepper~"),
					(t_newmethod)wavesetstepper_tilde_new,
					(t_method)wavesetstepper_tilde_free,
					sizeof(t_wavesetstepper_tilde),
					CLASS_DEFAULT,
				        A_DEFSYMBOL,
					0);
  CLASS_MAINSIGNALIN(wavesetstepper_tilde_class, t_wavesetstepper_tilde, x_f);
  class_addbang(wavesetstepper_tilde_class, wavesetstepper_tilde_bang);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_dsp,
		  gensym("dsp"), A_CANT, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_set,
		  gensym("set"), A_DEFSYMBOL, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_filter,
		  gensym("filter"), A_FLOAT, A_FLOAT, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_print,
		  gensym("print"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_force_pitch,
		  gensym("force_pitch"), A_FLOAT, A_FLOAT, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_normalise,
		  gensym("normalise"), A_FLOAT, 0);
}

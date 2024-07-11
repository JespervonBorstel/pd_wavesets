#include "wavesetbuffer.c"

/*
  wavesetstepper: wavesetplayer with additional Features
     - reads a certain amount of wavesets from an array by increment starting from the waveset
       specified by the index given in the first inlet.
     - waveset omission
 */

t_class *wavesetstepper_tilde_class;

/*
   function for returning the index of the nth next waveset within Filterrange
   nth is is the floor value of the step_counter
   should work circular on the waveset-array wrapping around at the end
   filtering should act as if the array wavesets out of
   range don't exist
   return -1 if no such waveset exists

   - needs adjustment for negative step values
*/

t_int *wavesetstepper_tilde_perform(t_int *w)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)(w[1]);
  const t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_sample *freq_out = (t_sample *)(w[4]);
  t_sample *trig_out = (t_sample *)(w[5]);
  int n = (int)(w[6]), i, maxindex;
  
  if(!x->bufp)
    goto zero;
  
  t_word *buf = x->bufp->a_vec;
  maxindex = x->bufp->a_vec_length;
  const t_waveset *waveset_array = x->bufp->waveset_array;

  const int* filter_lookup = x->filter_lookup;
  int lookup_size = x->lookup_size;

  const int* sorted_lookup = x->bufp->sorted_lookup;
  
  float sr = sys_getsr();
  
  // safety in case no waveset_array exists
  t_waveset cur_waveset;
  if (waveset_array)
    cur_waveset = waveset_array[x->current_waveset];
  
  int index = x->current_index;

  int is_omitted = x->is_omitted;
  t_float o_fac = (x->o_fac < 0) ? 0 : x->o_fac;
  t_float o_fac_c = x->o_fac_c;
  
  if (!buf
      || !waveset_array
      || !lookup_size)
    goto zero;
  maxindex -= 1;

  t_sample freq = (1 / (t_sample)cur_waveset.size) * sr;
  
  for (i = 0; i < n; i++) {
    trig_out[i] = 0;
    // in case playing a waveset is finished, a new waveset starts playing
    if(index > cur_waveset.end_index || index > maxindex) {
      
      x->step_c += x->step;
      x->delta_c += 1;
      int waveset_index;
      
      if(x->delta_c >= x->delta) {

	// trigger signal if waveset index is reset
	trig_out[i] = 1;
	x->step_c = 0;
	x->delta_c = 0;
      }

      // omission algorithm
      is_omitted = 0;
      o_fac_c += o_fac;
      if (o_fac_c > 1) {
        is_omitted = 1;
	o_fac_c = o_fac_c - 1;
      }
      
      // filtering
      waveset_index = sorted_lookup[filter_lookup[mod((in[i] + x->step_c), lookup_size)]];
      
      cur_waveset = waveset_array[waveset_index];
      freq = (1 / (t_sample)cur_waveset.size) * sr;
      x->current_waveset = waveset_index;
      index = cur_waveset.start_index;
    }
    *out++ = buf[index].w_float * is_omitted;
    *freq_out++ = freq;
    index++;
  }
  x->current_index = index;
  x->o_fac_c = o_fac_c;
  x->is_omitted = is_omitted;
  return (w+7);
  
 zero:
  while (n--) {
    *out++ = 0;
    *trig_out++ = 0;
    *freq_out++ = 0;
  } 
  return (w+7);
}

void wavesetstepper_tilde_bang(t_wavesetstepper_tilde* x)
{
  x->step_c = 0;
  x->delta_c = 0;
}

void wavesetstepper_tilde_set(t_wavesetstepper_tilde *x, t_symbol *s)
{
  if(s != x->buffer_name) {
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

void wavesetstepper_tilde_dsp(t_wavesetstepper_tilde *x, t_signal **sp)
{
  dsp_add(wavesetstepper_tilde_perform, 6, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
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

  if (!s)
    pd_error(x, "wavesetstepper~: no buffer name provided");
  t_wavesetbuffer *a;
  a = (t_wavesetbuffer*)pd_findbyclass((x->buffer_name = s), wavesetbuffer_class);

  if(a) {
    x->bufp = a;

    /* adding the object to the reference list in the bufferobject */
    reference_pointer rp;
    rp.wavesetstepper = x;
    add_to_reference_list(rp, wavesetstepper, a);
  }
  else {
    pd_error(x, "wavesetstepper~: no bufferobject found");
    x->bufp = NULL;
  }
   
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

  x->update_fun_pointer = &update_wavesetstepper_tilde;
  
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
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_filter,
		  gensym("print"), A_GIMME, 0);
}

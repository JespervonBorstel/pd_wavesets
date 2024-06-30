#include "wavesets.h"
#include "analyse.h"
#include <math.h>
#include "sort.h"

/*
  wavesetstepper: wavesetplayer with additional Features
     - reads a certain amount of wavesets from an array by increment starting from the waveset
       specified by the index given in the first inlet.
     - waveset omission
 */

static t_class *wavesetstepper_tilde_class;

/*
   function for returning the index of the nth next waveset within Filterrange
   nth is is the floor value of the step_counter
   should work circular on the waveset-array wrapping around at the end
   filtering should act as if the array wavesets out of
   range don't exist
   return -1 if no such waveset exists

   - needs adjustment for negative step values
*/

// implementation of the positive mod
int mod(int i, int n)
{
    return (i % n + n) % n;
}

int get_filtered_waveset_index (t_float upper_filt, t_float lower_filt, t_float step_c, t_float step,
				t_sample in_val, const t_waveset* waveset_array, int num_wavesets,
				const int sorted_lookup[])
{
  in_val = (int)floor(in_val);
  step_c = (int)floor(step_c);
  int matching_wavesets = 1;

  // iterates in the other direction if our step_increment is negative
  if(step > 0) {
    /*  - the while loop is needed in case a matching waveset was found
        but step_c didn`t reach zero in one loop over the waveset array */
    while(matching_wavesets) {
      matching_wavesets = 0;
      for(int i = in_val; i < (in_val + num_wavesets); i++) {
	int true_index = sorted_lookup[mod(i, num_wavesets)];
	t_float float_val = waveset_array[true_index].filt;
	if ((float_val >= lower_filt) && (float_val <= upper_filt) && (step_c <= 0)) {
	  return true_index;
	}
	if ((float_val >= lower_filt) && (float_val <= upper_filt) && (step_c > 0)) {
	  step_c -= 1;
	  matching_wavesets += 1;
	}
      }
      /* shortens the loop to avoid unnessesary iteration
	 so that the while loop while take two iteratons max */
      if(matching_wavesets)
	step_c = mod(step_c, matching_wavesets);
    }
  }
  else {
    while(matching_wavesets) {
      matching_wavesets = 0;
      for(int i = in_val; i > (in_val - num_wavesets); i--) {
	int true_index = sorted_lookup[mod(i, num_wavesets)];
	t_float float_val = waveset_array[true_index].filt;
	if ((float_val >= lower_filt) && (float_val <= upper_filt) && (step_c >= 0)) {
	  return true_index;
	}
	if ((float_val >= lower_filt) && (float_val <= upper_filt) && (step_c < 0)) {
	  step_c += 1;
	  matching_wavesets += 1;
	}
      }
      if(matching_wavesets)
	step_c = mod(step_c, (-1 * matching_wavesets));
    }
  }
  return -1;
}

t_int *wavesetstepper_tilde_perform(t_int *w)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)(w[1]);
  const t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_sample *freq_out = (t_sample *)(w[4]);
  t_sample *trig_out = (t_sample *)(w[5]);
  int n = (int)(w[6]), i, maxindex;
  t_word *buf;
  const t_waveset *waveset_array = x->waveset_array;

  t_float filt1 = x->filt_1;
  t_float filt2 = x->filt_2;
  t_float upper_filt = filt1 >= filt2 ? filt1 : filt2;
  t_float lower_filt = filt1 <= filt2 ? filt1 : filt2;

  float sr = sys_getsr();
  
  // safety in case no waveset_array exists
  t_waveset cur_waveset;
  if (x->waveset_array)
    cur_waveset = waveset_array[x->current_waveset];
  
  int index = x->current_index;
  int num_wavesets = x->num_wavesets;

  int is_omitted = x->is_omitted;
  t_float o_fac = (x->o_fac < 0) ? 0 : x->o_fac;
  t_float o_fac_c = x->o_fac_c;
  
  if (!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0)
      || x->num_wavesets == 0
      || !waveset_array)
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
      waveset_index = get_filtered_waveset_index(upper_filt, lower_filt, x->step_c, x->step,
						 in[i], waveset_array, num_wavesets, x->sorted_lookup);

      // check if a waveset within filter range has been found
      if(waveset_index < 0) {
	waveset_index = in[i];
	is_omitted = 0;
      }
      
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

/* Methods for sorting and unsorting the wavesettable */

void wavesetstepper_tilde_reset(t_wavesetstepper_tilde* x)
{
  if(x->sorted) {
    int* sorted_lookup = x->sorted_lookup;
    for(int i = 0; i < x->num_wavesets; i++) {
      sorted_lookup[i] = i;
    }
    x->sorted = 0;
  }
}

void wavesetstepper_tilde_sort(t_wavesetstepper_tilde* x)
{
  if(!x->sorted) {
    x->sorted = 1;
    wavesetstepper_tilde_reset(x);
    qsort_wavesets(x->sorted_lookup, 0, x->num_wavesets - 1, x->waveset_array);
    x->sorted = 1;
  }
}

void wavesetstepper_tilde_bang(t_wavesetstepper_tilde* x)
{
  x->step_c = 0;
  x->delta_c = 0;
}

void wavesetstepper_tilde_print(t_wavesetstepper_tilde* x)
{
  if(x->num_wavesets == 0) {
    post("waveset_array empty");
  }
  else {
    post("number of wavesets: %d", x->num_wavesets);
    for(int i = 0; i < x->num_wavesets; i++) {
      t_waveset waveset = x->waveset_array[i];
      post("i : %d", i);
      post("size : %d", waveset.size);
      post("start : %d", waveset.start_index);
      post("end : %d", waveset.end_index);
      post("filter value: %f\n", waveset.filt);
    }
  }
}

void wavesetstepper_tilde_size(t_wavesetstepper_tilde* x)
{
  outlet_float(x->f_out, x->num_wavesets);
}

void wavesetstepper_tilde_set(t_wavesetstepper_tilde *x, t_symbol *s,
    int argc, t_atom *argv)
{
    arrayvec_set(&x->x_v, argc, argv);
    int maxindex;
    t_word* buf;
    
    if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 1))
      pd_error(x, "wavesetstepper~: unable to read array");
    freebytes(x->sorted_lookup, x->num_wavesets * sizeof(int));
    analyse_wavesets_stepper(x, buf, maxindex);
    x->current_waveset = 0;
    x->current_index = 0;
    x->sorted_lookup = (int*)getbytes(x->num_wavesets * sizeof(int));
    
    if(x->sorted) {
      x->sorted = 0;
      wavesetstepper_tilde_sort(x);
    }
    else {
      x->sorted = 1;
      wavesetstepper_tilde_reset(x);
    }
}

void wavesetstepper_tilde_dsp(t_wavesetstepper_tilde *x, t_signal **sp)
{
  arrayvec_testvec(&x->x_v);  
  dsp_add(wavesetstepper_tilde_perform, 6, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

void wavesetstepper_tilde_free(t_wavesetstepper_tilde *x)
{
    arrayvec_free(&x->x_v);
    free_wavesets_stepper(x);

    inlet_free(x->step_in);
    inlet_free(x->delta_in);
    inlet_free(x->filt1_in);
    inlet_free(x->filt2_in);

    outlet_free(x->x_out);
    outlet_free(x->freq_out);
    outlet_free(x->f_out);
    outlet_free(x->trig_out);
}

void *wavesetstepper_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)pd_new(wavesetstepper_tilde_class);

  arrayvec_init(&x->x_v, x, argc, argv);
  t_word* buf;
  int maxindex;
  
  if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 1)) {
    pd_error(x, "wavesetstepper~: unable to read array");
    x->waveset_array = NULL;
    x->num_wavesets = 0;
    x->current_waveset = 0;
    x->current_index = 0;
  }
  
  else {
    analyse_wavesets_stepper(x, buf, maxindex);
    x->current_waveset = 0;
    x->current_index = x->waveset_array[0].start_index;
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

  x->sorted = 1;
  x->sorted_lookup = (int*)getbytes(x->num_wavesets * sizeof(int));
  wavesetstepper_tilde_reset(x);
  
  x->step_in = floatinlet_new(&x->x_obj, &x->step);
  x->delta_in = floatinlet_new(&x->x_obj, &x->delta);
  x->o_fac_in = floatinlet_new(&x->x_obj, &x->o_fac);
  x->filt1_in = floatinlet_new(&x->x_obj, &x->filt_1);
  x->filt2_in = floatinlet_new(&x->x_obj, &x->filt_2);
  
  x->x_out = outlet_new(&x->x_obj, &s_signal);
  x->freq_out = outlet_new(&x->x_obj, &s_signal);  
  x->f_out = outlet_new(&x->x_obj, &s_float);
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
					A_GIMME,
					0);
  CLASS_MAINSIGNALIN(wavesetstepper_tilde_class, t_wavesetstepper_tilde, x_f);
  class_addbang(wavesetstepper_tilde_class, wavesetstepper_tilde_bang);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_dsp,
		  gensym("dsp"), A_CANT, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_set,
		  gensym("set"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_print,
		  gensym("print"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_size,
		  gensym("size"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_sort,
		  gensym("sort"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_reset,
		  gensym("reset"), A_GIMME, 0);
}

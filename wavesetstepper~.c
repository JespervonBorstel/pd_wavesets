#include "wavesets.h"
#include "analyse.h"

/*
  wavesetstepper: wavesetplayer with additional Features
     - reads a certain amount of wavesets from an array by increment starting from the waveset
       specified by the index given in the first inlet.
     - waveset omission

 */

static t_class *wavesetstepper_tilde_class;

void *wavesetstepper_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)pd_new(wavesetstepper_tilde_class);
  arrayvec_init(&x->x_v, x, argc, argv);
  arrayvec_testvec(&x->x_v);
  
  t_word* buf;
  int maxindex;
  if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0))
    pd_error(x, "wavesetstepper~: couldn't read array");
  analyse_wavesets_stepper(x, buf, maxindex);
  x->current_waveset = 0;
  x->current_index = x->waveset_array[0].start_index;
  
  x->x_f = 0;
  
  x->step = 0;
  x->step_c = 0;
  
  x->delta = 1;
  x->delta_c = 0;
  
  x->o_fac = 0;
  x->o_fac_c = x->o_fac;
  x->is_omitted = 0;
  
  x->step_in = floatinlet_new(&x->x_obj, &x->step);
  x->delta_in = floatinlet_new(&x->x_obj, &x->delta);
  x->o_fac_in = floatinlet_new(&x->x_obj, &x->o_fac);
  
  x->x_out = outlet_new(&x->x_obj, &s_signal);
  x->f_out = outlet_new(&x->x_obj, &s_float);
  x->trig_out = outlet_new(&x->x_obj, &s_signal);
  return (x);
}

t_int *wavesetstepper_tilde_perform(t_int *w)
{
  t_wavesetstepper_tilde *x = (t_wavesetstepper_tilde *)(w[1]);
  t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_sample *trig_out = (t_sample *)(w[4]);
  int n = (int)(w[5]), i, maxindex;
  t_word *buf;
  
  t_waveset cur_waveset = x->waveset_array[x->current_waveset];
  int index = x->current_index;
  int num_wavesets = x->num_wavesets;

  int is_omitted = x->is_omitted;
  t_float o_fac = (x->o_fac < 0) ? 0 : x->o_fac;
  t_float o_fac_c = x->o_fac_c;
  
  if (!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0) || x->num_wavesets == 0)
    goto zero;
  maxindex -= 1;
  
  for (i = 0; i < n; i++) {
    trig_out[i] = (index == cur_waveset.end_index && x->delta_c > x->delta) ? 1 : 0;
    // in case playing a waveset is finished, a new waveset starts playing
    if(index > cur_waveset.end_index || index > maxindex) {
      x->step_c += x->step;
      x->delta_c += 1;
      int waveset_index;
      
      if(x->delta_c >= x->delta) {
	x->step_c = 0;
	x->delta_c = 0;
      }

      waveset_index = in[i] + floor(x->step_c);

      // omission algorithm
      is_omitted = 0;
      o_fac_c += o_fac;
      if (o_fac_c > 1) {
        is_omitted = 1;
	o_fac_c = o_fac_c - 1;
      }
      
      // clip the waveset_index
      waveset_index = (waveset_index >= num_wavesets) ? num_wavesets - 1 : waveset_index;
      waveset_index = (waveset_index < 0) ? 0 : waveset_index;
      
      cur_waveset = x->waveset_array[waveset_index];
      x->current_waveset = waveset_index;
      index = cur_waveset.start_index;
    }
    *out++ = buf[index].w_float * is_omitted;
    index++;
  }
  x->current_index = index;
  x->o_fac_c = o_fac_c;
  x->is_omitted = is_omitted;
  return (w+6);
  
 zero:
  while (n--) {
    *out++ = 0;
    *trig_out++ = 0;
  } 
  return (w+6);
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
      post("end : %d\n", waveset.end_index);
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
    
    if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0))
      pd_error(x, "wavesetstepper~: couldn't read array");
    analyse_wavesets_stepper(x, buf, maxindex);
    x->current_waveset = 0;
    x->current_index = 0;
}

void wavesetstepper_tilde_dsp(t_wavesetstepper_tilde *x, t_signal **sp)
{
     dsp_add(wavesetstepper_tilde_perform, 5, x,
	     sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void wavesetstepper_tilde_free(t_wavesetstepper_tilde *x)
{
    arrayvec_free(&x->x_v);
    free_wavesets_stepper(x);

    inlet_free(x->step_in);
    inlet_free(x->delta_in);
    outlet_free(x->x_out);
    outlet_free(x->f_out);
    outlet_free(x->trig_out);
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
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_dsp,
		  gensym("dsp"), A_CANT, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_set,
		  gensym("set"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_print,
		  gensym("print"), A_GIMME, 0);
  class_addmethod(wavesetstepper_tilde_class, (t_method)wavesetstepper_tilde_size,
		  gensym("size"), A_GIMME, 0);
}

#include "wavesets.h"
#include "analyse.h"

/*
  wavesetstepper: wavesetplayer with additional Features
     - reads a certain amount of wavesets from an array by increment starting from the waveset
       specified by the index given in the first inlet.
     - waveset omission
 */

static t_class *wavesetplayer_tilde_class;

void *wavesetplayer_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_wavesetplayer_tilde *x = (t_wavesetplayer_tilde *)pd_new(wavesetplayer_tilde_class);

  arrayvec_init(&x->x_v, x, argc, argv);
  t_word* buf;
  int maxindex;
  
  if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 1)) {
    pd_error(x, "wavesetplayer~: unable to read array");
    x->waveset_array = NULL;
    x->num_wavesets = 0;
    x->current_waveset = 0;
    x->current_index = 0;
  }
  
  else {
    analyse_wavesets_player(x, buf, maxindex);
    x->current_waveset = 0;
    x->current_index = x->waveset_array[0].start_index;
  }
  
  x->x_f = 0;
  
  x->x_out = outlet_new(&x->x_obj, &s_signal);
  x->f_out = outlet_new(&x->x_obj, &s_float);
  x->trig_out = outlet_new(&x->x_obj, &s_signal);
  
  return (x);
}

t_int *wavesetplayer_tilde_perform(t_int *w)
{
  t_wavesetplayer_tilde *x = (t_wavesetplayer_tilde *)(w[1]);
  const t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  t_sample *trig_out = (t_sample *)(w[4]);
  int n = (int)(w[5]), i, maxindex;
  t_word *buf;
  const t_waveset *waveset_array = x->waveset_array;

  // safety in case no waveset_array exists
  t_waveset cur_waveset;
  if (x->waveset_array)
    cur_waveset = waveset_array[x->current_waveset];
  
  int index = x->current_index;
  int num_wavesets = x->num_wavesets;
  
  if (!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0)
      || !waveset_array)
    goto zero;
  maxindex -= 1;
  
  for (i = 0; i < n; i++) {
    trig_out[i] = 0;
    // in case playing a waveset is finished, a new waveset starts playing
    if(index > cur_waveset.end_index || index > maxindex) {
      
      int waveset_index;

      // waveset_index safety
      waveset_index = mod((int)in[i], num_wavesets);
      
      cur_waveset = waveset_array[waveset_index];
      x->current_waveset = waveset_index;
      index = cur_waveset.start_index;
    }
    *out++ = buf[index].w_float;
    index++;
  }
  x->current_index = index;
  return (w+6);
  
 zero:
  while (n--) {
    *out++ = 0;
    *trig_out++ = 0;
  } 
  return (w+6);
}

void wavesetplayer_tilde_print(t_wavesetplayer_tilde* x)
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

void wavesetplayer_tilde_size(t_wavesetplayer_tilde* x)
{
  outlet_float(x->f_out, x->num_wavesets);
}

void wavesetplayer_tilde_set(t_wavesetplayer_tilde *x, t_symbol *s,
    int argc, t_atom *argv)
{
    arrayvec_set(&x->x_v, argc, argv);
    int maxindex;
    t_word* buf;
    
    if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 1))
      pd_error(x, "wavesetplayer~: unable to read array");
    analyse_wavesets_player(x, buf, maxindex);
    x->current_waveset = 0;
    x->current_index = 0;
}

void wavesetplayer_tilde_dsp(t_wavesetplayer_tilde *x, t_signal **sp)
{
  arrayvec_testvec(&x->x_v);  
  dsp_add(wavesetplayer_tilde_perform, 5, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void wavesetplayer_tilde_free(t_wavesetplayer_tilde *x)
{
    arrayvec_free(&x->x_v);
    free_wavesets_player(x);

    outlet_free(x->x_out);
    outlet_free(x->f_out);
    outlet_free(x->trig_out);
}

void wavesetplayer_tilde_setup(void)
{
  wavesetplayer_tilde_class = class_new(gensym("wavesetplayer~"),
					(t_newmethod)wavesetplayer_tilde_new,
					(t_method)wavesetplayer_tilde_free,
					sizeof(t_wavesetplayer_tilde),
					CLASS_DEFAULT,
					A_DEFSYMBOL,
					0);
  CLASS_MAINSIGNALIN(wavesetplayer_tilde_class, t_wavesetplayer_tilde, x_f);
  class_addmethod(wavesetplayer_tilde_class, (t_method)wavesetplayer_tilde_dsp,
		  gensym("dsp"), A_CANT, 0);
  class_addmethod(wavesetplayer_tilde_class, (t_method)wavesetplayer_tilde_set,
		  gensym("set"), A_GIMME, 0);
  class_addmethod(wavesetplayer_tilde_class, (t_method)wavesetplayer_tilde_print,
		  gensym("print"), A_GIMME, 0);
  class_addmethod(wavesetplayer_tilde_class, (t_method)wavesetplayer_tilde_size,
		  gensym("size"), A_GIMME, 0);
}

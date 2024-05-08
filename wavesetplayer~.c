#include "wavesets.h"
#include "analyse.h"

/* ---------- basic prototype that plays wavesets based on an index  ---------- */

static t_class *wavesetplayer_tilde_class;

void *wavesetplayer_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
  t_wavesetplayer_tilde *x = (t_wavesetplayer_tilde *)pd_new(wavesetplayer_tilde_class);
  arrayvec_init(&x->x_v, x, argc, argv);
  arrayvec_testvec(&x->x_v);
  
  t_word* buf;
  int maxindex;
  if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0))
    pd_error(x, "wavesetplayer~: couldn't read array");
  analyse_wavesets(x, buf, maxindex);
  x->current_waveset = 0;
  x->current_index = x->waveset_array[0].start_index;
  
  x->x_f = 0;
  x->x_out = outlet_new(&x->x_obj, &s_signal);
  x->f_out = outlet_new(&x->x_obj, &s_float);
  x->b_out = outlet_new(&x->x_obj, &s_bang);
  return (x);
}

t_int *wavesetplayer_tilde_perform(t_int *w)
{
  t_wavesetplayer_tilde *x = (t_wavesetplayer_tilde *)(w[1]);
  t_sample *in = (t_sample *)(w[2]);
  t_sample *out = (t_sample *)(w[3]);
  int n = (int)(w[4]), i, maxindex;
  t_word *buf;
  
  t_waveset cur_waveset = x->waveset_array[x->current_waveset];
  int index = x->current_index;
  int num_wavesets = x->num_wavesets;
  
  if (!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0) || x->num_wavesets == 0)
    goto zero;
  maxindex -= 1;

  for (i = 0; i < n; i++) {
    outlet_bang(x->b_out);
    // in case playing a waveset is finished, a new waveset starts playing
    if(index > cur_waveset.end_index || index > maxindex) {
      
      int waveset_index = in[i];
      // clip the waveset_index
      waveset_index = (waveset_index >= num_wavesets) ? num_wavesets - 1 : waveset_index;
      waveset_index = (waveset_index < 0) ? 0 : waveset_index;
      cur_waveset = x->waveset_array[waveset_index];
      x->current_waveset = waveset_index;
      index = cur_waveset.start_index;
    }
    *out++ = buf[index].w_float;
    index++;
  }
  x->current_index = index;
  //post("current waveset: %d", x->current_waveset);
  //post("%d", (int)in[0]);
  return (w+5);

 zero:
  while (n--) *out++ = 0;
  return (w+5);
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
      post("end : %d\n", waveset.end_index);
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
    
    if(!dsparray_get_array(x->x_v.v_vec, &maxindex, &buf, 0))
      pd_error(x, "wavesetplayer~: couldn't read array");
    analyse_wavesets(x, buf, maxindex);
    x->current_waveset = 0;
    x->current_index = 0;
}

void wavesetplayer_tilde_dsp(t_wavesetplayer_tilde *x, t_signal **sp)
{
     dsp_add(wavesetplayer_tilde_perform, 4, x,
	     sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void wavesetplayer_tilde_free(t_wavesetplayer_tilde *x)
{
    arrayvec_free(&x->x_v);
    free_wavesets(x);
    outlet_free(x->x_out);
    outlet_free(x->f_out);
    outlet_free(x->b_out);
}

void wavesetplayer_tilde_setup(void)
{
  wavesetplayer_tilde_class = class_new(gensym("wavesetplayer~"),
					(t_newmethod)wavesetplayer_tilde_new,
					(t_method)wavesetplayer_tilde_free,
					sizeof(t_wavesetplayer_tilde),
					CLASS_DEFAULT,
					A_GIMME,
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

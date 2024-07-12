#include "wavesets.h"
#include "utils.h"
#include "classes.h"

void update_wavesetplayer_tilde()
{
}

void *wavesetplayer_tilde_new(t_symbol *s)
{
  t_wavesetplayer_tilde *x = (t_wavesetplayer_tilde *)pd_new(wavesetplayer_tilde_class);

  if (!s)
    pd_error(x, "wavesetplayer~: no buffer name provided");
  t_wavesetbuffer *a;
  a = (t_wavesetbuffer*)pd_findbyclass((x->buffer_name = s), wavesetbuffer_class);
  
  if(a) {
    x->bufp = a;
    
    /* adding the object to the reference list in the bufferobject */
    reference_pointer rp;
    rp.wavesetplayer = x;
    add_to_reference_list(rp, wavesetplayer, a);
  }
  else {
    pd_error(x, "wavesetplayer~: no bufferobject found");
    x->bufp = NULL;
  }
  
  x->x_f = 0;
  
  x->update_fun_pointer = &update_wavesetplayer_tilde;
  
  x->x_out = outlet_new(&x->x_obj, &s_signal);
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

  if(!x->bufp)
    goto zero;

  t_word *buf = x->bufp->a_vec;
  maxindex = x->bufp->a_vec_length;
  const t_waveset *waveset_array = x->bufp->waveset_array;

  // safety in case no waveset_array exists
  t_waveset cur_waveset;
  if (waveset_array)
    cur_waveset = waveset_array[x->current_waveset];
  
  int index = x->current_index;
  int num_wavesets = x->bufp->num_wavesets;
  
  if (!buf
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

void wavesetplayer_tilde_set(t_wavesetplayer_tilde *x, t_symbol *s)
{
  if(s != x->buffer_name) {
    /* if a new buffer was specified remove the reference
       in the old buffer and add it to the new */
    reference_pointer rp;
    rp.wavesetplayer = x;
    remove_from_reference_list(rp, wavesetplayer, x->bufp);
    
    t_wavesetbuffer *a = NULL;
    a = (t_wavesetbuffer *)pd_findbyclass((x->buffer_name = s), wavesetbuffer_class);
    if(a) {
      x->bufp = a;
      /* adding the object to the reference list in the bufferobject */
      add_to_reference_list(rp, wavesetplayer, a);
    }
    else {
      pd_error(x, "wavesetplayer ~: no bufferobject found");
      x->bufp = NULL;
    }
  }
}

void wavesetplayer_tilde_dsp(t_wavesetplayer_tilde *x, t_signal **sp)
{
  dsp_add(wavesetplayer_tilde_perform, 5, x,
	  sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void wavesetplayer_tilde_free(t_wavesetplayer_tilde *x)
{
  if(x->bufp) {
    reference_pointer rp;
    rp.wavesetplayer = x;
    remove_from_reference_list(rp, wavesetplayer, x->bufp);
  }
  outlet_free(x->x_out);
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
		  gensym("set"), A_DEFSYMBOL, 0);
}

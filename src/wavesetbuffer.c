#include "wavesets.h"
#include "utils.h"
#include "analyse.h"
#include "classes.h"

/*
 * wavesetbuffer: object that stores and analyses soundfile arrays
 *   to hold the data that other waveset-dsp-objects reference 
 *   when implementing a new object class, it needs to be added to the free_all_reference and
 *   update_all_references
 */

/*
 * TODO:
 * make everything compile into one library file
 * create helpfiles for all objects and make them work
 * give build instructions on github in the Readme.md
 *
 * Maybe next: wavesetexchanger~: nimmt wellenform auf anderem Array und tauscht damit einzelne Wavesets aus
 */

void free_all_references(const t_ref_list* ref_listp)
{
  t_node* current = ref_listp->head;
  while(current) {
    switch(current->type) {
    case wavesetstepper: {
      t_wavesetstepper_tilde* objp = current->object_pointer.wavesetstepper;
      objp->bufp = NULL;
      break;
    }
    case wavesetplayer: {
      t_wavesetplayer_tilde* objp = current->object_pointer.wavesetplayer;
      objp->bufp = NULL;
      break;
    }
    case wavesetclouds: {
      t_wavesetclouds_tilde* objp = current->object_pointer.wavesetclouds;
      objp->bufp = NULL;
      break;
    }
    case wavesetjitter: {
      t_wavesetjitter_tilde* objp = current->object_pointer.wavesetjitter;
      objp->bufp = NULL;
      break;
    }
    }
    current = current->next;
  }
}

/* set all pointers to zero after freeing */ 
void wavesetbuffer_free(t_wavesetbuffer *x)
{
  freebytes(x->a_vec, x->a_vec_length * sizeof(t_word));
  freebytes(x->sorted_lookup, x->num_wavesets * sizeof(int));
  free_all_references(x->reference_listp);
  free_ref_list(x->reference_listp);
  freebytes(x->waveset_array, x->num_wavesets * sizeof(t_waveset));
  pd_unbind(&x->x_obj.ob_pd, x->buffer_name);
}

void wavesetbuffer_print(t_wavesetbuffer *x)
{  
  post("object name: %s\n", x->buffer_name->s_name);
  post("array name: %s\n", x->array_name->s_name);
  
  if(x->num_wavesets == 0) {
    post("waveset array empty");
  }
  else {
    post("number of wavesets: %d", x->num_wavesets);
    for(int i = 0; i < x->num_wavesets; i++) {
    t_waveset waveset = x->waveset_array[i];
    /*   post("i : %d", i); */
    post("size : %d", waveset.size);
    /*   post("start : %d", waveset.start_index); */
    /*   post("end : %d", waveset.end_index); */
    /*   post("filter value: %f\n", waveset.filt); */
    post("loudest: %f\n", waveset.loudest);
    }
  }
  // getting loudest and quietest sample
  t_sample highest = 0;
  t_sample lowest = 0;
  for (int i = 0; i < x->a_vec_length; i++) {
    t_sample samp = x->a_vec[i].w_float;
    if (samp < lowest) lowest = samp;
    if (samp > highest) highest = samp;
  }
  post("highest sample value in a_vec: %f\n", highest);
  post("lowest sample value in a_vec: %f\n", lowest);
  
  // getting loudest and quietest waveset
  highest = 0;
  lowest = 10000000;
  for (int i = 0; i < x->num_wavesets; i++) {
    t_sample cur_val = x->waveset_array[i].loudest;
    if (cur_val < lowest) lowest = cur_val;
    if (cur_val > highest) highest = cur_val;
  }
  post("loudest waveset: %f\n", highest);
  post("most quiet waveset: %f\n", lowest);


  
  print_reference_list(x->reference_listp);
}

/* Methods for sorting and unsorting the wavesettable */

void wavesetbuffer_reset(t_wavesetbuffer* x)
{
  if(x->sorted) {
    int* sorted_lookup = x->sorted_lookup;
    for(int i = 0; i < x->num_wavesets; i++) {
      sorted_lookup[i] = i;
    }
    x->sorted = 0;
  }
}

void wavesetbuffer_sort(t_wavesetbuffer* x)
{
  if(!x->sorted) {
    x->sorted = 1;
    wavesetbuffer_reset(x);
    qsort_wavesets(x->sorted_lookup, 0, x->num_wavesets - 1, x->waveset_array);
    x->sorted = 1;
  }
}

/*
 * saves a copy of the contents of the specified array into the object,
 * that is only refreshed when the set method is called (not at every dsp-cycle)!
 */

void update_all_references(const t_ref_list* reference_listp)
{
  t_node* current = reference_listp->head;
  while(current) {
    switch(current->type) {
    case wavesetstepper: {
      t_wavesetstepper_tilde* objp = current->object_pointer.wavesetstepper;
      objp->update_fun_pointer(objp);
      break;
    }
    case wavesetplayer: {
      t_wavesetplayer_tilde* objp = current->object_pointer.wavesetplayer;
      objp->update_fun_pointer(objp);
      break;
    }
    case wavesetclouds: {
      t_wavesetclouds_tilde* objp = current->object_pointer.wavesetclouds;
      objp->update_fun_pointer(objp);
      break;
    }
    case wavesetjitter: {
      t_wavesetjitter_tilde* objp = current->object_pointer.wavesetjitter;
      objp->update_fun_pointer(objp);
      break;
    }
    }
    current = current->next;
  }
}

void wavesetbuffer_size(t_wavesetbuffer* x)
{
  outlet_float(x->f_out, x->num_wavesets);
}

void wavesetbuffer_set(t_wavesetbuffer *x, t_symbol *array_name)
{
  t_garray *a;
  t_word* buf;

  freebytes(x->a_vec, x->a_vec_length * sizeof(t_word));
  freebytes(x->sorted_lookup, x->num_wavesets * sizeof(int));
  freebytes(x->waveset_array, x->num_wavesets * sizeof(t_waveset));

  x->waveset_array = NULL;
  x->a_vec = NULL;
  x->sorted_lookup = NULL;
  /* finding the garray object and saving its contents into buf */
  x->array_name = array_name;
  if (!(a = (t_garray *)pd_findbyclass(array_name, garray_class))) {
    if (*array_name->s_name) {
      pd_error(x, "wavesetbuffer: %s: no such array", x->array_name->s_name);
    }
    buf = NULL;
  } else if (!garray_getfloatwords(a, &x->a_vec_length, &buf)) {
    pd_error(x, "%s: bad template for wavesetbuffer", x->array_name->s_name);
    buf = NULL;
  }
  
  /* refreshing the copy of the array */
  x->a_vec = (t_word*)getbytes(x->a_vec_length * sizeof(t_word));
  for(int i = 0; i < x->a_vec_length; i++)
    x->a_vec[i] = buf[i];

  /* analising the samplevector */
  if(!x->a_vec)  {
    pd_error(x, "wavesetbuffer: unable to read array");
    x->waveset_array = NULL;
    x->num_wavesets = 0;
  }
  else {
    analyse_wavesets(x, x->a_vec, x->a_vec_length);
  }

  /* sorting the wavesetarray */
  x->sorted_lookup = (int*)getbytes(x->num_wavesets * sizeof(int));  
  if(x->sorted) {
    x->sorted = 0;
    wavesetbuffer_sort(x);
  }
  else {
    x->sorted = 1;
    wavesetbuffer_reset(x);
  }
  
  update_all_references(x->reference_listp);
  outlet_float(x->f_out, x->num_wavesets);
}

void *wavesetbuffer_new(t_symbol *s1, t_symbol *s2)
{
  t_wavesetbuffer *x = (t_wavesetbuffer *)pd_new(wavesetbuffer_class);

  if(!s1)
    pd_error(x, "please provide a buffer name");

  x->sorted = 0;
  x->sorted_lookup = NULL;

  x->reference_listp = new_ref_list();
 
  x->a_vec_length = 0;
  x->a_vec = NULL;
  x->num_wavesets = 0;
  x->waveset_array = NULL;
  x->buffer_name = s1;

  /* binding the name of the buffer object so that
     it can be found by pd_findbyclass in wavesetstepper~ */
  pd_bind(&x->x_obj.ob_pd, s1);
  
  x->f_out = outlet_new(&x->x_obj, &s_float);
  
  wavesetbuffer_set(x, s2);    
  
  return (x);
}

void wavesetbuffer_setup(void)
{
  wavesetbuffer_class = class_new(gensym("wavesetbuffer"),
				  (t_newmethod)wavesetbuffer_new,
				  (t_method)wavesetbuffer_free,
				  sizeof(t_wavesetbuffer),
				  CLASS_DEFAULT,
				  A_DEFSYMBOL,
				  A_DEFSYMBOL,
				  0);
    class_addmethod(wavesetbuffer_class, (t_method)wavesetbuffer_print,
		  gensym("print"), A_GIMME, 0);
    class_addmethod(wavesetbuffer_class, (t_method)wavesetbuffer_set,
		    gensym("set"), A_DEFSYMBOL, 0);
    class_addmethod(wavesetbuffer_class, (t_method)wavesetbuffer_sort,
		    gensym("sort"), A_DEFSYMBOL, 0);
    class_addmethod(wavesetbuffer_class, (t_method)wavesetbuffer_reset,
		    gensym("reset"), A_DEFSYMBOL, 0);
    class_addmethod(wavesetbuffer_class, (t_method)wavesetbuffer_size,
		    gensym("size"), A_DEFSYMBOL, 0);
}

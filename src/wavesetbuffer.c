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
 * TODO: 1. create wavesetbuffer-object (done)
 *       2. adjust wavesetstepper object to work with it (done)
 *       3. Fixing the unbind error message when creating the object
 *       4. Testing if wavesetstepper~ still works (done)
 *       5. making wavesetplayer~ work with it
 *       6. implementing wavesetfiltering in wavesetstepper at k-rate
 *         - this will need a way of communicating between buffer and dsp object
 *           in case the buffer is changed
 *         - implementation:
 *           - have a list of pointers to all dsp-objects referencing the buffer
 *           - if the buffer is set, call an update-function that works on every dsp-object in the linked list
 *           - if the dsp-object changes the buffer it references,
 *             the buffer needs to remove the pointer out of the list, and add it to the new buffer (done)
 *       7. cleaning up
 *       8. implementing playbackspeed with 4 point interpolation and a force frequency method
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
    }
    current = current->next;
  }
}

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
  
  //post("object name: %s\n", x->buffer_name->s_name);
  //post("array name: %s\n", x->array_name->s_name);
  //for(int i = 0; i < x->a_vec_length; i++)
  //  post("%f", x->a_vec[i].w_float);
  
  if(x->num_wavesets == 0) {
    post("waveset_array empty");
  }
  /*  else {
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
  */
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
    }
    current = current->next;
  }
}
  
void wavesetbuffer_set(t_wavesetbuffer *x, t_symbol *array_name)
{
  t_garray *a;
  t_word* buf;

  freebytes(x->a_vec, x->a_vec_length * sizeof(t_word));
  freebytes(x->sorted_lookup, x->num_wavesets * sizeof(int));

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
  } else {
    garray_usedindsp(a);
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
  else
    analyse_wavesets(x, x->a_vec, x->a_vec_length);

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
  x->array_name = array_name;
  update_all_references(x->reference_listp);
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
  
  wavesetbuffer_set(x, s2);    
  
  x->f_out = outlet_new(&x->x_obj, &s_float);
  
  return (x);
}

void wavesetbuffer_size(t_wavesetbuffer* x)
{
  outlet_float(x->f_out, x->num_wavesets);
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

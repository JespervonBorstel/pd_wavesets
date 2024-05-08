#include <stdio.h>
#include "m_pd.h"
#include <puredata/g_canvas.h>


typedef struct _dsparray
{
    t_symbol *d_symbol;
    t_gpointer d_gp;
    int d_phase;    /* used for tabwrite~ and tabplay~ */
    void *d_owner;  /* for pd_error() */
} t_dsparray;

typedef struct _arrayvec
{
    int v_n;
    t_dsparray *v_vec;
} t_arrayvec;

    /* LATER consider exporting this and using it for tabosc4~ too */
static int dsparray_get_array(t_dsparray *d, int *npoints, t_word **vec,
    int recover)
{
    t_garray *a;

    if (gpointer_check(&d->d_gp, 0))
    {
        *vec = (t_word *)d->d_gp.gp_stub->gs_un.gs_array->a_vec;
        *npoints = d->d_gp.gp_stub->gs_un.gs_array->a_n;
        return 1;
    }
    else if (recover || d->d_gp.gp_stub)
        /* when the pointer is invalid: if "recover" is true or if the array
        has already been successfully acquired, then try re-acquiring it */
    {
        if (!(a = (t_garray *)pd_findbyclass(d->d_symbol, garray_class)))
        {
            if (d->d_owner && *d->d_symbol->s_name)
                pd_error(d->d_owner, "%s: no such array", d->d_symbol->s_name);
            gpointer_unset(&d->d_gp);
            return 0;
        }
        else if (!garray_getfloatwords(a, npoints, vec))
        {
            if (d->d_owner)
                pd_error(d->d_owner, "%s: bad template", d->d_symbol->s_name);
            gpointer_unset(&d->d_gp);
            return 0;
        }
        else
        {
            gpointer_setarray(&d->d_gp, garray_getarray(a), *vec);
            return 1;
        }
    }
    return 0;
}

static void arrayvec_testvec(t_arrayvec *v)
{
    int i, vecsize;
    t_word *vec;
    for (i = 0; i < v->v_n; i++)
    {
        if (*v->v_vec[i].d_symbol->s_name)
            dsparray_get_array(&v->v_vec[i], &vecsize, &vec, 1);
    }
}

static void arrayvec_set(t_arrayvec *v, int argc, t_atom *argv)
{
    int i;
    for (i = 0; i < v->v_n && i < argc; i++)
    {
        gpointer_unset(&v->v_vec[i].d_gp); /* reset the pointer */
        if (argv[i].a_type != A_SYMBOL)
            pd_error(v->v_vec[i].d_owner,
                "expected symbolic array name, got number instead"),
                v->v_vec[i].d_symbol = &s_;
        else
        {
            v->v_vec[i].d_phase = 0x7fffffff;
            v->v_vec[i].d_symbol = argv[i].a_w.w_symbol;
        }
    }
    if (pd_getdspstate())
        arrayvec_testvec(v);
}

static void arrayvec_init(t_arrayvec *v, void *x, int rawargc, t_atom *rawargv)
{
    int i, argc;
    t_atom a, *argv;
    if (rawargc == 0)
    {
        argc = 1;
        SETSYMBOL(&a, &s_);
        argv = &a;
    }
    else argc = rawargc, argv=rawargv;

    v->v_vec = (t_dsparray *)getbytes(argc * sizeof(*v->v_vec));
    v->v_n = argc;
    for (i = 0; i < v->v_n; i++)
    {
        v->v_vec[i].d_owner = x;
        v->v_vec[i].d_phase = 0x7fffffff;
        gpointer_init(&v->v_vec[i].d_gp);
    }
    arrayvec_set(v, argc, argv);
}

static void arrayvec_free(t_arrayvec *v)
{
    int i;
    for (i = 0; i < v->v_n; i++)
        gpointer_unset(&v->v_vec[i].d_gp);
    freebytes(v->v_vec, v->v_n * sizeof(*v->v_vec));
}



static t_class *array_print_class;


/**
 * this is the dataspace of our new object
 * we don't need to store anything,
 * however the first (and only) entry in this struct
 * is mandatory and of type "t_object"
 */
typedef struct _array_print {
  t_object  x_obj;
  t_arrayvec *x_v;
} t_array_print;


/**
 * this method is called whenever a "bang" is sent to the object
 * the name of this function is arbitrary and is registered to Pd in the
 * array_print_setup() routine
 */
void array_print_bang(t_array_print *x)
{
  /*
   * post() is Pd's version of printf()
   * the string (which can be formatted like with printf()) will be
   * output to wherever Pd thinks it has too (pd's console, the stderr...)
   * it automatically adds a newline at the end of the string
   */
  t_word* buf;
  int maxindex;
  dsparray_get_array(x->x_v->v_vec, &maxindex, &buf);
  for(int i = 0; i < maxindex; i++)
    post("%d : %f\n", i, buf[i].w_float);
}


/**
 * this is the "constructor" of the class
 * this method is called whenever a new object of this class is created
 * the name of this function is arbitrary and is registered to Pd in the
 * array_print_setup() routine
 */
void *array_print_new(t_symbol *s, int argc, t_atom* argv)
{
  /*
   * call the "constructor" of the parent-class
   * this will reserve enough memory to hold "t_array_print"
   */
  t_array_print *x = (t_array_print *)pd_new(array_print_class);
  /*
   * return the pointer to the class - this is mandatory
   * if you return "0", then the object-creation will fail
   */
  arrayvec_init(&x->x_v, x, argc, argv);
  
  return (void *)x;
}

/**
 * define the function-space of the class
 * within a single-object external the name of this function is special
 */

static void array_print_set(t_array_print *x, t_symbol *s)
{ 
  arrayvec_set(&x->x_v, argc, argv);
}


static void array_print_free(t_array_print *UNUSED(x))
{
  array_vec_free(&x->x_v);
}


void array_print_setup(void) {
  /* create a new class */
  array_print_class = class_new(gensym("arrayprint"),        /* the object's name is "array_print" */
				(t_newmethod)array_print_new,
				(t_method) array_print_free /* the object's constructor is "array_print_new()" */
				sizeof(t_array_print),        /* the size of the data-space */
				CLASS_DEFAULT,               /* a normal pd object */
				A_GIMME,
				0);                          /* no creation arguments */

  /* attach functions to messages */
  /* here we bind the "array_print_bang()" function to the class "array_print_class" -
   * it will be called whenever a bang is received
   */
  class_addbang(array_print_class, array_print_bang);
  class_addmethod(array_print_class, (t_method)array_print_set,
		  gensym("set"), A_GIMME, 0);
}

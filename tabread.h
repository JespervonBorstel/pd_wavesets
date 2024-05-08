#include <m_pd.h>
#include <puredata/g_canvas.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

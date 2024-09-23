/*
  Mo 29. Apr 15:23:42 CEST 2024 JvB
  File for analysis of all kinds on the wavesets and audiofiles
*/

/* ---------- basic waveset analysis ---------- */

#include "wavesets.h"

int is_zero_cr (t_sample a, t_sample b);
int count_zerocr(t_word* buf, int maxindex);
int* get_zerocr_arr(t_word* buf, int maxindex, int num_zerocr);
void set_num_waveset(t_wavesetbuffer* x, int num_zerocr);
void init_waveset_array(t_wavesetbuffer* x);
void set_waveset_sizes (t_wavesetbuffer* x);
void get_extreme_wavesets(t_waveset* waveset_array, int num_wavesets, int* result);
void set_waveset_filt(t_waveset* waveset_array, int num_wavesets);

void set_waveset_peak_val(t_waveset* waveset, const t_word* buf);
void set_wavesets_loudness(t_waveset* waveset_array, int num_wavesets, const t_word* buf);
  
void analyse_wavesets(t_wavesetbuffer *x, t_word* buf, int maxindex);

#include "analyse.h"
#include "utils.h"

int is_zero_cr (t_sample a, t_sample b)
{
  if((a * b) < 0)
    return 1;
  else
    return 0;
}

int count_zerocr(t_word* buf, int maxindex)
{
  int counter = 0;
  int i = 1;
  int is_first_zerocr = 1;
  while (i < maxindex) {
    if(is_zero_cr(buf[i - 1].w_float, buf[i].w_float) && is_first_zerocr) {
      is_first_zerocr = 0;
      counter++;
      i++;
    }
    if(is_zero_cr(buf[i - 1].w_float, buf[i].w_float) && !is_first_zerocr) {
      is_first_zerocr = 1;
      i++;
    }
    else i++;
  }
  return counter;
}

int* get_zerocr_arr(t_word* buf, int maxindex, int num_zerocr)
{
  int* zerocr_arr = (int*)getbytes(num_zerocr * sizeof(int));
  int index = 0;
  int is_first_zerocr = 1;  
  int i = 1;
  
  while(i < maxindex) {
    if(is_zero_cr(buf[i-1].w_float, buf[i].w_float) && is_first_zerocr) {
      zerocr_arr[index] = i - 1;
      is_first_zerocr = 0;
      index++;
      i++;
    }
    if(is_zero_cr(buf[i - 1].w_float, buf[i].w_float) && (!is_first_zerocr)) {
      is_first_zerocr = 1;
      i++;
    }
    else i++;
  }
  return zerocr_arr;
}


void set_num_waveset(t_wavesetbuffer* x, int num_zerocr)
{
  if(num_zerocr < 1)
    x->num_wavesets = 0;
  else
  x->num_wavesets = num_zerocr - 1;
}

void init_waveset_array(t_wavesetbuffer* x)
{
  int num_wavesets = x->num_wavesets;

  if(num_wavesets == 0)
    pd_error(x, "wavesetbuffer: wavesetarray empty");
  t_waveset* waveset_array = (t_waveset*)getbytes(num_wavesets * sizeof(t_waveset));
  if(waveset_array == NULL)
    pd_error(x, "wavesetbuffer: waveset allocation failed");
  x->waveset_array = waveset_array;
}

void set_waveset_sizes (t_wavesetbuffer* x)
{
  for(int i = 0; i < x->num_wavesets; i++) {
    t_waveset* waveset = &x->waveset_array[i];
    waveset->size = (waveset->end_index - waveset->start_index) + 1;
  }
}

void set_waveset_filt(t_waveset* waveset_array, int num_wavesets)
{
  if(num_wavesets) {
    int* sorted_lookup = (int*)getbytes(num_wavesets * sizeof(int));    
    for(int i = 0; i < num_wavesets; i++) {
      sorted_lookup[i] = i;
    }    
    qsort_wavesets(sorted_lookup, 0, num_wavesets - 1, waveset_array);
    
    if(num_wavesets > 1)
      for(int i = 0; i < num_wavesets; i++) {
	waveset_array[sorted_lookup[i]].filt = (t_float)i / ((t_float)num_wavesets - 1.0);
      }
    else
      waveset_array[0].filt = 0;
    
    freebytes(sorted_lookup, num_wavesets * sizeof(int));
  }
}

void set_waveset_peak_val(t_waveset* waveset, const t_word* buf)
{
  t_sample highest = 0;
  for(int i = waveset->start_index; i < waveset->end_index; i++) {
    t_sample sample = (t_sample)fabsf(buf[i].w_float);
    if(sample > highest)
      highest = sample;
  }
  waveset->loudest = highest;
}

void set_wavesets_loudness(t_waveset* waveset_array, int num_wavesets, const t_word* buf)
{
  for(int i = 0; i < num_wavesets; i++) {
    set_waveset_peak_val(&(waveset_array[i]), buf);
  }
}

void analyse_wavesets(t_wavesetbuffer *x, t_word* buf, int maxindex)
{
  int num_zerocr = count_zerocr(buf, maxindex);
  int* zerocr_arr = get_zerocr_arr(buf, maxindex, num_zerocr);

  set_num_waveset(x, num_zerocr);
  init_waveset_array(x);

  t_waveset* waveset_array = x->waveset_array;
  
  for(int i = 0; i < (num_zerocr - 1); i++) {
    int j = i + 1;
    waveset_array[i].start_index = zerocr_arr[i];
    waveset_array[i].end_index = zerocr_arr[j] - 1;
  }
  
  set_waveset_sizes(x);
  set_waveset_filt(x->waveset_array, x->num_wavesets);

  set_wavesets_loudness(x->waveset_array, x->num_wavesets, buf);
  
  freebytes(zerocr_arr, num_zerocr * sizeof(int));
}

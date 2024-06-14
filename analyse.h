/*
  Mo 29. Apr 15:23:42 CEST 2024 JvB
  File for analysis of all kinds on the wavesets and audiofiles
*/

/* ---------- basic waveset analysis ---------- */

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

void set_num_waveset(t_wavesetplayer_tilde* x, int num_zerocr)
{
  if(num_zerocr < 1)
    x->num_wavesets = 0;
  else
  x->num_wavesets = num_zerocr - 1;
}

void init_waveset_array(t_wavesetplayer_tilde* x)
{
  int num_wavesets = x->num_wavesets;

  if(num_wavesets == 0)
    pd_error(x, "wavesetplayer~: wavesetarray empty");
  t_waveset* waveset_array = (t_waveset*)getbytes(num_wavesets * sizeof(t_waveset));
  if(waveset_array == NULL)
    pd_error(x, "wavesetplayer~: waveset allocation failed");
  x->waveset_array = waveset_array;
}

void set_waveset_sizes (t_wavesetplayer_tilde* x)
{
  for(int i = 0; i < x->num_wavesets; i++) {
    t_waveset* waveset = &x->waveset_array[i];
    waveset->size = (waveset->end_index - waveset->start_index) + 1;
  }
}

void analyse_wavesets (t_wavesetplayer_tilde* x, t_word* buf, int maxindex)
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
  freebytes(zerocr_arr, num_zerocr * sizeof(int));
}


// for wavesetstepper~

void set_num_waveset_stepper(t_wavesetstepper_tilde* x, int num_zerocr)
{
  if(num_zerocr < 1)
    x->num_wavesets = 0;
  else
  x->num_wavesets = num_zerocr - 1;
}

void init_waveset_array_stepper(t_wavesetstepper_tilde* x)
{
  int num_wavesets = x->num_wavesets;

  if(num_wavesets == 0)
    pd_error(x, "wavesetstepper~: wavesetarray empty");
  t_waveset* waveset_array = (t_waveset*)getbytes(num_wavesets * sizeof(t_waveset));
  if(waveset_array == NULL)
    pd_error(x, "wavesetstepper~: waveset allocation failed");
  x->waveset_array = waveset_array;
}

void set_waveset_sizes_stepper (t_wavesetstepper_tilde* x)
{
  for(int i = 0; i < x->num_wavesets; i++) {
    t_waveset* waveset = &x->waveset_array[i];
    waveset->size = (waveset->end_index - waveset->start_index) + 1;
  }
}

// analysing for the filtering data
// returns an array of size 2 with the size of the smallest and biggest waveset 
void get_extreme_wavesets(t_waveset* waveset_array, int num_wavesets, int* result)
{
  int smallest = waveset_array[0].size;
  int biggest = waveset_array[0].size;
  for(int i = 1; i < num_wavesets; i++) {
    int size = waveset_array[i].size;
    smallest = size < smallest ? size : smallest;
    biggest = size > biggest ? size : biggest;
  }
  result[0] = smallest;
  result[1] = biggest;
}

void set_waveset_filt(t_waveset* waveset_array, int num_wavesets)
{
  int* result = (int*)getbytes(2 * sizeof(int));
  get_extreme_wavesets(waveset_array, num_wavesets, result);

  t_float smallest = result[0];
  t_float biggest = result[1];
  //post("biggest: %d\nsmallest: %d\n", biggest, smallest);
  t_float delta = biggest - smallest;
  for(int i = 0; i < num_wavesets; i++) {
    t_waveset *waveset = &waveset_array[i];
    t_float size = waveset->size;
    waveset->filt = delta == 0 ? 0 : (size - smallest) / delta;
    //post("filter value: %f\n", waveset->filt);
  }
  freebytes(result, 2 * sizeof(int));
}

void analyse_wavesets_stepper (t_wavesetstepper_tilde* x, t_word* buf, int maxindex)
{
  int num_zerocr = count_zerocr(buf, maxindex);
  int* zerocr_arr = get_zerocr_arr(buf, maxindex, num_zerocr);

  set_num_waveset_stepper(x, num_zerocr);
  init_waveset_array_stepper(x);

  t_waveset* waveset_array = x->waveset_array;
  
  for(int i = 0; i < (num_zerocr - 1); i++) {
    int j = i + 1;
    waveset_array[i].start_index = zerocr_arr[i];
    waveset_array[i].end_index = zerocr_arr[j] - 1;
  }
  
  set_waveset_sizes_stepper(x);
  set_waveset_filt(waveset_array, x->num_wavesets);
  
  freebytes(zerocr_arr, num_zerocr * sizeof(int));
}

// Function to swap two elements 
void swap(int* a, int* b) 
{ 
	int temp = *a; 
	*a = *b; 
	*b = temp; 
} 

int partition(int sorted_lookup[], int low, int high, t_waveset waveset_array[]) 
{ 
  // initialize pivot to be the first element 
  t_float pivot = waveset_array[sorted_lookup[low]].filt; 
  int i = low; 
  int j = high;
  
  while (i < j) { 
    
    while (waveset_array[sorted_lookup[i]].filt <= pivot && i <= high - 1) { 
      i++;
    } 
    
    while (waveset_array[sorted_lookup[j]].filt > pivot && j >= low + 1) { 
      j--; 
    }
    if (i < j) {
      swap(&sorted_lookup[i], &sorted_lookup[j]); 
    }
  }
  swap(&sorted_lookup[low], &sorted_lookup[j]); 
  return j; 
}

// QuickSort function 
void qsort_wavesets(int sorted_lookup[], int low, int high, t_waveset waveset_array[]) 
{ 
	if (low < high) { 

		// call Partition function to find Partition Index 
	  int partitionIndex = partition(sorted_lookup, low, high, waveset_array); 

		// Recursively call quickSort() for left and right 
		// half based on partition Index 
	  qsort_wavesets(sorted_lookup, low, partitionIndex - 1, waveset_array);
	  qsort_wavesets(sorted_lookup, partitionIndex + 1, high, waveset_array);
	} 
}

#include "utils.h"

/* Function to swap two elements */
void swap(int* a, int* b) 
{ 
  int temp = *a; 
  *a = *b; 
  *b = temp; 
} 

int partition(int sorted_lookup[], int low, int high, t_waveset *waveset_array) 
{ 
  // initialize pivot to be the first element 
  int pivot = waveset_array[sorted_lookup[low]].size; 
  int i = low; 
  int j = high;
  
  while (i < j) { 
    
    while (waveset_array[sorted_lookup[i]].size <= pivot && i <= high - 1) { 
      i++;
    } 
    
    while (waveset_array[sorted_lookup[j]].size > pivot && j >= low + 1) { 
      j--; 
    }
    if (i < j) {
      swap(&sorted_lookup[i], &sorted_lookup[j]); 
    }
  }
  swap(&sorted_lookup[low], &sorted_lookup[j]); 
  return j; 
}

/* QuickSort function  */
void qsort_wavesets(int sorted_lookup[], int low, int high, t_waveset *waveset_array) 
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

/* implementation of the positive mod */
int mod(int i, int n)
{
    return (i % n + n) % n;
}

/* implementation of the reference list in the buffer object
 * if new dsp object are added everything here needs to be updated:
 *  - type of the object added to the enum and union
 *  - nodecreation und remove function
 *  - add case to add_to_reference-list
 */

/* converting the enem value to a string.
   Later needed for printing the list */
const char* get_type(object_type type) {
  switch(type) {
  case wavesetstepper: return "wavesetstepper~";
  case wavesetplayer: return "wavesetplayer~";
  default: return "unknown";
  }
}

t_ref_list *new_ref_list()
{
  t_ref_list *listp = (t_ref_list*)getbytes(sizeof(t_ref_list));
  listp->head = NULL;
  return listp;
}

/* Function to create a new node with wavesetstepperpointer */
t_node* create_wavesetstepper_node(t_wavesetstepper_tilde* x) {
  t_node *new_node = (t_node *)getbytes(sizeof(t_node));
  new_node->object_pointer.wavesetstepper = x;
  new_node->type = wavesetstepper;
  new_node->next = NULL;
  return new_node;
}

/* same for wavesetplayer */
t_node* create_wavesetplayer_node(t_wavesetplayer_tilde* x) {
  t_node *new_node = (t_node *)getbytes(sizeof(t_node));
  new_node->object_pointer.wavesetplayer = x;
  new_node->type = wavesetplayer;
  new_node->next = NULL;
  return new_node;
}

void append_node(t_ref_list *list, t_node *new_node) {
  t_node *head = list->head;
  if (head == NULL) {
    list->head = new_node;
  } else {
    t_node *current = head;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = new_node;
  }
}

void remove_node(t_ref_list *list, object_type type, reference_pointer rp)
{
  t_node* head = list->head;
  t_node* current = head;
  t_node* prev = NULL;
  while(current) {
    int match = 0;
    if (current->type == type) {
      switch (type) {
      case wavesetstepper:
	if (current->object_pointer.wavesetstepper == rp.wavesetstepper) {
	  match = 1;
	}
	break;
      case wavesetplayer:
	if (current->object_pointer.wavesetplayer == rp.wavesetplayer) {
	  match = 1;
	}
	break;
	/* Add other cases if needed: will be needed for other objects referencing wavesetbuffer */
      }
      if(match) {
	if(prev == NULL)
	  list->head = current->next;
	else
	  prev->next = current->next;
	freebytes(current, sizeof(t_node));
	return;
      }
    }
    prev = current;
    current = current->next;
  }
}

/* Function to free the linked list
 * segfaults for some reason, needs fixing */
void free_list(t_node *head) {
  t_node *current = head;
  t_node *next_node = NULL;
  while (current != NULL) {
    next_node = current->next;
    freebytes(current, sizeof(t_node));
    current = next_node;
  }
}

void free_ref_list(t_ref_list *ref_listp)
{
  /* uncomment if fixed */
  if(ref_listp) {
    //free_list(ref_listp->head);
    freebytes(ref_listp, sizeof(t_ref_list));
  }
}

void add_to_reference_list(reference_pointer rp, object_type type, t_wavesetbuffer* bufp)
{
  switch(type) {
  case wavesetstepper: {
    t_node* new_node = create_wavesetstepper_node(rp.wavesetstepper);
    append_node(bufp->reference_listp, new_node);
    break;
  }
  case wavesetplayer: {
    t_node* new_node = create_wavesetplayer_node(rp.wavesetplayer);
    append_node(bufp->reference_listp, new_node);
    break;
  }
  default: break;
  }
}

void remove_from_reference_list(reference_pointer rp, object_type type, t_wavesetbuffer* bufp)
{
  if(bufp)
    remove_node(bufp->reference_listp, type, rp);
}

void print_reference_list(t_ref_list *list)
{
  t_node *current = list->head;
  if(!current)
    post("wavesetbuffer: reference list empty");
  while (current != NULL) {
    post("object: %s\npointerval: %d\n", get_type(current->type), (int)(current->object_pointer.wavesetstepper));
    current = current->next;
  }
}

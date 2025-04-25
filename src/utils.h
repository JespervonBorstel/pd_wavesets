#ifndef UTILS_H
#define UTILS_H
#include "wavesets.h"


typedef struct _event {
  t_waveset* waveset;
  int play_index;
  /* bool wether the grain is played or not */
  int skip;
} t_event;
/* Function to swap two elements */
void swap(int* a, int* b);
int partition(int sorted_lookup[], int low, int high, t_waveset *waveset_array);
/* QuickSort function  */
void qsort_wavesets(int sorted_lookup[], int low, int high, t_waveset *waveset_array);

int filter_partition(int filter_lookup[], int low, int high, t_waveset *waveset_array);
/* QuickSort function  */
void qsort_filter(int filter_lookup[], int low, int high, t_waveset *waveset_array);

/* implementation of the positive mod */
int mod(int i, int n);

/* converting the enem value to a string.
   Later needed for printing the list */
const char* get_type(object_type type);

t_ref_list *new_ref_list();
/* Function to create a new node with wavesetstepperpointer */
t_node* create_wavesetstepper_node(t_wavesetstepper_tilde* x);
/* same for wavesetplayer */
t_node* create_wavesetplayer_node(t_wavesetplayer_tilde* x);
t_node* create_wavesetclouds_node(t_wavesetclouds_tilde* x);
t_node* create_wavesetjitter_node(t_wavesetjitter_tilde* x);

void append_node(t_ref_list *list, t_node *new_node);
void remove_node(t_ref_list *list, object_type type, reference_pointer rp);
/* Function to free the linked list
 * segfaults for some reason, needs fixing */
void free_list(t_node *head);
void free_ref_list(t_ref_list *ref_listp);
void add_to_reference_list(reference_pointer rp, object_type type, t_wavesetbuffer* bufp);
void remove_from_reference_list(reference_pointer rp, object_type type, t_wavesetbuffer* bufp);
void print_reference_list(t_ref_list *list);

#endif

/**
 * CS 2110 - Fall 2011 - Homework #11
 * Edited by: Tomer Elmalem
 *
 * list.c: Complete the functions!
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>  
#include "list.h"

/* The node struct.  Has a prev pointer, next pointer, and data. */
/* DO NOT DEFINE ANYTHING OTHER THAN WHAT'S GIVEN OR YOU WILL GET A ZERO*/
/* Design consideration only this file should know about nodes */
/* Only this file should be manipulating nodes */
typedef struct lnode
{
  struct lnode* prev; /* Pointer to previous node */
  struct lnode* next; /* Pointer to next node */
  void* data; /* User data */
} node;


/* Do not create any global variables here. Your linked list library should obviously work for multiple linked lists */
// This function is declared as static since you should only be calling this inside this file.
static node* create_node(void* data);

/** create_list
  *
  * Creates a list by allocating memory for it on the heap.
  * Be sure to initialize size to zero and head to NULL.
  *
  * @return an empty linked list
  */
list* create_list(void)
{
	//printf("sizeof(list):%d\n",(unsigned int)sizeof(list));	
  list *l = malloc(sizeof(list));
	if(l == NULL)
	{
		printf("*******%d%d\n",errno,ENOMEM);
		return NULL;

	}
  l->head = NULL;
  l->size = 0;
 // pthread_mutex_init(&(l->mutex), NULL);
  return l;
}

/** create_node
  *
  * Helper function that creates a node by allocating memory for it on the heap.
  * Be sure to set its pointers to NULL.
  *
  * @param data a void pointer to data the user wants to store in the list
  * @return a node
  */
static node* create_node(void* data)
{
	//printf("sizeof(node):%d\n",(int)sizeof(node));
	node *n = malloc(sizeof(node));
	if(!n)
	{
		//printf("%s\n",strerror(errno)); 
		printf("errno=%d\n",errno);
		printf("create_node malloc fail..\n");
	}
  
  n->data = data;
  n->prev = NULL;
  n->next = NULL;
  return n;
}

/** push_front
  *
  * Adds the data to the front of the linked list
  *
  * @param llist a pointer to the list.
  * @param data pointer to data the user wants to store in the list.
  */
void push_front(list* llist, void* data)
{
  node *n = create_node(data);

  //pthread_mutex_lock(&(llist->mutex));
  // if the list if size 0
  if (!llist->size) {
    // then the next and prev nodes to itself
    n->next = n;
    n->prev = n;
  } else {
    node *head = llist->head;
    node *prev = head->prev;
    
    // set the new nodes next and prev pointers
    n->next = head;
    n->prev = head->prev;

    // set the prev and next pointers to the new node
    head->prev = n;
    prev->next = n;
  }

  // set the head of the list to the new node
  llist->head = n;
  llist->size++;
  //pthread_mutex_unlock(&(llist->mutex));
}

/** push_back
  *
  * Adds the data to the back/end of the linked list
  *
  * @param llist a pointer to the list.
  * @param data pointer to data the user wants to store in the list.
  */
void push_back(list* llist, void* data)
{
  node *n = create_node(data);
  
  //pthread_mutex_lock(&(llist->mutex));

  // if the list size is 0
  if (!llist->size) {
    // set the next and prev to the new node
    n->next = n;
    n->prev = n;

    // since there are no other nodes this node will be the head
    llist->head = n;
  } else {
    node *head = llist->head;
    node *prev = head->prev;

    // insert to the back by setting next to the head
    n->next = head;
    n->prev = head->prev;

    // Update the next and prev pointers to the current node
    head->prev = n;
    prev->next = n;
  }
  llist->size++;
  //pthread_mutex_unlock(&(llist->mutex));
}

/** push_if
  *
  * Adds the data to the linked list
  *
  * @param llist a pointer to the list.
  * @param data pointer to data the user wants to store in the list.
  */
void push_if(list* llist, void* data,equal_op compare_func)
{
  node *n = create_node(data);
  int i;
  
  //printf("llist->size:%d\n",llist->size);
  // if the list size is 0
  if (!llist->size) {
    // set the next and prev to the new node
    n->next = n;
    n->prev = n;

    // since there are no other nodes this node will be the head
    llist->head = n;
	llist->size++;
		
  } else {
	node *current = llist->head;
	for (i=0; i<llist->size; i++) {
	    // if the compare func is 1, occurrence found return 1
	    if (compare_func(data, current->data)) 
		{
			node *prev = current->prev;
			// insert to before current node.
			n->next = current;
			n->prev = prev;

			// Update the next and prev pointers to the current node
			current->prev = n;
			prev->next = n;
			llist->size++;

			//Add at the first.
			if(i==0) llist->head = n;		
			return ;
	    }
	    current = current->next;
  	}
	//need add at last.
	//and free the new one.
	free(n);
	push_back(llist,data);
  }
  
}


/** remove_front
  *
  * Removes the node at the front of the linked list
  *
  * @warning Note the data the node is pointing to is also freed. If you have any pointers to this node's data it will be freed!
  *
  * @param llist a pointer to the list.
  * @param free_func pointer to a function that is responsible for freeing the node's data.
  * @return -1 if the remove failed (which is only there are no elements) 0 if the remove succeeded.
  */
int remove_front(list* llist, list_op free_func)
{
	//pthread_mutex_lock(&(llist->mutex));

	if(llist== NULL)
	{
		printf("remove_front llist is null\n");
	}
	if (!llist->size)
	{
		//pthread_mutex_unlock(&(llist->mutex));
		return -1;
	}

  //pthread_mutex_lock(&(llist->mutex));

  node *head = llist->head;
  
  if (llist->size == 1) {
    // if the size is 1 set the head to NULL since there are no other nodes
    llist->head = NULL;
  } else {
    node *next = head->next;
    node *prev = head->prev;

    // update the head
    llist->head = next;

    // update the pointers
    next->prev = prev;
    prev->next = next;
  }

  // free the data and the node
  free_func(head->data);
  free(head);

  llist->size--;
 // pthread_mutex_unlock(&(llist->mutex));
  return 0;
}

/** remove_index
  *
  * Removes the indexth node of the linked list
  *
  * @warning Note the data the node is pointing to is also freed. If you have any pointers to this node's data it will be freed!
  *
  * @param llist a pointer to the list.
  * @param index index of the node to remove.
  * @param free_func pointer to a function that is responsible for freeing the node's data.
  * @return -1 if the remove failed 0 if the remove succeeded.
  */
int remove_index(list* llist, int index, list_op free_func)
{
  if (!llist->size) return -1;

  int i;

  //pthread_mutex_lock(&(llist->mutex));

  node *current = llist->head; // = index of 0

  // loop through until you get to where you want
  for (i=0; i<index; i++) {
    current = current->next;
  }

  // if the size is 1
  if (llist->size == 1) {
    // make the head null
    llist->head = NULL;
  } else {
    node *next = current->next;
    node *prev = current->prev;
    
    // update the pointers to remove the node
    prev->next = next;
    next->prev = prev;
  }
  
  // Free the data and node
  free_func(current->data);
  free(current);
  
  llist->size--;
  //pthread_mutex_unlock(&(llist->mutex));

  return 0;
}

/** remove_back
  *
  * Removes the node at the back of the linked list
  *
  * @warning Note the data the node is pointing to is also freed. If you have any pointers to this node's data it will be freed!
  *
  * @param llist a pointer to the list.
  * @param free_func pointer to a function that is responsible for freeing the node's data.
  * @return -1 if the remove failed 0 if the remove succeeded.
  */
int remove_back(list* llist, list_op free_func)
{ 
	//pthread_mutex_lock(&(llist->mutex));
	if (!llist->size)
	{
		//pthread_mutex_unlock(&(llist->mutex));
		return -1;
	}
  
  

  node *head = llist->head;
  node *tbr = head->prev; // to be removed
  node *nb = tbr->prev; // new back

  // if list if of size 1
  if (llist->size == 1) {
    // make the head null
    llist->head = NULL;
  } else {
    // update the pointers to back is gone.
    head->prev = nb;
    nb->next = head;
  }

  // free the data and node
  free_func(tbr->data);
  free(tbr);
  
  llist->size--;
  //pthread_mutex_unlock(&(llist->mutex));

  return 0;
}

/** remove_data
  *
  * Removes ALL nodes whose data is EQUAL to the data you passed in or rather when the comparison function returns true (!0)
  * @warning Note the data the node is pointing to is also freed. If you have any pointers to this node's data it will be freed!
  *
  * @param llist a pointer to the list
  * @param data data to compare to.
  * @param compare_func a pointer to a function that when it returns true it will remove the element from the list and do nothing otherwise @see equal_op.
  * @param free_func a pointer to a function that is responsible for freeing the node's data
  * @return the number of nodes that were removed.
  */
int remove_data(list* llist, const void* data, equal_op compare_func, list_op free_func)
{
  int removed = 0;
  int i;

  if (!llist->size) return removed;

  //pthread_mutex_lock(&(llist->mutex));

  node *current = llist->head;
  node *next = current->next;
  node *prev = current->prev;
  int is_head = 1;

  for (i=0; i<llist->size; i++) {
    if (compare_func(data, current->data)) {
      // if we are still on the head update the current head
      if (is_head) llist->head = next;

      // update the pointers
      next->prev = prev;
      prev->next = next;

      // free the data and node
      free_func(current->data);
      free(current);

      // the current is the next node
      current = next;

      removed++;
    } else {
      // no longer on the head
      is_head = 0;
      
      // move to the next node
      current = current->next;
    }

    // update the previous and next node
    if (llist->size > 1) {
      next = current->next;
      prev = current->prev;
    }
  }

  // update the size
  llist->size-=removed;

  // if the size is zero the list is empty and the head should be null
  if (!llist->size) llist->head = NULL;
  //pthread_mutex_unlock(&(llist->mutex));

  return removed;
}


/** front
  *
  * Gets the data at the front of the linked list
  * If the list is empty return NULL.
  *
  * @param llist a pointer to the list
  * @return The data at the first node in the linked list or NULL.
  */
void* front(list* llist)
{
  if (llist->size) {
    // if the list has a size return the data
    return llist->head->data;
  } else {
    // or return NULL
    return NULL;
  }
}

/** get_index
  *
  * Gets the data at the indexth node of the linked list
  * If the list is empty or if the index is invalid return NULL.
  *
  * @param llist a pointer to the list
  * @return The data at the indexth node in the linked list or NULL.
  */
void* get_index(list* llist, int index)
{
  // if the list is 0 or if the index in larger than the size
  if (!llist->size || index >= llist->size) return NULL;

  int i;
  
 // pthread_mutex_lock(&(llist->mutex));
  
  // loop through the list until you get to the desired index
  node *current = llist->head; // index = 0
  for (i=0; i<index; i++) {
    current = current->next;
  }
  //pthread_mutex_unlock(&(llist->mutex));

  return current->data;
}

/** get_next
  *
  * Gets the next data at the linked list
  * If the list is empty or if the index is invalid return NULL.
  *
  * @param llist a pointer to the list
  * @return The data at the linked list or NULL.
  */
void* get_next(list* llist,const void* search, equal_op compare_func)
{
  int i;
  // if the list is 0 
  if (!llist->size ) return NULL;

  // loop through the list
  node *current = llist->head;
  for (i=0; i<llist->size; i++) {
    // if the compare func is 1, occurrence found return 1
    if (compare_func(search, current->data)) return current->next->data;
    current = current->next;
  }
  
  return NULL;
}

/** get_if
  *
  * Gets the data at the linked list
  * If the list is empty or if the index is invalid return NULL.
  *
  * @param llist a pointer to the list
  * @return The data at the linked list or NULL.
  */
void* get_if(list* llist,const void* search, equal_op compare_func)
{
  int i;
  // if the list is 0 
  if (!llist->size ) return NULL;

  // loop through the list
  node *current = llist->head;
  for (i=0; i<llist->size; i++) {
    // if the compare func is 1, occurrence found return 1
    if (compare_func(search, current->data)) return current->data;
    current = current->next;
  }
  
  return NULL;
}


/** back
  *
  * Gets the data at the "end" of the linked list
  * If the list is empty return NULL.
  *
  * @param llist a pointer to the list
  * @return The data at the last node in the linked list or NULL.
  */
void* back(list* llist)
{
  // if the list is empty return null
  if (!llist->size) return NULL;
  
  //pthread_mutex_lock(&(llist->mutex));

  // return the previous of the head
  node *end = llist->head->prev;
  //pthread_mutex_unlock(&(llist->mutex));
  return end->data;
}

/** is_empty
  *
  * Checks to see if the list is empty.
  *
  * @param llist a pointer to the list
  * @return 1 if the list is indeed empty 0 otherwise.
  */
int is_empty(list* llist)
{
  if (llist->size == 0 && llist->head == NULL) {
    return 1;
  } else {
    return 0;
  }
}

/** size
  *
  * Gets the size of the linked list
  *
  * @param llist a pointer to the list
  * @return The size of the linked list
  */
int size(list* llist)
{
	//printf("%s %d ...\n",__FUNCTION__,__LINE__);
	if(llist)
	  return llist->size;
	else
	{
		printf("%s %d llist is null...\n",__FUNCTION__,__LINE__);
		return 0;
	}
}

/** find_occurence
  *
  * Tests if the search data passed in is in the linked list.
  *
  * @param llist a pointer to a linked list.
  * @param search data to search for the occurence.
  * @param compare_func a pointer to a function that returns true if two data items are equal @see equal_op.
  * @return 1 if the data is indeed in the linked list 0 otherwise.
  */
int find_occurrence(list* llist, const void* search, equal_op compare_func)
{
  int i;
  // loop through the list
  node *current = llist->head;
  for (i=0; i<llist->size; i++) {
    // if the compare func is 1, occurrence found return 1
    if (compare_func(search, current->data)) return 1;
    current = current->next;
  }

  // no occurrence founds
  return 0;
}

/** empty_list
  *
  * Empties the list after this is called the list should be empty.
  *
  * @param llist a pointer to a linked list.
  * @param free_func function used to free the node's data.
  *
  */
void empty_list(list* llist, list_op free_func)
{
  // if the size is 0 return
  if (!llist->size) return;

  int i;
  node *current = llist->head;
  node *next = current->next;

  // loop through the list and free all the nodes
  for (i=0; i<llist->size; i++) {
    free_func(current->data);
    free(current);
    current = next;
    
    // if it's not the end of the list set the next node
    if (i < llist->size-1) next = current->next;
  }

  // reset the head and size
  llist->head=NULL;
  llist->size=0;
}

/** traverse
  *
  * Traverses the linked list calling a function on each node's data.
  *
  * @param llist a pointer to a linked list.
  * @param do_func a function that does something to each node's data.
  */
void traverse(list* llist, list_op do_func)
{
  int i;
  node *current = llist->head;
  for (i=0; i<llist->size; i++) {
    do_func(current->data);
    current = current->next;
  }
}

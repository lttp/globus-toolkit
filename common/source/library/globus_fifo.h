#ifndef GLOBUS_COMMON_FIFO_H
#define GLOBUS_COMMON_FIFO_H

/********************************************************************
 *
 * This file defines the globus_fifo_t type
 * useful for queuing arbitrary data (via void-pointer)
 *
 ********************************************************************/
#include "globus_common_include.h"
#include "globus_list.h"
 
EXTERN_C_BEGIN

struct globus_fifo_s;
typedef struct globus_fifo_s *                          globus_fifo_t;

/**
 *  Initialize the fifo structure.
 *
 *  This function initializes the fifo data structure.  The structure
 *  must be initalized before it can be used with any other function.
 */
extern int
globus_fifo_init(
    globus_fifo_t *                                     fifo);

/**
 *  Destroy the fifo data structure.
 *
 *  This function destroys the memory associate with the fifo data structure.
 *  For every call to globus_fifo_init() there must be a corresponding call to
 *  globus_fifo_destroy()
 */
extern void
globus_fifo_destroy(
    globus_fifo_t *                                     fifo);

/**
 *  This function returns a boolean indicating whether or not the 
 *  fifo is empty.
 */
extern int 
globus_fifo_empty(
    const globus_fifo_t *                               fifo);

/**
 *  This function returns a interger representing the number
 *  of elements in the queue.
 */
extern int 
globus_fifo_size(
    const globus_fifo_t *                               fifo);

/**
 *  Add data to the back of the queue.
 */
extern int
globus_fifo_enqueue(
    globus_fifo_t *                                     fifo,
	void *                                              datum);

/**
 *  Make a copy of the fifo.  globus_fifo_destroy() must be called
 *  the returned pointer to free memory associated with it.
 */
extern globus_fifo_t *
globus_fifo_copy(
    const globus_fifo_t *                               fifo);

/**
 *  get a pointer to the element at the front of the queue.
 */
extern void *
globus_fifo_peek(
    globus_fifo_t *                                     fifo);

/**
 *  Get a pointer to the element at the back of the queue.
 */
extern void *
globus_fifo_tail_peek(
    globus_fifo_t *                                     fifo);

/**
 *  remove datum from anywhere in the queue.
 */
extern void *
globus_fifo_remove(
    globus_fifo_t *                                     headp, 
    void *                                              datum);

/**
 *  dequeue the element at the front of the queue.
 */
extern void *
globus_fifo_dequeue(
    globus_fifo_t *                                     fifo);

/**
 *  Move the queue from fifo_src pointer to fifo_dest pointer.
 */
extern int
globus_fifo_move(
    globus_fifo_t *				                        fifo_dest,
    globus_fifo_t *				                        fifo_src);

EXTERN_C_END

#endif /* GLOBUS_COMMON_FIFO_H */


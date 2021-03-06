/*
 * include/linux/idr.h
 * 
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *      Copyright (C) 2002 by Concurrent Computer Corporation
 *      Distributed under the GNU GPL license version 2.
 *
 * Small id to pointer translation service avoiding fixed sized
 * tables.
 */
#include <radix.h>

#define INT_MAX		((int)(~0U>>1))

struct idr {
	struct radix_tree_root	idr_rt;
	unsigned long		idr_base;
	unsigned int		idr_next;
};

/*
 * The IDR API does not expose the tagging functionality of the radix tree
 * to users. Use tag 0 to track whether a node has free space below it.
 */
#define IDR_FREE	0

/* Set the IDR flag and the IDR_FREE tag */
#define IDR_RT_MARKER	(ROOT_IS_IDR | (gfp_t)(1 << 		\
					(ROOT_TAG_SHIFT + IDR_FREE)))

#define IDR_INIT_BASE(name, base) {				\
	.idr_rt = RADIX_TREE_INIT(name, IDR_RT_MARKER),		\
	.idr_base = (base),					\
	.idr_next = 0,						\
}

/*
 * IDR_INIT() - Initialise an IDR.
 * @name: Name of IDR.
 *
 * A freshly-initialised IDR contains no IDs.
 */
#define IDR_INIT(name)	IDR_INIT_BASE(name, 0)

/*
 * DEFINE_IDR() - Define a statically-allocatted IDR.
 * @name: Name of IDR.
 *
 * An IDR defined using this macro is ready for use with no additional
 * initialisation required. It contains no IDs.
 */
#define DEFINE_IDR(name)	struct idr name = IDR_INIT(name)

/**
 * idr_get_cursor - Return the current position of the cyclic allocator
 * @idr: idr handle
 *
 * The value returned is the value that will be next returned from
 * idr_alloc_cyclic() if it is free (otherwise the search will start from
 * this position).
 */
static inline unsigned int idr_get_cursor(const struct idr *idr)
{
	return READ_ONCE(idr->idr_next);
}

/**
 * idr_set_cursor - Set the current position of the cyclic allocator
 * @idr: idr handle
 * @val: new position
 *
 * The next call to idr_alloc_cyclic() will return @val if it is free
 * (otherwise the search will start from this position).
 */
static inline void idr_set_cursor(struct idr *idr, unsigned int val)
{
	WRITE_ONCE(idr->idr_next, val);
}

/**
 * idr_init_base() - Initialise an IDR.
 * @idr: IDR handle.
 * @base: The base value for the IDR.
 *
 * This variation of idr_init() creates an IDR which will allocate IDs
 * starting at %base.
 */
static inline void idr_init_base(struct idr *idr, int base)
{
	INIT_RADIX_TREE(&idr->idr_rt, IDR_RT_MARKER);
	idr->idr_base = base;
	idr->idr_next = 0;
}

/**
 * idr_init() - Initialise an IDR.
 * @idr: IDR handle.
 *
 * Initialise a dynamically allocated IDR.  To initialise a
 * statically allocated IDR, use DEFINE_IDR().
 */
static inline void idr_init(struct idr *idr)
{
	idr_init_base(idr, 0);
}

/**
 * idr_is_empty() - Are there any IDs allocated?
 * @idr: IDR handle.
 *
 * Return: %true if any IDs have been allocated from this IDR.
 */
static inline bool idr_is_empty(const struct idr *idr)
{
	return radix_tree_empty(&idr->idr_rt) &&
		radix_tree_tagged(&idr->idr_rt, IDR_FREE);
}

/*
 * idr_preload_end - end preload section started with idr_preload()
 *
 * Each idr_preload() should be matched with an invocation of this
 * function. See idr_preload() for details.
 */
static inline void idr_preload_end(void)
{
	preempt_enable();
}

/**
 * idr_for_each_entry() - Iterate over an IDR's elements of a given type.
 * @idr: IDR handle.
 * @entry: The type * to use as cursor
 * @id: Entry ID.
 *
 * @entry and @id do not need to be initialized before the loop, and
 * after normal termination @entry is left with the value NULL.  This
 * is convenient for a "not found" value.
 */
#define idr_for_each_entry(idr, entry, id)			\
	for (id = 0; ((entry) = idr_get_next(idr, &(id))) != NULL; ++id)

/**
 * idr_for_each_entry_ul() - Iterate over an IDR's elements of a given type.
 * @idr: IDR handle.
 * @entry: The type * to use as cursor.
 * @id: Entry ID.
 *
 * @entry and @id do not need to be initialized before the loop, and
 * after normal termination @entry is left with the value NULL.  This
 * is convenient for a "not found" value.
 */
#define idr_for_each_entry_ul(idr, entry, id)			\
	for (id = 0; ((entry) = idr_get_next_ul(idr, &(id))) != NULL; ++id)

/**     
 * idr_for_each_entry_continue() - Continue iteration over an IDR's elements of a given type
 * @idr: IDR handle.
 * @entry: The type * to use as a cursor.
 * @id: Entry ID.
 *      
 * Continue to iterate over entries, continuing after the current position.
 */
#define idr_for_each_entry_continue(idr, entry, id)			\
	for ((entry) = idr_get_next((idr), &(id));			\
		entry;							\
		++id, (entry) = idr_get_next((idr), &(id)))

extern int idr_alloc(struct idr *idr, void *ptr, int start, int end, gfp_t gfp);
extern int idr_alloc_u32(struct idr *idr, void *ptr, unsigned int *nextid,
			unsigned long max, gfp_t gfp);
extern int idr_alloc_cyclic(struct idr *idr, void *ptr, int start, 
			int end, gfp_t gfp);
extern void *idr_remove(struct idr *idr, unsigned long id);
extern void *idr_find(const struct idr *idr, unsigned long id);
extern int idr_for_each(const struct idr *idr,
		int (*fn)(int id, void *p, void *data), void *data);
extern void *idr_get_next(struct idr *idr, int *nextid);
extern void *idr_get_next_ul(struct idr *idr, unsigned long *nextid);
extern void *idr_replace(struct idr *idr, void *ptr, unsigned long id);

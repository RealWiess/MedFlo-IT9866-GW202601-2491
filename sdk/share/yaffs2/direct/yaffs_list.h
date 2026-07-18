/*
 * YAFFS: Yet Another Flash File System. A NAND-flash specific file
 * system.
 *
 * Copyright (C) 2002-2021 Aleph One Ltd.
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This release of YAFFS is for commercial use by commercially licensed
 * customers only. It may be used under the terms of your agreement with
 * Aleph One Ltd. and is limited to use in products for which the
 * customer is licensee, unless the customer has received an unlimited
 * licence from Aleph One Ltd., in which case it may be used in any
 * product.
 * 
 * In the event that this code is used outside the terms of a current,
 * valid and paid agreement with Aleph One Ltd, the code is subject to
 * the GPL version3 license under which the open source code is
 * distributed.
 * 
 * This release was created on 2025-04-18.
 *
 */

/*
 * This file is just holds extra declarations of macros that would normally
 * be providesd in the Linux kernel. These macros have been written from
 * scratch but are functionally equivalent to the Linux ones.
 *
 */

#ifndef __YAFFS_LIST_H__
#define __YAFFS_LIST_H__


/*
 * This is a simple doubly linked list implementation that matches the
 * way the Linux kernel doubly linked list implementation works.
 */

struct list_head {
	struct list_head *next; /* next in chain */
	struct list_head *prev; /* previous in chain */
};


/* Initialise a static list */
#ifdef LIST_HEAD
#undef LIST_HEAD
#endif

#define LIST_HEAD(name) \
struct list_head name = { &(name), &(name)}



/* Initialise a list head to an empty list */
#define INIT_LIST_HEAD(p) \
do { \
	(p)->next = (p);\
	(p)->prev = (p); \
} while (0)


/* Add an element to a list */
static inline void list_add(struct list_head *new_entry,
				struct list_head *list)
{
	struct list_head *list_next = list->next;

	list->next = new_entry;
	new_entry->prev = list;
	new_entry->next = list_next;
	list_next->prev = new_entry;

}

static inline void list_add_tail(struct list_head *new_entry,
				 struct list_head *list)
{
	struct list_head *list_prev = list->prev;

	list->prev = new_entry;
	new_entry->next = list;
	new_entry->prev = list_prev;
	list_prev->next = new_entry;

}


/* Take an element out of its current list, with or without
 * reinitialising the links.of the entry*/
static inline void list_del(struct list_head *entry)
{
	struct list_head *list_next = entry->next;
	struct list_head *list_prev = entry->prev;

	list_next->prev = list_prev;
	list_prev->next = list_next;

}

static inline void list_del_init(struct list_head *entry)
{
	list_del(entry);
	entry->next = entry->prev = entry;
}


/* Test if the list is empty */
static inline int list_empty(struct list_head *entry)
{
	return (entry->next == entry);
}


/* list_entry takes a pointer to a list entry and offsets it to that
 * we can find a pointer to the object it is embedded in.
 */


#define list_entry(entry, type, member) \
	((type *)((char *)(entry)-(unsigned long)(&((type *)NULL)->member)))


/* list_for_each and list_for_each_safe  iterate over lists.
 * list_for_each_safe uses temporary storage to make the list delete safe
 */

#define list_for_each(itervar, list) \
	for (itervar = (list)->next; itervar != (list); itervar = itervar->next)

#define list_for_each_safe(itervar, save_var, list) \
	for (itervar = (list)->next, save_var = (list)->next->next; \
		itervar != (list); \
		itervar = save_var, save_var = save_var->next)


#endif

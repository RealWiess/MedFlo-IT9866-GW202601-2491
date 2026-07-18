#include <stddef.h>
#include <ith/ith_utility.h>

/**
 * Pushes the given node to the front of the list. If the list was
 * previously empty (i.e. had no items in it), this new node is
 * both the first and last node in the list. If the list already
 * had one or more items in it, the new node becomes the first node
 * in the list, and the previous first node (which was now the
 * second node) becomes the second node.
 *
 * @param[in] list The list to add the node to.
 * @param[in] node The node to add to the list.
 */
void ithListPushFront (ITHList * list, void * node)
{
    ITHList * listNode = (ITHList *)node;

    if (list->next == NULL)
    {
        /* This is the first item we add */
        list->prev = listNode;
        /* Make sure the new node is not linked to any existing nodes */
        listNode->next = NULL;
    }
    else
    {
        /* We already have items in the list */
        /* The new node becomes the first node in the list */
        listNode->next = list->next;
        /* The original first node becomes the second node */
        listNode->next->prev = listNode;
    }
    /* Make the list's next pointer point to the new node */
    list->next = listNode;
    /* The new node's previous pointer points back to the list */
    listNode->prev = list;
}

/**
 * Adds the given node to the end of the list. If the list was
 * previously empty (i.e. had no items in it), this new node is
 * both the first and last node in the list. If the list already
 * had one or more items in it, the new node becomes the last node
 * in the list, and the previous last node becomes the new node's
 * previous node.
 *
 * @param[in] list The list to add the node to.
 * @param[in] node The node to add to the list.
 */
void ithListPushBack (ITHList * list, void * node)
{
    ITHList * listNode = (ITHList *)node;

    if (list->prev == NULL)
    {
        /* This is the first item we add */
        /* The list's next pointer points to the new node */
        list->next = listNode;
        /* The new node's previous pointer points back to the list */
        listNode->prev = list;
    }
    else
    {
        /* We already have items in the list */
        /* The new node's previous pointer points to the previous last node */
        listNode->prev = list->prev;
        /* The node that was previously the last node in the list's next
         * pointer points to the new node */
        listNode->prev->next = listNode;
    }
    /* The list's previous pointer points to the new node */
    list->prev = listNode;
    /* The new node becomes the last node in the list, so its next
     * pointer is NULL */
    listNode->next = NULL;
}


void ithListInsertBefore (ITHList * list, void * listNode, void * node)
{
    ITHList * theListNode   = (ITHList *)listNode;
    ITHList * the_node      = (ITHList *)node;

    the_node->prev          = theListNode->prev;
    the_node->next          = theListNode;
    theListNode->prev->next = the_node;
    theListNode->prev       = the_node;
}

void ithListInsertAfter (ITHList * list, void * listNode, void * node)
{
    ITHList * theListNode = (ITHList *)listNode;
    ITHList * the_node    = (ITHList *)node;

    the_node->next        = theListNode->next;
    the_node->prev        = theListNode;

    if (theListNode->next)
    {
        theListNode->next->prev = the_node;
    }
    else
    {
        list->prev = the_node;
    }
    theListNode->next = the_node;
}

/**
 * Remove the given node from the list.
 *
 * @param list Pointer to the list to operate on.
 * @param node Pointer to the node to be removed.
 *
 * @return None.
 */
void ithListRemove (ITHList * list, void * node)
{
    ITHList * listNode = (ITHList *)node;

    /* If the list is empty or the node to be removed is NULL, do nothing */
    if (list->next == NULL || listNode == NULL)
    {
        return;
    }

    /* If the node to be removed is the last node in the list, simply set the
     * list's prev pointer to the node's previous pointer. This effectively
     * removes the node from the list. */
    if (list->prev == listNode)
    {
        list->prev = (listNode->prev == list) ? NULL : listNode->prev;
    }

    /* If the node to be removed is not the last node in the list, then the
     * node's next pointer must be non-NULL. Set the node's previous pointer's
     * next pointer to the node's next pointer. This effectively removes the
     * node from the list without changing the list's prev pointer. */
    if (listNode->next != NULL)
    {
        listNode->next->prev = listNode->prev;
    }

    /* Set the node's previous pointer's next pointer to the node's next
     * pointer. This completes the removal of the node from the list. */
    listNode->prev->next = listNode->next;
}

void ithListClear (ITHList * list, void (*dtor)(void * node))
{
    if (dtor != NULL)
    {
        ITHList *q;

        q = list->next;
        while (NULL != q)
        {
            ITHList *p;

            p = q;
            q = q->next;
            dtor(p);
        }
    }
    list->next = list->prev = NULL;
}

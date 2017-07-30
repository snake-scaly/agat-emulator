/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "tsq.h"

#include <assert.h>

/* Initialize a thread-safe queue. */
int tsq_init(struct TSQ *tsq)
{
	list_init(&tsq->queue);
	tsq->mutex = CreateMutex(NULL, FALSE, NULL);
	return tsq->mutex == NULL;
}

/*
	Free resources held by the queue.
	The queue must be empty. Freeing a non-empty queue is likely
	to result in memory leaks.
*/
void tsq_uninit(struct TSQ *tsq)
{
	assert(list_empty(&tsq->queue));
	CloseHandle(tsq->mutex);
}

/* Add an item to the end of the queue. */
void tsq_add(struct TSQ *tsq, struct LIST_NODE *new_node)
{
	WaitForSingleObject(tsq->mutex, INFINITE);
	list_insert(new_node, &tsq->queue);
	ReleaseMutex(tsq->mutex);
}

/*
	Remove the first element in the queue and return it.
	Returns NULL if the queue is empty.
*/
struct LIST_NODE* tsq_take(struct TSQ *tsq)
{
	struct LIST_NODE* result = NULL;
	WaitForSingleObject(tsq->mutex, INFINITE);
	if (!list_empty(&tsq->queue)) {
		result = tsq->queue.next;
		list_remove(result);
	}
	ReleaseMutex(tsq->mutex);
	return result;
}

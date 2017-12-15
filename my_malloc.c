/*
 * CS 2110 Spring 2017
 * Author: Akib bin Nizam
 */

/* we need this for uintptr_t */
#include <stdint.h>
#include <stdio.h>
/* we need this for memcpy/memset */
#include <string.h>
/* we need this for my_sbrk */
#include "my_sbrk.h"
/* we need this for the metadata_t struct and my_malloc_err enum definitions */
#include "my_malloc.h"

/* You *MUST* use this macro when calling my_sbrk to allocate the
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* This is the size of the metadata struct and canary footer in bytes */
#define TOTAL_METADATA_SIZE (sizeof(metadata_t) + sizeof(int))

/* This is the minimum size of a block in bytes, where it can
 * store the metadata, canary footer, and at least 1 byte of data
 */
#define MIN_BLOCK_SIZE (TOTAL_METADATA_SIZE + 1)

/* Used in canary calcuations. See the "Block Allocation" section of the
 * homework PDF for details.
 */
#define CANARY_MAGIC_NUMBER 0xE629

/* Feel free to delete this (and all uses of it) once you've implemented all
 * the functions
 */
#define UNUSED_PARAMETER(param) (void)(param)


#define CALCAN(x) ((unsigned int) ((uintptr_t)x ^ CANARY_MAGIC_NUMBER) - x->size)

/* Our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t *freelist;

metadata_t *createSpace(int amount);
void merger(metadata_t *newSpace);
metadata_t *findBlock(size_t space);


/* Set on every invocation of my_malloc()/my_free()/my_realloc()/
 * my_calloc() to indicate success or the type of failure. See
 * the definition of the my_malloc_err enum in my_malloc.h for details.
 * Similar to errno(3).
 */
enum my_malloc_err my_malloc_errno;

/* MALLOC
 * See my_malloc.h for documentation
 */

metadata_t *createSpace(int amount) {
	metadata_t *newSpace = (metadata_t *) my_sbrk(amount);
	if (newSpace == NULL)
	{
		return NULL;
	}
	newSpace->size = amount;
	newSpace->next = NULL;
	return newSpace;
}


void merger(metadata_t *newSpace) {
	if (freelist == NULL) {
    	freelist = newSpace;
    	return;
    }
	metadata_t *parent = freelist;
	metadata_t *temp = freelist;

	int count = 0;

	metadata_t *keeper = newSpace;

	unsigned int *keepBegin = (unsigned int *) ((uint8_t *)newSpace);
	unsigned int *keepEnd = (unsigned int *) ((uint8_t *)newSpace + newSpace->size);

	//bool merged = false;

	while (temp != NULL) {
		unsigned int *adBack = (unsigned int *) ((uint8_t *)temp + temp->size);
		unsigned int *adFront = (unsigned int *) ((uint8_t *)temp);

		if (adBack == keepBegin || adFront == keepEnd)
		{
			if (count == 0)
			{
				freelist = freelist->next;
				parent = freelist;
				count--;
			} else {
				parent->next = temp->next;
			}
			//metadata_t *secTemp = temp;
			if (adBack == keepBegin)
			{
				temp->size = temp->size + keeper->size;
				keeper = temp;
			} else {
				keeper->size = temp->size + keeper->size;
			}
			//temp->next = keeper->next;
			//keeper = temp;

			//temp = temp->next;
		}

		if (count > 0)
		{
			parent = temp;
		}

		if (temp == NULL)
		{
			break;
		}
		temp = temp->next;
		count++;
	}

	parent = freelist;
	temp = freelist;
	count = 0;

	while (temp != NULL) {
		if (temp == NULL || temp->size > keeper->size)
		{
			break;
		}

		if (count != 0)
		{
			parent = temp;
		}
		temp = temp->next;
		count++;
	}



	if (count == 0)
	{
		keeper->next = freelist;
		freelist = keeper;
	} else {
		keeper->next = parent->next;
		parent->next = keeper;
	}
}


metadata_t *findBlock(size_t space) {
	metadata_t *parent = NULL;
	metadata_t *temp = freelist;
	int count = 0;

	while (temp != NULL) {
		if (temp->size >= space) {
			unsigned int dif = temp->size - space;

			if (dif == 0 || dif < MIN_BLOCK_SIZE) {
				if (count == 0)
				{
					freelist = freelist->next;
				} else {
					parent->next = temp->next;
				}
			} else {
				metadata_t *remain = (metadata_t *)((uint8_t *)temp + space);
				remain->size = temp->size - space;
				remain->next = temp->next;
				if (count == 0)
				{
					freelist = remain;
				} else {
					parent->next = remain;
				}
				temp->size = space;
			}

			temp->canary = (unsigned int) ((uintptr_t)temp ^ CANARY_MAGIC_NUMBER) - temp->size;
			unsigned int *tail_canary = (unsigned int *) ((uint8_t *)temp + temp->size - sizeof(int));
			*tail_canary = temp->canary;

			return temp;
		}

		parent = temp;
		temp = temp->next;
		count++;
	}

	metadata_t *ttt = createSpace(SBRK_SIZE);
	if (ttt == NULL)
	{
		return NULL;
	}

	merger(ttt);
	return findBlock(space);
}




void *my_malloc(size_t size) {
	if (size == 0) {
		my_malloc_errno = NO_ERROR;
		return NULL;
	}
    size_t space = size + TOTAL_METADATA_SIZE;
    if (space > SBRK_SIZE) {
    	my_malloc_errno = SINGLE_REQUEST_TOO_LARGE;
    	return NULL;
    }

    if (freelist == NULL) {
    	freelist = createSpace(SBRK_SIZE);
    	if (freelist == NULL)
    	{
    		my_malloc_errno = OUT_OF_MEMORY;
    		return NULL;
    	}
    }

    metadata_t *mySpace = findBlock(space);

    if (mySpace == NULL)
    {
    	my_malloc_errno = OUT_OF_MEMORY;
    	return NULL;
    }

    void *point = (void *) ((uint8_t *)mySpace + sizeof(metadata_t));
    my_malloc_errno = NO_ERROR;
    return point;
}

/* REALLOC
 * See my_malloc.h for documentation
 */
void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
    	return my_malloc(size);
    }

    if (size == 0)
    {
    	my_free(ptr);
    }

    metadata_t *tt = (metadata_t *) ((uint8_t *)ptr - sizeof(metadata_t));

    unsigned int *tail = (unsigned int *) ((uint8_t *)tt + tt->size - sizeof(int));

    if (tt->canary != CALCAN(tt) || tt->canary != *tail)
    {
    	my_malloc_errno = CANARY_CORRUPTED;	
    	return NULL;
    }

    int min = tt->size < size ? tt->size : size;

    char *temp = (char *) my_malloc(size);
    if (temp == NULL)
    {
    	return NULL;
    }
    char *pp = (char *)ptr;

    for (int i = 0; i < min; ++i)
    {
    	temp[i] = pp[i];
    }

    my_free(ptr);

    return (void *) temp;
    
}

/* CALLOC
 * See my_malloc.h for documentation
 */
void *my_calloc(size_t nmemb, size_t size) {
    return my_malloc(nmemb * size);
}

/* FREE
 * See my_malloc.h for documentation
 */
void my_free(void *ptr) {
	if (ptr == NULL)
	{
		my_malloc_errno = NO_ERROR;
		return;
	}

    metadata_t *temp = (metadata_t *) ((uint8_t *)ptr - sizeof(metadata_t));
    
    unsigned int *tail = (unsigned int *) ((uint8_t *)temp + temp->size - sizeof(int));

    if (temp->canary != CALCAN(temp) || temp->canary != *tail)
    {
    	my_malloc_errno = CANARY_CORRUPTED;	
    	return;
    }

    my_malloc_errno = NO_ERROR;
    merger(temp);
}

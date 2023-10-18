#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * BEFORE GETTING STARTED:
 *
 * Familiarize yourself with the functions and constants/variables
 * in the following included files.
 * This will make the project a LOT easier as you go!!
 *
 * The diagram in Section 4.1 (Specification) of the handout will help you
 * understand the constants in mm.h
 * Section 4.2 (Support Routines) of the handout has information about
 * the functions in mminline.h and memlib.h
 */
#include "./memlib.h"
#include "./mm.h"
#include "./mminline.h"

block_t *prologue;
block_t *epilogue;
// block_t *curr_block;

// rounds up to the nearest multiple of WORD_SIZE
static inline long align(long size) {
    return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) {
    flist_first = NULL;
    prologue = mem_sbrk(TAGS_SIZE);
    if ((prologue == (void *)-1)) {
        return -1;
    }
    block_set_size_and_allocated(prologue, TAGS_SIZE, 1);
    epilogue = mem_sbrk(TAGS_SIZE);
    if (epilogue == (void *)-1) {
        return -1;
    }
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    return 0;
}
/*
extend heap is a function that given a size, will add a new free block
onto the heap of that size.
arguments: size, a long that dictates the size extended
output: the new block
*/
block_t *extend_heap(long size) {
    block_t *temp = epilogue;
    void *extra_mem = mem_sbrk(size);
    block_set_size_and_allocated(temp, size, 0);

    epilogue = block_next(temp);
    block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);

    return temp;
}

/*     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * allocates a block of memory and returns a pointer to that block's payload
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred
 */
void *mm_malloc(long size) {
    // TODO
    // step 1: search through the list
    long aligned_size = align(size) + TAGS_SIZE;
    if (flist_first == NULL) {
        insert_free_block(extend_heap(align(416)));
    }

    block_t *insert_point = NULL;
    block_t *start = flist_first;

    do {
        if (block_size(start) >= aligned_size) {
            insert_point = start;
            break;
        } else {
            start = block_flink(start);
        }
    } while (start != flist_first);

    if (insert_point == NULL) {
        if (!block_prev_allocated(epilogue)) {
            long new_size = aligned_size - block_size(block_prev(epilogue));
            insert_point = block_prev(epilogue);
            extend_heap(new_size);
            block_set_size(insert_point, aligned_size);

        } else {
            insert_point = extend_heap(aligned_size);
            insert_free_block(insert_point);
        }
    }
    // step 2: split
    if (block_size(insert_point) - aligned_size >= aligned_size) {
        block_set_size_and_allocated(
            insert_point, block_size(insert_point) - aligned_size, 0);
        block_set_size_and_allocated(block_next(insert_point), aligned_size, 1);
        return block_next(insert_point)->payload;

    } else {
        pull_free_block(insert_point);
        block_set_allocated(insert_point, 1);
        return insert_point->payload;
    }
}

/*
    coalesce takes in a block and checks if either the preceding or next
    block is free. If they are, it merges all relvant blocks
    argument: block, of tyoe block_t
    output: the merged block
*/
block_t *mm_coalesce(block_t *block) {
    block_set_allocated(block, 0);
    block_t *n = block_next(block);
    block_t *p = block_prev(block);

    if (block_allocated(n) == 0) {
        pull_free_block(n);
        block_set_size_and_allocated(block, block_size(n) + block_size(block),
                                     0);
    }
    if (block_allocated(p) == 0) {
        pull_free_block(p);
        block_set_size_and_allocated(p, block_size(p) + block_size(block), 0);
        block = p;
    }
    return block;
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */

void mm_free(void *ptr) {
    block_t *block = payload_to_block(ptr);
    insert_free_block(mm_coalesce(block));
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new payload size
 * returns: a pointer to the new memory block's payload
 */
void *mm_realloc(void *ptr, long size) {
    block_t *block = payload_to_block(ptr);
    long aligned_size = align(size) + TAGS_SIZE;
    block_t *n = block_next(block);
    block_t *p = block_prev(block);

    if (ptr == NULL) {
        return mm_malloc(size);
    } else if (size == 0) {
        mm_free(ptr);
        return NULL;
    } else if (block_size(block) > aligned_size) {
        // split if possible
        if (block_size(block) - aligned_size >= aligned_size) {
            long og_block_size = block_size(block);
            long free_size = og_block_size - aligned_size;

            block_set_size_and_allocated(block, block_size(block) - free_size,
                                         1);
            block_set_size_and_allocated(block_next(block), free_size, 0);
            insert_free_block(block_next(block));
            return ptr;
        } else {
            return ptr;
        }
    } else {
        void *new_b = mm_malloc(size);
        memcpy(new_b, ptr, block_size(block) - 16);
        mm_free(ptr);
        return new_b;
    }
}

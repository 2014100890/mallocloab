#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h> 
#include "mm.h"
#include "memlib.h"

team_t team = {
    /* Team name */
    "Hi",
    /* First member's full name */
    "Hong",
    /* First member's email address */
    "sajiho@naver.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

#define WSIZE 4
#define DSIZE 8 
#define CHUNKSIZE (1<<12)
#define MAX(x, y) ((x)>(y)?(x):(y))
#define PACK(size, alloc) ((size)|(alloc))
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))
#define GET_SIZE(p) (GET(p)&~0x7)
#define GET_ALLOC(p) (GET(p)& 0x1)
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)-DSIZE)))

static char *heap_listp = NULL; 
static char *next_ptr = NULL;

static void *coalesce(void *bp); 
static void *extend_heap(size_t words);
static void *find_fit(size_t asize); 
static void place(void *bp, size_t asize); 

int mm_init(void){
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1; 
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE); 
    next_ptr = heap_listp; 
    if (extend_heap(CHUNKSIZE/WSIZE)== NULL) // 못넓힐 때
        return -1; 
    return 0; 
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size; 

    size = words * WSIZE; 

    if ((long)(bp = mem_sbrk(size))== -1)
        return NULL; 

    PUT(HDRP(bp), PACK(size, 0)); // epilog block 헤드 
    PUT(FTRP(bp), PACK(size, 0)); 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); 

    return coalesce(bp); 
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); 
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));  
    size_t size = GET_SIZE(HDRP(bp)); 

    if (prev_alloc && next_alloc){
        return bp; 
    }

    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); 
        PUT(HDRP(bp), PACK(size, 0)); 
        PUT(FTRP(bp), PACK(size, 0)); 
    }

    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); 
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
        bp = PREV_BLKP(bp); 
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); 
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); 
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp); 
    }

    if ((next_ptr > (char *) bp) && (next_ptr < NEXT_BLKP(bp)))
        next_ptr = bp;
    return bp; 
}

void *mm_malloc(size_t size){
    size_t asize; 
    size_t extendsize; 
    char *bp; 

    if (size == 0)
        return NULL; 

    if (size <= DSIZE)
        asize = 2*DSIZE; 
    
    else{
        asize = DSIZE * ((size + DSIZE + DSIZE -1)/DSIZE); 
    }

    if ((bp = find_fit(asize))!= NULL){
        place(bp, asize); 
        return bp; 
    }

    extendsize = MAX(asize, CHUNKSIZE); 
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    
    place(bp, asize); 
    return bp; 
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp)); 
    if ((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1)); 
        PUT(FTRP(bp), PACK(asize, 1)); 
        bp = NEXT_BLKP(bp); 
        PUT(HDRP(bp), PACK(csize-asize, 0));  
        PUT(FTRP(bp), PACK(csize-asize, 0)); 
    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));  
        PUT(FTRP(bp), PACK(csize, 1)); 
    }
}

static void *find_fit(size_t asize){
    // void *bp; 
    // for (bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)){
    //     if (!GET_ALLOC(HDRP(bp)) && (asize<= GET_SIZE(HDRP(bp)))){
    //         return bp; 
    //     }
    // }

    char *bp = next_ptr; 
    for( ; GET_SIZE(HDRP(next_ptr))>0; next_ptr = NEXT_BLKP(next_ptr)){
        if (!GET_ALLOC(HDRP(next_ptr)) && (asize<= GET_SIZE(HDRP(next_ptr)))){
            return next_ptr; 
        }
    }

    for(next_ptr = heap_listp; next_ptr < bp; next_ptr = NEXT_BLKP(next_ptr)){
        if (!GET_ALLOC(HDRP(next_ptr)) && (asize<= GET_SIZE(HDRP(next_ptr)))){
            return next_ptr; 
        }
    }
    return NULL;
}

void *mm_realloc(void *ptr, size_t size){
    if(size == 0){
        mm_free(ptr); 
        return 0; 
    }
    if (ptr == NULL){
        return mm_malloc(size); 
    }
    void *newptr = mm_malloc(size); 
    if (newptr == NULL){ 
        return 0; 
    }
    size_t old_size = GET_SIZE(HDRP(ptr)); 
    if (size < old_size){
        old_size = size; 
    }
    memcpy(newptr, ptr, old_size); 
    mm_free(ptr); 
    return newptr; 
}

void mm_free(void *ptr){
    if (!ptr) return; 
    size_t size = GET_SIZE(HDRP(ptr)); 
    PUT(HDRP(ptr), PACK(size, 0)); 
    PUT(FTRP(ptr), PACK(size, 0)); 
    coalesce(ptr);
}
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20171628",
    /* Your full name*/
    "HyunWoo Kim",
    /* Your email address */
    "kimhw0820@sogang.ac.kr",
};

#define NEXT_FIT
#define TESTx

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */            
#define DSIZE 8                                                      
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))              
#define PUT(p, val) (*(unsigned int *)(p) = (val)) 

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) 
#define GET_ALLOC(p) (GET(p) & 0x1) 

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)                        
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) 
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))   
/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0; /* Pointer to first block */
#ifdef NEXT_FIT
static char *allocPointer; /* Next fit allocPointer */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);

int mm_init(void)
{
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) // line:vm:mm:begininit
        return -1;
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2 * WSIZE);                     // line:vm:mm:endinit
    /* $end mminit */

#ifdef NEXT_FIT
    allocPointer = heap_listp;
#endif
    /* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}



void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)     
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE); // line:vm:mm:sizeadjust3

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {                     
        place(bp, asize); 
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE); // line:vm:mm:growheap1
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;  
    place(bp, asize); 
#ifdef TEST
    mm_checkheap(0);
#endif
    return bp;
}


void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
#ifdef TEST
    mm_checkheap(0);
#endif
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    { /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc)
    { /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc)
    { /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else
    { /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

#ifdef NEXT_FIT
    if ((allocPointer > (char *)bp))
        allocPointer = bp;
#endif
    return bp;
}


void *mm_realloc(void *ptr, size_t size)
{
    size_t prevsize;
    void *newptr;

    prevsize = GET_SIZE(HDRP(ptr));
    // 들어온 사이즈가 작다면 
    if (size < prevsize)
        size = prevsize;

    /*malloc 에 오류가 발생할 때 realloc이 fail 했음을 알림*/
    if(!(newptr = mm_malloc(size)))
        return NULL;

    /* Copy the old data. */
    memcpy(newptr, ptr, prevsize);

    /* Free the old block. */
    mm_free(ptr);
#ifdef TEST
    mm_checkheap(0);
#endif
    return newptr;
}

/*
 * checkheap - We don't check anything right now.
 */
void mm_checkheap(int verbose)
{
    int allocFlag = 0;
    for (char *bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        // alloc 이 되어 있지 않고
        // 주소 check
        if (bp < heap_listp || bp > heap_listp + MAX_HEAP)
        {
            fprintf(stdout, "location out of range between %p ~ %p! location : %p\n", heap_listp, heap_listp + MAX_HEAP, bp);
            exit(1);
        }
        // 주소 check

        // 1번 앞에가 alloc 이 되어있지 않고 자신도 alloc 이 아니라면 오류 방출

        if (!GET_ALLOC(HDRP(bp)) && !allocFlag)
        {
            fprintf(stdout, "coalescing failed! location : %p\n", bp);
            exit(1);
        }

        // 앞뒤로 비교
        if (GET_ALLOC(HDRP(bp)) != GET_ALLOC(FTRP(bp)))
        {
            fprintf(stdout, "alloc pair check failed! location : %p\n", bp);
            exit(1);
        }

        if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp)))
        {
            fprintf(stdout, "size pair check failed! location : %p\n", bp);
            exit(1);
        }
        // 앞뒤로 비교

        if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp)))
        {
            fprintf(stdout, "size pair check failed! location : %p\n", bp);
            exit(1);
        }

        if(bp != heap_listp)
        {
            if(GET_SIZE(bp - WSIZE * 2) != GET_SIZE(FTRP(PREV_BLKP(bp))))
            {
                fprintf(stdout, "malloc overrun pair check failed! location : %p\n", bp);
                exit(1);
            }        
        }


        allocFlag = GET_ALLOC(HDRP(bp));
    }
}

/*
 * The remaining routines are internal helper routines
 */

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL; // line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */           // line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */           // line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ // line:vm:mm:newepihdr

    /* Coalesce if the previous block was free */
    return coalesce(bp); // line:vm:mm:returnblock
}
/* $end mmextendheap */

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
/* $end mmplace */

static void *find_fit(size_t asize)
/* $end mmfirstfit-proto */
{
    /* $end mmfirstfit */

#ifdef NEXT_FIT
    /* Next fit search */
    char *temp = allocPointer;

    /* Search from the allocPointer to the end of list */

    for (; GET_SIZE(HDRP(allocPointer)) > 0; allocPointer = NEXT_BLKP(allocPointer))
    {
        // alloc 이 되어 있지 않고  현 블록의 사이즈가 새로 들어올 블럭의 사이즈 보다 작은 경우
        if (!GET_ALLOC(HDRP(allocPointer)) && asize <= GET_SIZE(HDRP(allocPointer)))
        {
            return allocPointer;
        }

    }

    allocPointer = heap_listp;

    for (; allocPointer < temp; allocPointer = NEXT_BLKP(allocPointer))
    {
        // alloc 이 되어 있지 않고
        if (!GET_ALLOC(HDRP(allocPointer)) && asize <= GET_SIZE(HDRP(allocPointer)))
        {
            return allocPointer;
        }
    }

    return NULL; /* no fit found */
#endif
#ifdef FRIST_FIT
    /* $begin mmfirstfit */
    /* First fit search */
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }
    return NULL; /* No fit */
#endif
#ifdef BEST_FIT
    void *ret = NULL;
    void *bp;
    // asize <= GET_SIZE(HDRP(bp))) 할당할 사이즈가 현재 보고 있는 사이즈 보다 작은 경우
    // !GET_ALLOC(HDRP(bp)) 할당할 공간이라는 의미

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        // 할당이 가능할 시에
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
        {
            if (ret)
            {
                if (GET_SIZE(HDRP(bp)) < GET_SIZE(HDRP(ret)))
                    ret = bp;
            }
            else
            {
                ret = bp;
            }
        }
    }
    return ret; /* No fit */
/* $end mmfirstfit */
#endif
}
#if STRATEGY != 0

#include "brk.h"
#include <unistd.h>
#include <string.h> 
#include <errno.h> 
#include <sys/mman.h>

#define NALLOC 1024             /* minimum #units to request */

typedef long Align;             /* for alignment to long boundary */

union header {                  /* block header */
    struct {
        union header *ptr;      /* next block if on free list */
        unsigned size;          /* size of this block  - what unit? */ 
    } s;
    Align x;                    /* force alignment of blocks */
};

typedef union header Header;

static Header base;             /* empty list to get started */
static Header *freep = NULL;    /* start of free list */

/* free: put block ap in the free list */

void free(void * ap) {
    Header *bp, *p;

    if(ap == NULL)
        return;                                                 /* Nothing to do */

    bp = (Header *) ap - 1;                                     /* point to block header */
    for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
            break;                                              /* freed block at atrt or end of arena */

    if(bp + bp->s.size == p->s.ptr) {                           /* join to upper nb */
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else {
        bp->s.ptr = p->s.ptr;
    }

    if(p + p->s.size == bp) {                                   /* join to lower nbr */
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else {
        p->s.ptr = bp;
    }
}

/* morecore: ask system for more memory */

#ifdef MMAP
static void * __endHeap = 0;

void * endHeap(void) {
    if(__endHeap == 0)
        __endHeap = sbrk(0);
    return __endHeap;
}
#endif


static Header *morecore(unsigned nu) {
    void *cp;
    Header *up;

    #ifdef MMAP
    unsigned noPages;
    if(__endHeap == 0)
        __endHeap = sbrk(0);
    #endif

    if(nu < NALLOC)
        nu = NALLOC;

    #ifdef MMAP
    noPages = ((nu*sizeof(Header))-1)/getpagesize() + 1;
    cp = mmap(__endHeap, noPages*getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    nu = (noPages*getpagesize())/sizeof(Header);
    __endHeap += noPages*getpagesize();
    #else
    cp = sbrk(nu*sizeof(Header));
    #endif

    if(cp == (void *) -1){                      /* no space at all */
        perror("failed to get more memory");
        return NULL;
    }

    up = (Header *) cp;
    up->s.size = nu;
    free((void *)(up+1));

    return freep;
}

void * malloc(size_t nbytes) {
    Header *p, *prevp;                                      /* p and prevp are for stepping through the list*/
    Header *chosenp = NULL, *prechosenp = NULL;             /* chosenp and prechosenp are for marking the chosen pointer */
    Header * morecore(unsigned);
    unsigned nunits;

    if(nbytes == 0)                                         /* Nothing to allocate, return null */
        return NULL;

    nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) +1;   /* Calculate how many header units to allocate */

    if((prevp = freep) == NULL) {                           /* start looking at beginning of list */
        base.s.ptr = freep = prevp = &base;                 /* choose list-start as base */
        base.s.size = 0;
    }

    #ifndef STRATEGY
    #define STRATEGY 1
    #endif

    if ( !(STRATEGY == 1 || STRATEGY == 3) )                /* we only support first fit and worst fit */
        return NULL;

    int found = 0;

    for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {      /* go through all empty areas */
        if(p->s.size >= nunits) {                           /* big enough */
            if (STRATEGY == 1) {                            /* first fit */
                chosenp = p;                                /* choose first empty area that is big enough */
                prechosenp = prevp;                         /* remember the area preceding the chosen */
                break;
            } else if (STRATEGY == 3) {                     /* worst fit */
                if (!found || p->s.size > chosenp->s.size) {/* if this is the first found empty area, or it is larger than the previously largest */
                    chosenp = p;                            /* choose this area */
                    prechosenp = prevp;                     /* remember the area preceding the chosen */
                }
                found = 1;                                  /* make a note that a big enough area has been found */
            }
        }
        if(p == freep) {                                    /* wrapped around free list */
            if (STRATEGY == 3 && found)                     /* checked all empty areas and found at least one large enough */
                break;
            if((p = morecore(nunits)) == NULL)              /* wrapped around free list without finding a big enough empty area, so ask for more */
                return NULL;                                /* no memory left */
        }                                      
    }
    if (chosenp == NULL)                                    /* Did not find empty area */
        return NULL;
    if (chosenp->s.size == nunits) {                        /* chosen area is exactly the correct size */
        prechosenp->s.ptr = chosenp->s.ptr;                 /* reroute the next-pointer of the previous area to the next area */
    } else {                                                /* allocate tail end */
        chosenp->s.size -= nunits;                          /* make empty area smaller */
        chosenp += chosenp->s.size;                         /* move to location where memory is to be allocated */
        chosenp->s.size = nunits;                           /* set size of allocated memory header */
    }
    return (void *)(chosenp+1);                             /* return the address of chosen memory area */

    
}

void *realloc(void *ptr, size_t nbytes) {
    void *new_ptr = malloc(nbytes);                                 /* allocate new memory  */

    if (ptr == NULL || new_ptr == NULL)                             /* If trying to realloc a null pointer just return the new pointer. If allocation failed, return null */
        return new_ptr;

    Header *header_ptr = (Header *) ptr - 1;                        /* get header of old pointer */
    size_t old_size = header_ptr->s.size * sizeof(header_ptr);      /* get size of old memory block */
    size_t copy_size = nbytes > old_size ? old_size : nbytes;       /* get min of new and old block */
    memcpy(new_ptr, ptr, copy_size);                                /* copy from old to new memory block */
    free(ptr);                                                      /* free the old memory block */
    return new_ptr;
}

#endif
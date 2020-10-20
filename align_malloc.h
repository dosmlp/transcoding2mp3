#pragma once

/*****************************************************************************
* align_malloc
*
* This function allocates 'size' bytes (usable by the user) on the heap and
* takes care of the requested 'alignment'.
* In order to align the allocated memory block, the xvid_malloc allocates
* 'size' bytes + 'alignment' bytes. So try to keep alignment very small
* when allocating small pieces of memory.
*
* NB : a block allocated by xvid_malloc _must_ be freed with xvid_free
*      (the libc free will return an error)
*
* Returned value : - NULL on error
*                  - Pointer to the allocated aligned block
*
****************************************************************************/

void * align_malloc(unsigned int size, unsigned int alignment)
{
    unsigned char * mem_ptr;
    unsigned char * tmp;

    if(!alignment) alignment=4;
    if ((tmp = (unsigned char *) malloc(size + alignment)) != NULL) {

        /* Align the tmp pointer */
        mem_ptr =
            (unsigned char *) ((unsigned int) (tmp + alignment - 1) &
            (~(unsigned int) (alignment - 1)));

        /* Special case where malloc have already satisfied the alignment
        * We must add alignment to mem_ptr because we must store
        * (mem_ptr - tmp) in *(mem_ptr-1)
        * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
        * to a forbidden memory space */
        if (mem_ptr == tmp)
            mem_ptr += alignment;

        /* (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
        * the real malloc block allocated and free it in xvid_free */
        *(mem_ptr - 1) = (unsigned char) (mem_ptr - tmp);
        //PRT("Alloc mem addr: 0x%08x, size:% 8d, file:%s <line:%d>, ", tmp, size, file, line);
        /* Return the aligned pointer */
        return ((void *)mem_ptr);
    }


    return(NULL);
}

/*****************************************************************************
* align_free
*
* Free a previously 'xvid_malloc' allocated block. Does not free NULL
* references.
*
* Returned value : None.
*
****************************************************************************/

void align_free(void *mem_ptr)
{

    unsigned char *ptr;
    if (mem_ptr == NULL)
        return;

    /* Aligned pointer */
    ptr = (    unsigned char *)mem_ptr;

    /* *(ptr - 1) holds the offset to the real allocated block
    * we sub that offset os we free the real pointer */
    ptr -= *(ptr - 1);

    /* Free the memory */
    free(ptr);
}

/*
    FreeRTOS V8.0.1 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * A sample implementation of pvPortMalloc() and vPortFree() that combines
 * (coalescences) adjacent memory blocks as they are freed, and in so doing
 * limits memory fragmentation.
 *
 * See heap_1.c, heap_2.c and heap_3.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */

#include <stdio.h>
#include <stdlib.h> 
#include "typedef.h"
#include "zbar.h"
typedef size_t mem_size_t;
/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#define configTOTAL_HEAP_SIZE				( ( size_t ) 40*1024*1024 )


#define portBYTE_ALIGNMENT		32
#if( portBYTE_ALIGNMENT == 32 || portBYTE_ALIGNMENT == 16)
	#define portBYTE_ALIGNMENT_MASK ( portBYTE_ALIGNMENT-1 )
#endif

#ifndef portPOINTER_SIZE_TYPE
	#define portPOINTER_SIZE_TYPE uint32_t
#endif


#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( heapSTRUCT_SIZE * 2 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* A few bytes might be lost to byte aligning the heap start address. */
#define heapADJUSTED_HEAP_SIZE	( configTOTAL_HEAP_SIZE - portBYTE_ALIGNMENT )

/* Allocate the memory for the heap. */
//static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*
 * Called automatically to setup the required heap structures the first time
 * pvPortMalloc() is called.
 */
static void prvHeapInit( void );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const uint16_t heapSTRUCT_SIZE	= ( ( sizeof ( BlockLink_t ) + ( portBYTE_ALIGNMENT - 1 ) ) & ~portBYTE_ALIGNMENT_MASK );

/* Ensure the pxEnd pointer will end up on the correct byte alignment. */
static size_t xTotalHeapSize;// = ( ( size_t ) heapADJUSTED_HEAP_SIZE ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining;// = ( ( size_t ) heapADJUSTED_HEAP_SIZE ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK );
static size_t xMinimumEverFreeBytesRemaining;// = ( ( size_t ) heapADJUSTED_HEAP_SIZE ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK );

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/


size_t xMallocCounter = 0;
size_t xFreeCounter = 0;

extern int heap_start;

extern void *heap_ptr;

extern void *RAMSIZE;

extern unsigned int mem_get_free(void);


void *pvPortMalloc( size_t xWantedSize )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;

#if 0

    //pvReturn = malloc_(xWantedSize);
	//pvReturn = vdec_aligned_malloc(xWantedSize, 32);
	//pvReturn = vdec_malloc(xWantedSize);
	pvReturn = vdec_aligned_malloc(xWantedSize, 64);
   
	if (pvReturn == NULL)
    {
	    //extern void vApplicationMallocFailedHook( void );
		//vApplicationMallocFailedHook();
		deg("\n\n [ENTER] %s, Line = %d, xFreeBytesRemaining = %d, xTotalHeapSize = %d, xWantedSize = %d\n\n",__func__, __LINE__, mem_get_free(), xTotalHeapSize, xWantedSize);
		//deg("\n\n [ENTER] %s, Line = %d, xMallocCounter = %d, xFreeCounter = %d\n\n",__func__, __LINE__, xMallocCounter, xFreeCounter);
		deg("\n\n [ENTER] %s, Line = %d, xMallocCounter = %d, xFreeCounter = %d, diff = %d \n\n",__func__, __LINE__, xMallocCounter, xFreeCounter, xMallocCounter - xFreeCounter);	
	    //deg("\n\n\n [ENTER] %s, Line = %d, xFreeBytesRemaining = %d, xTotalHeapSize = %d \n\n\n",__func__, __LINE__, xFreeBytesRemaining, xTotalHeapSize);
	    //deg("\n\n\n [ENTER] %s, Line = %d, xMallocCounter = %d, xFreeCounter = %d\n\n\n",__func__, __LINE__, xMallocCounter, xFreeCounter);
    }
    else
    {
        xMallocCounter++;
		//mtCOVERAGE_TEST_MARKER();
	}

	if ((pvReturn > RAMSIZE) || (pvReturn < heap_start))
	{
	    //deg("\n\n\n pvPortMalloc, pvReturn = 0x%x \n\n\n", pvReturn);
	}
	
	return pvReturn;

	//deg("\n\n\n [ENTER] %s, Line = %d, xMallocCounter = %d, xFreeCounter = %d\n\n\n",__func__, __LINE__, xMallocCounter, xFreeCounter);
#endif
	//printf("Ms:%d\r\n",xWantedSize);
	//vTaskSuspendAll();
	{
		/* If this is the first call to malloc then the heap will require
		initialisation to setup the list of free blocks. */
		if( pxEnd == NULL )
		{
			prvHeapInit();
		}
		else
		{
	//		mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( xWantedSize > 0 )
			{
				xWantedSize += heapSTRUCT_SIZE;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
				}
				else
				{
		//			mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
		//		mtCOVERAGE_TEST_MARKER();
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + heapSTRUCT_SIZE );
					//printf("pvReturn:%x\r\n",pvReturn);
					if(pvReturn == 0x170){
							printf("err");
					}

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
					//printf("pxPre:%x %x pb:%x %x\r\n",pxPreviousBlock,pxPreviousBlock->pxNextFreeBlock,pxBlock,pxBlock->pxNextFreeBlock);
					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;
						//printf("pxNewBlockLink:%x\r\n",pxNewBlockLink);
						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
	//					mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
//						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
//					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
//				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
//			mtCOVERAGE_TEST_MARKER();
		}

//		traceMALLOC( pvReturn, xWantedSize );
	}
//	( void ) xTaskResumeAll();

	//#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			//extern void vApplicationMallocFailedHook( void );
			//vApplicationMallocFailedHook();
			printf("\n\n [ENTER] %s, Line = %d, xFreeBytesRemaining = %d, xTotalHeapSize = %d, xWantedSize = %d\n\n",__func__, __LINE__, xFreeBytesRemaining, xTotalHeapSize, xWantedSize);
			//deg("\n\n [ENTER] %s, Line = %d, xMallocCounter = %d, xFreeCounter = %d\n\n",__func__, __LINE__, xMallocCounter, xFreeCounter);
//			deg("\n\n [ENTER] %s, Line = %d, xMallocCounter = %d, xFreeCounter = %d, diff = %d \n\n",__func__, __LINE__, xMallocCounter, xFreeCounter, xMallocCounter - xFreeCounter);
		}
		else
		{
//		    xMallocCounter++;
//			mtCOVERAGE_TEST_MARKER();
		}
	}
	//#endif
	//printf("M:%x W:%d\r\n",pvReturn,xWantedSize);
	return pvReturn;
}
/*-----------------------------------------------------------*/

unsigned int vPortFree( void *pv )
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;

#if 0
    if (pv != NULL)
	{
        xFreeCounter++;
    }
	
    //return free_(pv);
    //return vdec_free(pv);
	vdec_aligned_free(pv);
	
    return 0;
#endif	
	if((pv > 0x18800000) && (pv < 0x20000000)){
		return ;
	}

	if( pv != NULL )
	{
	    xFreeCounter++;
			
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= heapSTRUCT_SIZE;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
//		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
//		configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

//				vTaskSuspendAll();
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
//					traceFREE( pv, pxLink->xBlockSize );
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
//				( void ) xTaskResumeAll();
			}
			else
			{
//				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
//			mtCOVERAGE_TEST_MARKER();
		}
	}

	return 0;
}

size_t get_heap4_size(void *pv)
{
	uint8_t *puc = ( uint8_t * ) pv;
	BlockLink_t *pxLink;
	size_t size_r;
	if( pv != NULL )
	{	
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= heapSTRUCT_SIZE;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				//pxLink->xBlockSize &= ~xBlockAllocatedBit;
				size_r = (pxLink->xBlockSize&~xBlockAllocatedBit);
				size_r = size_r - 32;
				{
					/* Add this block to the list of free blocks. */				
					//return pxLink->xBlockSize;
					return size_r;
				}
			}
			else
			{
			
			}
		}
		else
		{

		}
	}

	return 0;	

}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* This just exists to keep the linker quiet. */
}
/*-----------------------------------------------------------*/

extern int __heap_start;
extern int __heap_end;

static void prvHeapInit( void )
{
BlockLink_t *pxFirstFreeBlock;
uint8_t *pucHeapEnd, *pucAlignedHeap;

	/* add by jornny */
	unsigned int heap_start = (unsigned int)&__heap_start;
	unsigned int heap_end = (unsigned int)&__heap_end;

	xTotalHeapSize =  ( ( size_t ) (heap_end - heap_start) ) & ( ( size_t ) ~portBYTE_ALIGNMENT_MASK );
	xFreeBytesRemaining = xTotalHeapSize;
	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	
	//printf("> OS heap_start-- = 0x%x\n", heap_start);
	//printf("> OS heap_end-- = 0x%x\n", heap_end);
	printf("> OS heap_size-- = 0x%x\n", (u32)xTotalHeapSize); 
	memset(heap_start,0,xTotalHeapSize);

	/* Ensure the heap starts on a correctly aligned boundary. */
	pucAlignedHeap = ( uint8_t * ) ( ( ( portPOINTER_SIZE_TYPE )(heap_start + portBYTE_ALIGNMENT - 1) ) & ( ( portPOINTER_SIZE_TYPE ) ~portBYTE_ALIGNMENT_MASK ) );
	
	/* xStart is used to hold a pointer to the first item in the list of free
	blocks.  The void cast is used to prevent compiler warnings. */
	xStart.pxNextFreeBlock = ( void * ) pucAlignedHeap;
	xStart.xBlockSize = ( size_t ) 0;

	/* pxEnd is used to mark the end of the list of free blocks and is inserted
	at the end of the heap space. */
	pucHeapEnd = pucAlignedHeap + xTotalHeapSize;
	pucHeapEnd -= heapSTRUCT_SIZE;
	pxEnd = ( void * ) pucHeapEnd;
//	configASSERT( ( ( ( uint32_t ) pxEnd ) & ( ( uint32_t ) portBYTE_ALIGNMENT_MASK ) ) == 0UL );
	pxEnd->xBlockSize = 0;
	pxEnd->pxNextFreeBlock = NULL;

	/* To start with there is a single free block that is sized to take up the
	entire heap space, minus the space taken by pxEnd. */
	pxFirstFreeBlock = ( void * ) pucAlignedHeap;
	pxFirstFreeBlock->xBlockSize = xTotalHeapSize - heapSTRUCT_SIZE;
	pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

	/* The heap now contains pxEnd. */
	xFreeBytesRemaining -= heapSTRUCT_SIZE;

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}
	
	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
//		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
//		mtCOVERAGE_TEST_MARKER();
	}
}

unsigned int mem_get_free(void)
{
	return xFreeBytesRemaining;
}

uint32_t get_mem_size(void *mem)
{
	return 1;
}


void *mem_trim(void *mem, unsigned int newsize)
{
	//printf("miss %s\n", __FUNCTION__);
	return NULL;
}


void *mem_calloc_ex(mem_size_t size, mem_size_t count)
{
	void *p;

	/* allocate 'count' objects of size 'size' */
	p = pvPortMalloc(  count *size);
	if (p) {
		/* zero the memory */
		memset(p, 0, count *size);
	} 

	return p;
}





#if 0
void dma_memcpy(void *dst, void *src, int cnt)
{
#if 0
	WritebackDCacheRegion((u32)src,cnt);
	FlushDCacheRegion((u32)dst,cnt);

	MCPCON |= BIT(0); 	//en
	MCPCON &= ~BIT(2);	//no sir
	
	MCPSCADDR = (u32)src;
	MCPTGADDR = (u32)dst;
	MCPLENGTH = cnt-1;

	while(!(MCPCON & BIT(1)));	
#endif

#if 1

	if(cnt <=32)
	{
		BtB_memcpy(dst,src,cnt);
		return;
	}


	WritebackDCacheRegion((u32)src,cnt);
	FlushDCacheRegion((u32)dst,cnt);

//	mutex_lock(Dma_mutex);
	
	while(!(MCPCON & BIT(1)));
	MCPCON |= BIT(0); 	//en
	MCPCON &= ~BIT(2);	//no sir
	MCPSCADDR = (u32)src;
	MCPTGADDR = (u32)dst;
	MCPLENGTH = cnt-1;	
	while(!(MCPCON & BIT(1)));


#endif
#if 0
    UINT32 dwcount = 0;
    UINT32 dwlen = 0;
    UINT32 *dwdestbuf = NULL;
    UINT32 *dwsrcbuf = NULL;
    UINT16 *hfdestbuf = NULL;
    UINT16 *hfsrcbuf = NULL;
    UINT8 *destbuf = NULL;
    UINT8 *srcbuf = NULL;
    UINT8 *destbufbyte = NULL;
    UINT8 *srcbufbyte = NULL;

    if(!((UINT32)dst & 0x3) && !((UINT32)src & 0x3) && (0x03 < cnt))
    {
        dwlen = (cnt & 0xfffffffc) >> 2;
        dwdestbuf = (UINT32 *)dst;
        dwsrcbuf = (UINT32 *)src;
        for(dwcount=0; dwcount < dwlen; dwcount++)
        {
            *(dwdestbuf+dwcount) = *(dwsrcbuf+dwcount);
        }
        dwlen = cnt & 0x3;
        destbufbyte = (UINT8 *)(dst + (dwcount<<2));
        srcbufbyte = (UINT8 *)(src +(dwcount<<2));
        for(dwcount=0; dwcount < dwlen; dwcount++)
        {
            *(destbufbyte+dwcount) = *(srcbufbyte+dwcount);
        }
    }
    else if(!((UINT32)dst & 0x1) && !((UINT32)src & 0x1) && (0x01 < cnt))
    {
        dwlen = (cnt & 0xfffffffe) >> 1;
        hfdestbuf=(UINT16 *)dst;
        hfsrcbuf=(UINT16 *)src;
        for(dwcount=0; dwcount < dwlen; dwcount++)
        {
            *(hfdestbuf+dwcount) = *(hfsrcbuf+dwcount);
        }
        dwlen = cnt & 0x1;
        destbufbyte = (UINT8 *)(dst +(dwcount << 1));
        srcbufbyte = (UINT8 *)(src + (dwcount << 1));
        for(dwcount=0; dwcount<dwlen; dwcount++)
        {
            *(destbufbyte+dwcount)=* (srcbufbyte+dwcount);
        }
    }
    else
    {
        destbuf = (UINT8 *)dst,
        srcbuf = (UINT8 *)src;

        for (dwcount = 0; dwcount < cnt; dwcount++)
        {
            *(destbuf + dwcount) = *(srcbuf + dwcount);
        }
    }
#endif	
    return;
}

#endif

/*******************************************************************************
* Function Name  : my_memcpy
* Description    : copy data from src to dst
* Input          : *dst: destination pointer
*                  *src: source pointer
*                  cnt :length
* Output         : None
* Return         : None
*******************************************************************************/
void my_memcpy(void *dst, void *src, int cnt)
{
	BYTE *pDstTemp = (BYTE *)dst;
	BYTE *pSrcTemp = (BYTE *)src;
	while (cnt--)
	{
		*pDstTemp++ = *pSrcTemp++;
	}
}


void* port_realloc(void *p, size_t sz) {
	void *pt;
	int size; 
    size = get_heap4_size(p);
	//printf("%s %x %d %d\r\n",__func__,p,size,sz);
    if(size == 0){
		//printf("%s %d\r\n",__func__,__LINE__);
        return mem_malloc(sz);
    }
    if(size < sz){
		
		//printf("%s %d [%d]\r\n",__func__,__LINE__,sz);
		pt = mem_malloc(sz);
		memcpy(pt,p,size);
        vPortFree(p);
		//printf("%s %d\r\n",__func__,__LINE__);
		return pt;
    }
	//printf("%s %d\r\n",__func__,__LINE__);
    return p;

}


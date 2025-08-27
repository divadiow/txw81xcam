/*------------------------------------------------------------------------*/
/* Sample code of OS dependent controls for FatFs                         */
/* (C)ChaN, 2012                                                          */
/*------------------------------------------------------------------------*/

#include <stdlib.h>		/* ANSI process controls */
#include <stdio.h>

#include "ff.h"
#include "csi_kernel.h"
#include "osal/string.h"
//#include "osal/osal_sema.h"
//#include "osal/osal_alloc.h"



#if FF_FS_REENTRANT
/*------------------------------------------------------------------------*/
/* Create a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to create a new
   synchronization object, such as semaphore and mutex. When a 0 is returned,
   the f_mount() function fails with FR_INT_ERR.
*/

int ff_cre_syncobj (	/* 1:Function succeeded, 0:Could not create due to any error */
	BYTE vol,			/* Corresponding logical drive being processed */
	FF_SYNC_t *sobj		/* Pointer to return the created sync object */
)
{
	
	int ret;
	static FF_SYNC_t sem[FF_VOLUMES];	/* FreeRTOS */
	//_os_printf("%s vol:%d\t%X\n",__FUNCTION__,vol,sem[vol]);

	//*sobj = CreateMutex(NULL, FALSE, NULL);		/* Win32 */
	//ret = (*sobj != INVALID_HANDLE_VALUE);

//	*sobj = SyncObjects[vol];		/* uITRON (give a static created sync object) */
    //	ret = 1;						/* The initial value of the semaphore must be 1. */
    
    //	*sobj = OSMutexCreate(0, &err);	/* uC/OS-II */
    //	ret = (err == OS_NO_ERR);
    
      if (!sem[vol])					/* FreeRTOS */
	  	{
			sem[vol] = csi_kernel_mutex_new();
			
	  	}
    	*sobj = sem[vol];
    
    	//_os_printf("cre:%X\n",sem[vol]);
    	ret = (*sobj != NULL);

	return ret;
}



/*------------------------------------------------------------------------*/
/* Delete a Synchronization Object                                        */
/*------------------------------------------------------------------------*/
/* This function is called in f_mount() function to delete a synchronization
/  object that created with ff_cre_syncobj function. When a 0 is returned,
/  the f_mount() function fails with FR_INT_ERR.
*/

int ff_del_syncobj (	/* 1:Function succeeded, 0:Could not delete due to any error */
	FF_SYNC_t sobj		/* Sync object tied to the logical drive to be deleted */
)
{
	int ret;


	//ret = CloseHandle(sobj);	/* Win32 */

//	ret = 1;					/* uITRON (nothing to do) */

//	OSMutexDel(sobj, OS_DEL_ALWAYS, &err);	/* uC/OS-II */
//	ret = (err == OS_NO_ERR);

	//ret = 1;					/* FreeRTOS (nothing to do) */
	//_os_printf("del:%X\t%X\n",sobj,&sobj);
	ret = 1;					/* FreeRTOS (nothing to do) */

	return ret;

}



/*------------------------------------------------------------------------*/
/* Request Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on entering file functions to lock the volume.
/  When a 0 is returned, the file function fails with FR_TIMEOUT.
*/

int ff_req_grant (	/* 1:Got a grant to access the volume, 0:Could not get a grant */
	FF_SYNC_t sobj	/* Sync object to wait */
)
{
	int ret;

	//ret = (WaitForSingleObject(sobj, _FS_TIMEOUT) == WAIT_OBJECT_0);	/* Win32 */

//	ret = (wai_sem(sobj) == E_OK);				/* uITRON */

//	OSMutexPend(sobj, _FS_TIMEOUT, &err));		/* uC/OS-II */
//	ret = (err == OS_NO_ERR);

	//ret = (xSemaphoreTake(sobj, FF_FS_TIMEOUT) == pdTRUE);	/* FreeRTOS */
	//_os_printf("A:%X\t%X\n",sobj,&sobj);
	ret = csi_kernel_mutex_lock(sobj,FF_FS_TIMEOUT, 0);
	//_os_printf("B\n");
	if(!ret)
	{
		ret = 1;
	}
	else
	{
		ret = 0;
	}

	return ret;
}



/*------------------------------------------------------------------------*/
/* Release Grant to Access the Volume                                     */
/*------------------------------------------------------------------------*/
/* This function is called on leaving file functions to unlock the volume.
*/

void ff_rel_grant (
	FF_SYNC_t sobj	/* Sync object to be signaled */
)
{
	//ReleaseMutex(sobj);		/* Win32 */

//	sig_sem(sobj);			/* uITRON */

//	OSMutexPost(sobj);		/* uC/OS-II */

	//xSemaphoreGive(sobj);	/* FreeRTOS */
	//_os_printf("C\n");
	csi_kernel_mutex_unlock(sobj);
	//_os_printf("D\n");
}

#endif




#if FF_USE_LFN == 3	/* LFN with a working buffer on the heap */
/*------------------------------------------------------------------------*/
/* Allocate a memory block                                                */
/*------------------------------------------------------------------------*/
/* If a NULL is returned, the file function fails with FR_NOT_ENOUGH_CORE.
*/

void* ff_memalloc (	/* Returns pointer to the allocated memory block */
	UINT msize		/* Number of bytes to allocate */
)
{
	//_os_printf("msize:%d\r\n",msize);
	return os_malloc(msize);	/* Allocate a memory block with POSIX API */
}


/*------------------------------------------------------------------------*/
/* Free a memory block                                                    */
/*------------------------------------------------------------------------*/


void ff_memfree (
	void* mblock	/* Pointer to the memory block to free */
)
{
	os_free(mblock);
}

#endif

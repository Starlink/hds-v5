/*
*+
*  Name:
*     dat1HandleLock

*  Purpose:
*     Manage the lock on a Handle.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     int dat1HandleLock( Handle *handle, int oper, int recurs,
*                         int rdonly, int *status );

*  Arguments:
*     handle = Handle * (Given)
*        Handle to be checked.
*     oper = int (Given)
*        Operation to be performed:
*
*        1 - Return information about the current locks on the supplied Handle.
*
*            0: unlocked;
*            1: locked for writing by the current thread;
*            2: locked for writing by another thread;
*            3: locked for reading by the current thread (other threads
*               may also have a read lock on the Handle);
*            4: locked for reading by one or more other threads (the
*               current thread does not have a read lock on the Handle);
*
*            Any child Handles are always ignored.
*        2 - Lock the handle for read-write or read-only use by the current
*            thread. Returns 0 if the requested lock conflicts with an
*            existing lock (in which case the request to lock the supplied
*            Handle is ignored) and +1 otherwise. Any child Handles within
*            the supplied Handle are left unchanged unless "recurs" is
*            non-zero.
*        3 - Unlock the handle. If the current thread has a lock - either
*            read-write or read-only - on the Handle, it is removed.
*            Otherwise the Handle is left unchanged. Any child Handles
*            within the supplied Handle are left unchanged unless "recurs"
*            is non-zero. A value of +1 is always returned.
*
*     recurs = int (Given)
*        Only used if "oper" is 2 or 3, and the returned value is non-zero
*        (that is, after a successful lock or unlock operation). If "recurs"
*        is zero, the supplied Handle is the only Handle to be modified -
*        any child Handles are left unchanged. If "recurs" is non-zero, any
*        child Handles contained within the supplied Handle are locked or
*        unlocked recursively, in addition to locking or unlocking the
*        supplied Handle. Any child Handles that cannot be locked or
*        unlocked and left unchanged. The returned value is not affected
*        by the value of "recurs".
*     rdonly = int (Given)
*        Only used if "oper" is 2. It indicates if the new lock is for
*        read-only or read-write access.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned function value:
*     The values returned for each operation are included in the
*     description of the "oper" argument above.

*  Description:
*     This function locks or unlocks the supplied Handle for read-only or
*     read-write access. The Handle is always in one of the following
*     three mutually exclusive states:
*
*     - Unlocked
*     - Locked for read-write access by one and only one thread.
*     - Locked for read-only access by one or more threads.
*
*     When locked for read-write access, the locking thread has exclusive
*     read-write access to the object attached to the supplied handle. When
*     locked for read-only access, the locking thread shares read-only
*     access with zero or more other threads.
*
*     A request to lock or unlock an object can be propagated recursively
*     to all child objects by setting "recurs" non-zero. However, if the
*     attempt to lock or unlock a child fails, no error will be reported
*     and the child will be left unchanged.

*  Notes:
*     - If a thread gets a read-write lock on the handle, and
*     subsequently attempts to get a read-only lock, the existing
*     read-write lock will be demoted to a read-only lock.
*     - If a thread gets a read-only lock on the handle, and
*     subsequently attempts to get a read-write lock, the existing
*     read-only lock will be promoted to a read-write lock only if
*     there are no other locks on the Handle.
*     - "oper" values of -2 and -3 are used in place of 2 and 3 when
*     calling this function recursively from within itself.
*     - A value of zero is returned if an error has already ocurred, or
*     if this function fails for any reason.

*  Authors:
*     DSB: David S Berry (DSB)
*     {enter_new_authors_here}

*  History:
*     10-JUL-2017 (DSB):
*        Initial version
*     19-JUL-2017 (DSB):
*        Added argument rdonly.
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2017 East Asian Observatory.
*     All Rights Reserved.

*  Licence:
*     Redistribution and use in source and binary forms, with or
*     without modification, are permitted provided that the following
*     conditions are met:
*
*     - Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*
*     - Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials
*       provided with the distribution.
*
*     - Neither the name of the {organization} nor the names of its
*       contributors may be used to endorse or promote products
*       derived from this software without specific prior written
*       permission.
*
*     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
*     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
*     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
*     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
*     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
*     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
*     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
*     USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
*     AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*     IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
*     THE POSSIBILITY OF SUCH DAMAGE.

*  Bugs:
*     {note_any_bugs_here}
*-
*/


#include <pthread.h>
#include "sae_par.h"
#include "dat1.h"
#include "ems.h"
#include "dat_err.h"

/* The initial size for the array holding the identifiers for the threads
   that have a read lock on a handle. It is also the incremement in size
   when the array needs to be extended. */
#define NTHREAD 10

int dat1HandleLock( Handle *handle, int oper, int recurs, int rdonly,
                    int *status ){

/* Local Variables; */
   int ichild;
   int result = 0;
   int top_level;
   pthread_t *locker;
   pthread_t *rlocker;
   int i;
   int j;

/* Check inherited status. */
   if( *status != SAI__OK ) return result;

/* To avoid deadlocks, we only lock the Handle mutex for top level
   entries to this function. If "oper" is negative, negate it and set a
   flag indicating we do not need to lock the mutex. */
   if( oper < 0 ) {
      oper = -oper;
      top_level = 0;
   } else {
      top_level = 1;
   }

/* For top-level entries to this function, we need to ensure no other thread
   is modifying the details in the handle, so attempt to lock the handle's
   mutex. */
   if( top_level ) pthread_mutex_lock( &(handle->mutex) );

/* Return information about the current lock on the supplied Handle.
   ------------------------------------------------------------------ */
   if( oper == 1 ) {

/* Default: unlocked */

      if( handle->nwrite_lock ) {
         if( pthread_equal( handle->write_locker, pthread_self() )) {

/* Locked for writing by the current thread. */
            result = 1;
         } else {

/* Locked for writing by another thread. */
            result = 2;
         }

      } else if( handle->nread_lock ){

/* Locked for reading by one or more other threads (the current thread does
   not have a read lock on the Handle). */
         result = 4;

/* Now check to see if the current thread has a read lock, changing the
   above result value if it does. */
         locker = handle->read_lockers;
         for( i = 0; i < handle->nread_lock;i++,locker++ ) {
            if( pthread_equal( *locker, pthread_self() )) {

/* Locked for reading by the current thread (other threads may also have
   a read lock on the Handle). */
               result = 3;
               break;
            }
         }
      }




/* Lock the handle for use by the current thread.
   ------------------------------------------------------------------ */
   } else if( oper == 2 ) {

/* A read-only lock requested.... */
      if( rdonly ) {

/* If the current thread has a read-write lock on the Handle, demote it
   to a read-only lock and return 1 (success). In this case, we know
   there will be no other read-locks. Otherwise if any other thread has
   read-write lock, return zero (failure). */
         if( handle->nwrite_lock ) {
            if( pthread_equal( handle->write_locker, pthread_self() )) {

/* If we do not have an array in which to store read lock thread IDs,
   allocate one now with room for NTHREAD locks. It will be extended as
   needed. */
               if( !handle->read_lockers ) {
                  handle->read_lockers = MEM_CALLOC(NTHREAD,sizeof(pthread_t));
                  if( !handle->read_lockers ) {
                     *status = DAT__NOMEM;
                     emsRep( "", "Could not allocate memory for HDS "
                             "Handle read locks list.", status );
                  }
               }

/* If we now have an array, store the current thread in the first element. */
               if( handle->read_lockers ) {
                  handle->read_lockers[ 0 ] = pthread_self();
                  handle->nread_lock = 1;
                  handle->nwrite_lock = 0;
                  result = 1;
               }
            }

/* If there is no read-write lock on the Handle, add the current thread
   to the list of threads that currently have a read-only lock, but only
   if it is not already there. */
         } else {

/* Set "result" to 1 if the current thread already has a read-only lock. */
            locker = handle->read_lockers;
            for( i = 0; i < handle->nread_lock;i++ ) {
               if( pthread_equal( *locker, pthread_self() )) {
                  result = 1;
                  break;
               }
            }

/* If not, extend the read lock thread ID array if necessary, and append
   the current thread ID to the end. */
            if( result == 0 ) {
               handle->nread_lock++;
               if( handle->maxreaders < handle->nread_lock ) {
                  handle->maxreaders += NTHREAD;
                  handle->read_lockers = MEM_REALLOC( handle->read_lockers,
                                                    handle->maxreaders*sizeof(pthread_t));
                  if( !handle->read_lockers ) {
                     *status = DAT__NOMEM;
                     emsRep( "", "Could not reallocate memory for HDS "
                             "Handle read locks list.", status );
                  }
               }

               if( handle->read_lockers ) {
                  handle->read_lockers[ handle->nread_lock - 1 ] = pthread_self();

/* Indicate the read-only lock was applied successfully. */
                  result = 1;
               }
            }
         }

/* A read-write lock requested. */
      } else {

/* If there are currently no locks of any kind, apply the lock. */
         if( handle->nread_lock == 0 ) {
            if( handle->nwrite_lock == 0 ) {
               handle->write_locker = pthread_self();
               handle->nwrite_lock = 1;
               result = 1;

/* If the current thread already has a read-write lock, indicate success. */
            } else if( pthread_equal( handle->write_locker, pthread_self() )) {
               result = 1;
            }

/* If there is currently only one read-only lock, and it is owned by the
   current thread, then promote it to a read-write lock. */
         } else if( handle->nread_lock == 1 &&
                    pthread_equal( handle->read_lockers[0], pthread_self() )) {
            handle->nread_lock = 0;
            handle->write_locker = pthread_self();
            handle->nwrite_lock = 1;
            result = 1;
         }
      }

/* If required, and if the above lock operation was successful, lock any
   child handles that can be locked. */
      if( result && recurs ){
         for( ichild = 0; ichild < handle->nchild; ichild++ ) {
            (void) dat1HandleLock( handle->children[ichild], -2, 1,
                                   rdonly, status );
         }
      }





/* Unlock the handle.
   ----------------- */
   } else if( oper == 3 ) {

/* Always indicate success because we should always be able to guarantee
   that the current thread does not have a lock on exit. */
      result = 1;

/* If the current thread has a read-write lock, remove it. */
      if( handle->nwrite_lock ) {
         if( pthread_equal( handle->write_locker, pthread_self() )) {
            handle->nwrite_lock = 0;
         }

/* Otherwise, if the current thread has a read-only lock, remove it. */
      } else {

/* Loop through all the threads that have read-only locks. */
         locker = handle->read_lockers;
         for( i = 0; i < handle->nread_lock; i++,locker++ ) {

/* If the current thread is found, shuffle any remaining threads down one
   slot to fill the gap left by removing the current thread from the list. */
            if( pthread_equal( *locker, pthread_self() )) {
               rlocker = locker + 1;
               for( j = i + 1; j < handle->nread_lock; j++,locker++ ) {
                  *(locker++) = *(rlocker++);
               }

/* Reduce the number of read-only locks. */
               handle->nread_lock--;
               break;
            }
         }
      }

/* If required, and if the above unlock operation was successful, unlock any
   child handles that are not currently locked by another thread. */
      if( result && recurs ){
         for( ichild = 0; ichild < handle->nchild; ichild++ ) {
            (void) dat1HandleLock( handle->children[ichild], -3, 1, 0,
                                   status );
         }
      }



/* Report an error for any other "oper" value. */
   } else if( *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf( " ", "dat1HandleLock: Unknown 'oper' value (%d) supplied - "
               "(internal HDS programming error).", status, oper );
   }

/* If this is a top-level entry, unlock the Handle's mutex so that other
   threads can access the values in the Handle. */
   if( top_level ) pthread_mutex_unlock( &(handle->mutex) );

/* Return the result. */
   return result;
}


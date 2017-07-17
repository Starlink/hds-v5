/*
*+
*  Name:
*     dat1Handle

*  Purpose:
*     Find a handle structure that describes a component of an HDF object.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     Handle *dat1Handle( const HDSLoc *parent, const char *name, int *status );

*  Arguments:
*     parent = const HDSLoc * (Given)
*        Pointer to a locator for the HDF object that contains the
*        required  component. This may be NULL if the required component
*        has no parent (i.e. the 'component' is actually a top level object).
*     name = const char * (Given)
*        The name of the HDF component within "parent" to which the
*        returned Handle should refer. It is assumed that the named
*        component does in fact exist within the parent object, although
*        this is not checked. If "parent" is NULL, the path to the
*        contained file should be supplied.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned function value:
*     Pointer to the Handle structure for the name component. A NULL
*     value will be returned if an error occurs.

*  Description:
*     A Handle structure contains information about a specific HDF object
*     (group or dataset) that is constant for all locators. Any one
*     HDF object can have multiple locators that refer to it. Each HDF
*     object that has been accessed by HDS will have one (and only one)
*     Handle structure associated with it, which contains information
*     specific to the HDF object that is shared by all locators that
*     refer to the object.

*  Notes:
*     - NULL will be returned if an error has already occurred, or if
*     this function fails for any reason.

*  Authors:
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  History:
*     5-JUL-2017 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Cornell University
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
#include <string.h>
#include <pthread.h>

#include "ems.h"
#include "sae_par.h"
#include "dat1.h"
#include "dat_err.h"

Handle *dat1Handle( const HDSLoc *parent_loc, const char *name, int * status ){


/* Local Variables; */
   char *ext;
   char *lname = NULL;
   Handle *parent;
   Handle *result = NULL;
   int ichild;

/* Return immediately if an error has already occurred. */
   if( *status != SAI__OK ) return result;

/* Get a local copy of "Name" without any trailing ".sdf". */
   if( name ) {
      lname = MEM_CALLOC( strlen( name ) + 1, sizeof(char) );
      if( !lname ) {
         *status = DAT__NOMEM;
         emsRep("dat1Handle", "Could not reallocate memory for the "
                "component name in an HDS Handle", status );
      } else {
         strcpy( lname, name );
         ext = strstr( lname, DAT__FLEXT );
         if( ext ) *ext = 0;
      }
   }

/* Get the handle for the parent object (if any). */
   parent = parent_loc ? parent_loc->handle : NULL;

/* If a parent Handle is available, search through the Handles for any
   known child objects to see if the requested component within the parent
   (identified by 'name') is already known and therefore already has an
   associated Handle structure. If it does, return a pointer to the child
   Handle structure. */
   if( parent && lname ) {
      for( ichild = 0; ichild < parent->nchild; ichild++ ) {
         if( !strcmp( parent->children[ichild]->name, lname ) ) {
            result =  parent->children[ichild];
            break;
         }
      }
   }

/* If we need to create a new Handle... */
   if( !result ) {

/* Allocate the memory, filling it with zeros (NULLs). Report an error
   if the memory could not be allocated */
      result = MEM_CALLOC( 1, sizeof(*result) );
      if( !result ) {
         *status = DAT__NOMEM;
         emsRep("dat1Handle", "Could not allocate memory for HDS Handle",
                status );

/* If the memory for the new Handle was allocated succesfully... */
      } else {

/* Create links between the new Handle and any supplied parent. */
         result->parent = parent;
         if( parent ) {
            parent->nchild++;
            parent->children = MEM_REALLOC( parent->children,
                                            parent->nchild*sizeof(Handle *) );
            if( !parent->children ) {
               *status = DAT__NOMEM;
               emsRep("dat1Handle", "Could not reallocate memory for "
                      "child links in an HDS Handle", status );
            } else {
               parent->children[ parent->nchild-1 ] = result;
            }

/* Copy other information from the parent Handle. */
            result->locked = parent->locked;
            result->locker = parent->locker;
         }

/* Store the component name. Nullify "lname" to indicate the memory is
   now part of the Handle structure and should not be freed below. */
         result->name = lname;
         lname = NULL;

/* Initialise a mutex that is used to serialise access to the values
   stored in the handle. */
         if( *status == SAI__OK &&
             pthread_mutex_init( &(result->mutex2), NULL ) != 0 ) {
            emsRep( " ", "Failed to initialise POSIX mutex2 for a new Handle.",
                    status );
         }

/* Indicate the Handle is locked for use by the current thread. */
         result->locked = 1;
         result->locker = pthread_self();
      }
   }

/* Free the local copy of the name, unless it has been transferred to a new
   Handle (in which case "lname" will be NULL). */
   if( lname ) MEM_FREE( lname );

/* If an error occurred, free the resources used by the Handle. */
   if( *status != SAI__OK ) result = dat1FreeHandle( result );

/* Return the Handle pointer */
   return result;
}



/*
*+
*  Name:
*     dat1Annul

*  Purpose:
*     Annul locator without freeing struct memory

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     dat1Annul( HDSLoc *locator, int * status );

*  Arguments:
*     locator = HDSLoc * (Given)
*        Locator to free. File resources will be freed but memory
*        associated with the struct will remain valid.
*     status = int* (Given and Returned)
*        Pointer to global status. Attempts to run even
*        if status is bad.

*  Description:
*     Free up file resources associated with a locator. Cancel the
*     association between a locator and an object.  Any primitive
*     value currently mapped to the locator is automatically unmapped.
*     The C struct itself will be zeroed (except for the version number)
*     and should be freed elsewhere.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  Notes:
*     - This routine attempts to execute even if status is set on entry,
*     although no further error report will be made if it subsequently
*     fails under these circumstances. In particular, it will fail if
*     the locator supplied is not valid, but this will only be
*     reported if status is set to SAI__OK on entry.
*     - This routine should be called when a locator is being annulled
*     without knowledge of the library user. This happens when the final
*     primary locator is annulled and the secondary locators automatically
*     get released. If datAnnul was called the memory would be freed and
*     a caller might end up with a double free.

*  History:
*     2014-08-26 (TIMJ):
*        Initial version
*     2014-08-29 (TIMJ):
*        datUnmap just in case.
*     2014-10-21 (TIMJ):
*        Try to remove locator from group if we are being asked
*        to free a locator stored in a group.
*     2014-10-30 (TIMJ):
*        Only annul the file_id if it is a primary locator.
*     2014-11-11 (TIMJ):
*        Use new error context. We need to unmap and unregister
*        even if status is bad.
*     2014-11-19 (TIMJ):
*        Cloned from datAnnul
*     2020-07-17 (DSB):
*        Re-written to avoid recursive calls to this function from within
*        hds1UnregLocator.
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Cornell University
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 3 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*     MA 02110-1301, USA.

*  Bugs:
*     {note_any_bugs_here}
*-
*/

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "hdf5.h"

#include "sae_par.h"
#include "ems.h"

#include "hds1.h"
#include "dat1.h"
#include "dat_err.h"
#include "hds.h"

static void dat1Anloc( HDSLoc *locator, int * status );

int dat1Annul( HDSLoc *locator, int * status ) {

/* Local Variables: */
   HDSLoc *loc = NULL;
   Handle *tophandle = NULL;
   HdsFile *hdsFile = NULL;
   HdsFile *context = NULL;
   hid_t file_id = 0;
   int erase = 0;
   int errstat = 0;

/* Return if a null locator is supplied, but do not check the inherited
   status. */
   if( !locator ) return *status;

/* Begin an entirely new error context as we need to run this
   regardless of external errors */
   emsBegin( status );

/* Unregister the supplied locator - this removes the locator from the
   list of locators associated with its container file and returns a flag
   indicating if there are then no remaining primary locators associated
   with the container file. If this is the case, we annull any secondary
   locators still associated with the file and close the file. */
   if( hds1UnregLocator( locator, status ) ) {

/* Loop round popping any remaining locators off the list of secondary
   locators still associated with the container file, and annulling each
   one. */
      loc = hds1PopSecLocator( locator, &context, status );
      while( loc ){
         dat1Anloc( loc, status );
         loc = hds1PopSecLocator( NULL, &context, status );
      }

/* Note if the container file should be erased after annulling the
   supplied locator. */
      erase = locator->handle->erase;

/* Get a pointer to the Handle at the top of the tree so that the tree
   can be erased after annulling the supplied locator. */
      tophandle =  dat1TopHandle( locator->handle, status );

/* Get a pointer to the HdsFile structure for the container file (do
   this now so that we can still access the HdsFile after the locator has
   been annulled). */
      hdsFile = locator->hdsFile;

/* Note the HDF5 id for the file to be closed. */
      file_id = locator->file_id;
   }

/* Annul the supplied locator. */
   dat1Anloc( locator, status );

/* If required, close the file */
   if( file_id ) {
      if( H5Fclose( file_id ) < 0 && *status == SAI__OK ) {
         *status = DAT__FATAL;
         dat1H5EtoEMS( status );
         if( hdsFile && hdsFile->path ) {
            emsRepf( " ", "dat1Annul: Failed to close file '%s'.",
                     status, hdsFile->path );
         } else {
            emsRepf( " ", "dat1Annul: Failed to close file.", status );
         }
      }
   }

/* If required, delete the file. */
   if( erase && hdsFile && hdsFile->path ) {
      errstat = unlink( hdsFile->path );
      if (*status == SAI__OK && errstat > 0) {
         *status = DAT__FILND;
         emsErrno( "ERRNO", errno );
         emsRepf( " ", "Error deleting file %s: ^ERRNO", status, hdsFile->path );
      }
   }

/* If required, erase the whole tree of handles. */
   if( tophandle ) dat1EraseHandle( tophandle, NULL, status );

/* If required, free the HdsFile structure. */
   if( hdsFile ) hdsFile = hds1FreeHdsFile( hdsFile, status );

/* End the error context and return the final status */
   emsEnd( status );

   return *status;
}




static void dat1Anloc( HDSLoc *locator, int * status ) {

/* Local Variables: */
   hdsbool_t ingrp = 0;
   int ver;

/* Return if a null locator is supplied, but do not check the inherited
   status. */
   if( !locator) return;

/* Begin an entirely new error context as we need to run this
   regardless of external errors */
   emsBegin( status );

/* Remove from group. If we do not do this then we risk a segv if someone
   later calls hdsFlush. They are not meant to call datAnnul if it is part
   of a group and maybe we should simply return without doing anything if it
   is part of a group. For now we continue but remove from the group. */
   ingrp = hds1RemoveLocator( locator, status );

/* The following code can be used to indicated whether we should be worried
   about group usage */
/*
   if( ingrp ){
      printf("ANNULING LOCATOR %p : part of group '%s'\n", *locator, (*locator)->grpname);
   }
*/

/* Sort out any memory mapping */
   datUnmap( locator, status );

/* Free HDF5 resources. We zero them out so that unregistering the locator
   does not cause confusion */
   if( locator->dtype ){
      H5Tclose( locator->dtype );
      locator->dtype = 0;
   }

   if( locator->dataspace_id ){
      H5Sclose( locator->dataspace_id );
      locator->dataspace_id = 0;
   }

   if( locator->dataset_id ){
      H5Dclose( locator->dataset_id );
      locator->dataset_id = 0;
   }

   if( locator->group_id ){
      H5Gclose( locator->group_id );
      locator->group_id = 0;
   }

/* Nullify the pointer to the structure holding lists of locators
   associated with the same container file. */
   locator->hdsFile = NULL;

/* End the error context and return the final status */
   emsEnd( status  );

/* Clear the locator but retain the version number. This is required to
   ensure that the locator is sent to the correct HDS implementation when
   it is finally freed */
   ver = locator->hds_version;
   memset( locator, 0, sizeof(*locator) );
   locator->hds_version = ver;
}





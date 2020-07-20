/*
*+
*  Name:
*     dat1Reopen

*  Purpose:
*     Reopen a file, retaining any existing locators associated with the file

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hid_t dat1Reopen( hid_t file_id, unsigned int flags, hid_t fapl,
*                       int *status )

*  Arguments:
*     file_id = hid_t (Given)
*        The file_id for the file that is to be re-opened. It should be
*        open on entry to this function.
*     flags = unsigned int (Given)
*        The new file access flags with which to reopen the file.
*     fapl = hid_t (Given)
*        The new file access properties list with which to reopen the file.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Returned function value:
*     The new file_id for the reopened file.

*  Description:
*     Any active locators associated with the supplied file, via any
*     file_id,  are recorded. The file is then closed and re-opened
*     using the supplied access flags and properties list. Any
*     recorded locators are then updated to refer to the new file_id.

*  Notes:
*     - An error is reported if any part of the supplied file is mapped
*     for access on entry.

*  Authors:
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  History:
*     8-MAY-2019 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2019 East Asian Observatory
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

#include "ems.h"
#include "ems_par.h"
#include "sae_par.h"
#include "dat1.h"
#include "dat_err.h"
#include "hds.h"
#include "hdf5.h"

hid_t dat1Reopen( hid_t file_id, unsigned int flags, hid_t fapl,
                  int *status ){

/* Local Variables; */
   HDSLoc **loc;
   HDSLoc **loclist;
   char **paths;
   char file[EMS__SZMSG+1];
   char path[EMS__SZMSG+1];
   hid_t *file_ids;
   hid_t id;
   int *isgroup;
   int iloc;
   int nlev;
   int nloc;
   ssize_t size;

/* Return immediately if an error has already occurred. */
   if( *status != SAI__OK ) return file_id;

/* Get a list of any active locators associated with the file. Also get a
   list of file_ids for the same file that have associated locators. */
   hds1GetLocators( file_id, &nloc, &loclist, &file_ids, status );

/* Check that none of the locators are mapped. */
   if( *status == SAI__OK ) {
      loc = loclist;
      for( iloc = 0; iloc < nloc; iloc++,loc++ ) {
         if( (*loc)->regpntr ) {
            hdsTrace( (*loc), &nlev, path, file, status, sizeof(path), sizeof(file) );
            *status = DAT__PRMAP;
            emsRepf( " ", "hdsOpen: Cannot re-open '%s' in read-write mode "
                     "since '%s' is currently mapped.", status, file, path );
            break;
         }
      }
   }

/* Store the path to the HDF5 object associated with each active locator,
   and also a flag indicating if the HDF5 object is a group or dataset.
   Then close the HDF5 objects. */
   paths = MEM_CALLOC( nloc, sizeof( *paths ) );
   isgroup = MEM_CALLOC( nloc, sizeof( *isgroup ) );
   if( paths && isgroup && *status == SAI__OK ) {
      loc = loclist;
      for( iloc = 0; iloc < nloc; iloc++,loc++ ) {
         if( (*loc)->group_id ) {
            isgroup[ iloc ] = 1;
            id = (*loc)->group_id;
         } else {
            isgroup[ iloc ] = 0;
            id = (*loc)->dataset_id;
         }
         if( id ) {
            size = H5Iget_name( id, NULL, 0 );
            paths[ iloc ] = MEM_CALLOC( size + 1, 1 );
            H5Iget_name( id, paths[ iloc ], size + 1 );
         } else {
            hdsTrace( (*loc), &nlev, path, file, status, sizeof(path), sizeof(file) );
            *status = DAT__FATAL;
            emsRepf( " ", "hdsOpen: Locator for '%s.%s' has no group or "
                     "dataset so cannot be reopened.", status, file, path );
            break;
         }
         if( H5Oclose( id ) < 0 ) {
            hdsTrace( (*loc), &nlev, path, file, status, sizeof(path), sizeof(file) );
            *status = DAT__FATAL;
            dat1H5EtoEMS( status );
            emsRepf( " ", "hdsOpen: Failed to close HDF5 object for '%s.%s'.",
                     status, file, path );
            break;
         }
      }
   }

/* Get the path for the file. */
   H5Fget_name( file_id, path, sizeof(path) );

/* Close all HDF5 file_ids associated with file. */
   if( *status == SAI__OK ) {
      int this_closed = 0;

      int i = -1;
      while( file_ids[ ++i ] ) {

         if( file_ids[ i ] == file_id ) this_closed = 1;

         if( H5Fclose( file_ids[ i ] ) < 0 ) {
            *status = DAT__FATAL;
            dat1H5EtoEMS( status );
            emsRepf( " ", "hdsOpen: Failed to close file '%s' prior to "
                     "re-opening it.", status, path );
            break;
         }
      }

/* Close the supplied file_id if it has not already been closed. */
      if( !this_closed ) {
         if( H5Fclose( file_id ) < 0 ) {
            *status = DAT__FATAL;
            dat1H5EtoEMS( status );
            emsRepf( " ", "hdsOpen: Failed to close file '%s' prior to "
                     "re-opening it.", status, path );
         }
      }
   }

/* Re-open it. */
   if( *status == SAI__OK ) {
      file_id = H5Fopen( path, flags, fapl );
      if( file_id < 0 ) {
         *status = DAT__FATAL;
         dat1H5EtoEMS( status );
         emsRepf( " ", "hdsOpen: Failed to reopen file '%s'.",
                  status, path );
      }
   }

/* Update the file, group and dataset id in each locator. */
   if( *status == SAI__OK ) {
      loc = loclist;
      for( iloc = 0; iloc < nloc; iloc++,loc++ ) {
         (*loc)->file_id = file_id;
         if( isgroup[ iloc ] ) {
            (*loc)->group_id = H5Gopen2( file_id, paths[ iloc ], H5P_DEFAULT );
         } else {
            (*loc)->dataset_id = H5Dopen2( file_id, paths[ iloc ], H5P_DEFAULT );
         }
      }
   }

/* Free resources. */
   if( paths ) {
      for( iloc = 0; iloc < nloc; iloc++ ) {
         MEM_FREE( paths[ iloc ] );
      }
      MEM_FREE( paths );
   }
   if( isgroup ) MEM_FREE( isgroup );
   if( loclist ) MEM_FREE( loclist );
   if( file_ids ) MEM_FREE( file_ids );

/* Return the new file id. */
   return file_id;
}

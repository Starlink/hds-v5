/*
*+
*  Name:
*     dat1CloseAllIds

*  Purpose:
*     Close all HDF5 identifiers associated with a given file id.

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     dat1CloseAllIds( hid_t file_id, int * status );

*  Arguments:
*     file_id = hid_t (Given)
*        The file id containing the objects t be closed.
*     status = int* (Given and Returned)
*        Pointer to global status. Attempts to run even if status is bad.

*  Description:
*     Close all HDF5 object identifiers currently associated with a file,
*     either throught the supplied file id or some other file id. All
*     file ids associated with the file (inclusing the supplied file id) are
*     also closed.

*  Authors:
*     DSB: David S Berry (EAO)
*     {enter_new_authors_here}

*  Notes:
*     - This routine attempts to execute even if status is set on entry,
*     although no further error report will be made if it subsequently
*     fails under these circumstances.

*  History:
*     2020-10-13 (DSB):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2020 East Asian Observatory
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


#include "hdf5.h"
#include "sae_par.h"
#include "ems.h"
#include "hds1.h"
#include "dat1.h"
#include "dat_err.h"
#include "hds.h"

int dat1CloseAllIds( hid_t file_id, int * status ){

/* Local Variables: */
   H5I_type_t objtype;
   HdsFile *hdsFile = NULL;
   herr_t herr;
   hid_t *objs;
   int howmany;
   int i;
   ssize_t cnt;

/* Return if a null file_id is supplied, but do not check the inherited
   status. */
   if( file_id <= 0 ) return *status;

/* Begin an entirely new error context as we need to run this
   regardless of external errors */
   emsBegin( status );

/* Get the number of active HDF5 identifiers of any type associated with
   the file. */
   cnt = H5Fget_obj_count( file_id, H5F_OBJ_ALL );

/* There should always be at least one active identifier (the supplied
   file identifier). */
   if( cnt == 0 ) {
      *status = DAT__FATAL;
      emsRepf( " ", "dat1CloseAllIds: No active HDF5 idenfifiers for supplied file.",
               status );

/* Alocate an array to hold the active identifiers then store the active
   identifiers in it. */
   } else {
      objs = MEM_CALLOC( cnt, sizeof(hid_t) );
      if( objs ) {
         howmany = H5Fget_obj_ids( file_id, H5F_OBJ_ALL, cnt, objs );

/* Loop round closing each one, except for the supplied file identifier. */
         for( i = 0; i < howmany; i++ ) {
            if( objs[i] != file_id ){
               objtype = H5Iget_type( objs[i] );
               if ( objtype == H5I_FILE ) {
                  herr = H5Fclose( objs[i] );
               } else if ( objtype == H5I_GROUP || objtype == H5I_DATASET ) {
                  herr = H5Oclose( objs[i] );
               } else if ( objtype == H5I_DATASPACE ) {
                  herr = H5Dclose( objs[i] );
               } else if( *status == SAI__OK ){
                  herr = 0;
                  *status = DAT__FATAL;
                  emsRepf( " ", "dat1CloseAllIds: Cannot close HDF5 idenfifier - wrong type.",
                           status );
               }

               if( herr < 0 && *status == SAI__OK ){
                  *status = DAT__FATAL;
                  dat1H5EtoEMS( status );
                  emsRepf( " ", "dat1CloseAllIds: Cannot close HDF5 idenfifier.",
                           status );
               }
            }
         }

/* Free the memory allocated above. */
         MEM_FREE( objs );
      }
   }

/* Close the supplied file id. */
   if( H5Fclose( file_id ) < 0 && *status == SAI__OK ){
      *status = DAT__FATAL;
      dat1H5EtoEMS( status );
      if( hdsFile && hdsFile->path ) {
         emsRepf( " ", "dat1CloseAllIds: Failed to close file '%s'.",
                  status, hdsFile->path );
      } else {
         emsRepf( " ", "dat1CloseAllIds: Failed to close file.", status );
      }
   }

/* End the error context and return the final status */
   emsEnd( status );

   return *status;
}


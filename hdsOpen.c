/*
*+
*  Name:
*     hdsOpen

*  Purpose:
*     Open container file

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     hdsOpen( const char * file, const char * mode, HDSLoc **locator,
*              int * status );

*  Arguments:
*     file = const char * (Given)
*        Container filename.
*     mode = const char * (Given)
*        Access mode ("READ", "UPDATE" or "WRITE")
*     locator = HDSLoc ** (Returned)
*        Object locator.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Open an existing container file for reading, writing or updating
*     and return a primary locator to the top-level object.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - A file extension of DAT__FLEXT (".h5sdf") is the default.
*     - There is no difference betwee UPDATE and WRITE mode in HDF5
*     - If the top-level object is a structure array, loc will be
*       associated with the complete array, not the first cell. Thus,
*       access to any of the structure's components can only be made
*       through another locator which is explicitly associated with an
*       individual cell (see datCell).
*     - HDS assumes a single dataset/group at the root of the hierarchy.
*       Currently hdsOpen will pick the first valid item if there is a
*       choice. In the future the HDSTYPE attribute might be examined to see
*       which of the top-level items was created by this library.

*  History:
*     2014-08-29 (TIMJ):
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

#include <unistd.h>
#include <string.h>

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

int
hdsOpen( const char *file_str, const char *mode_str,
         HDSLoc **locator, int *status) {
  char * fname = NULL;
  unsigned int flags = 0;
  hid_t file_id = 0;
  hid_t group_id = 0;
  hsize_t iposn;
  char dataset_name[DAT__SZNAM+1];
  HDSLoc *temploc = NULL;
  htri_t filstat = 0;

  *locator = NULL;
  if (*status != SAI__OK) return *status;

  /* Configure the HDF5 library for our needs as this routine could be called
     before any others. */
  dat1InitHDF5();

  /* Work out the flags for opening */
  switch (mode_str[0]) {
  case 'U':
  case 'u':
  case 'w':
  case 'W':
    flags = H5F_ACC_RDWR;
    break;
  case 'R':
  case 'r':
    flags = H5F_ACC_RDONLY;
    break;
  default:
    *status = DAT__MODIN;
  }

  /* work out the file name */
  fname = dau1CheckFileName( file_str, status );

  /* Before we go any further, check that the file really is an HDF5
     file. If it isn't then we return with a special error code that
     allows the putative wrapper library to know to fall back to HDSv4
     or whatever. */
  if (*status != SAI__OK) goto CLEANUP;
  filstat = H5Fis_hdf5( fname );
  if (filstat < 0) {
    /* Probably indicates the file is not there */
    *status = DAT__FILNF;
    emsRepf("hdsOpen_fnf", "File '%s' does not seem to exist",
           status, fname);
    goto CLEANUP;
  }

  /* Open the HDF5 file */
  CALLHDF( file_id,
           H5Fopen( fname, flags, H5P_DEFAULT ),
           DAT__HDF5E,
           emsRepf("hdsOpen_1", "Error opening HDS file: %s",
                   status, fname )
           );

  /* Now we need to find a top-level object. For now just pick the
     first */
  CALLHDF( group_id,
           H5Gopen2(file_id, "/", H5P_DEFAULT),
           DAT__HDF5E,
           emsRepf("hdsOpen_2","Error opening root group of file %s",
                  status, fname)
           );

  iposn = 0;
  CALLHDFQ( H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC,
                               iposn, dataset_name, sizeof(dataset_name), H5P_DEFAULT ) );

  /* Now that we have a name we can use datFind once we have a locator,
     we create a temporary locator but must register it so that an error
     in datFind will not close the file immediately before we close it ourselves later. */
  temploc = dat1AllocLoc( status );
  if (*status == SAI__OK) {
    temploc->file_id = file_id;
    file_id = 0; /* now owned by the locator system */
    temploc->isprimary = HDS_TRUE;
    temploc->group_id = group_id;
    hds1RegLocator( temploc, status );
  }
  datFind( temploc, dataset_name, locator, status );

  /* force the locator to be primary as we get rid of the parent locator */
  if (*status == SAI__OK) (*locator)->isprimary = HDS_TRUE;

 CLEANUP:

  /* Free the temporary which will close the parent group */
  if (temploc) datAnnul(&temploc, status );

  if (*status != SAI__OK) {
    /* cleanup */
    if (*locator) datAnnul( locator, status );
    if (file_id > 0) H5Fclose( file_id );
  }

  return *status;

}

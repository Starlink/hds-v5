/*
*+
*  Name:
*     datAlter

*  Purpose:
*     Alter object size

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datAlter( HDSLoc *locator, int ndim, const hdsdim dims[], int *status);

*  Arguments:
*     locator = HDSLoc * (Given)
*        Object locator to alter.
*     ndim = int (Given)
*        Number of dimensions specified in "dim". Must be the number of dimensions
*        in the object itself.
*     dim = const hdsdim [] (Given)
*        New dimensions for object.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Alter the size of an array by increasing or reducing the last
*     (or only) dimension. If a structure array is to be reduced in
*     size, the operation will fail if any truncated elements contain
*     components.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - The dimensionality of the object can not differ before and after this
*     routine is called.
*     - Can not be called on a vectorized locator.

*  History:
*     2014-10-14 (TIMJ):
*        Initial version
*     2014-10-29 (TIMJ):
*        Now can reshape structure arrays.
*     2014-11-05 (TIMJ):
*        Resize by creating a new dataset and copying across
*        and deleting the original. Required for memory mapping.
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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"
#include "star/one.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

const hdsbool_t USE_H5RESIZE = 0;

int
datAlter( HDSLoc *locator, int ndim, const hdsdim dims[], int *status) {

  hid_t h5type = 0;
  hdsdim curdims[DAT__MXDIM];
  int curndim;
  int i;
  HDSLoc * parloc = NULL;
  HDSLoc * temploc = NULL;
  hid_t new_dataset_id = 0;
  hid_t new_dataspace_id = 0;

  if (*status != SAI__OK) return *status;

  if (locator->vectorized) {
    *status = DAT__OBJIN;
    emsRep("datAlter_1", "Can not alter the size of a vectorized object",
           status);
    return *status;
  }

  if (locator->pntr) {
    *status = DAT__OBJIN;
    emsRep("datAlter_2", "Can not alter the size of a mapped primitive",
           status);
    return *status;
  }

  if (locator->isslice) {
    *status = DAT__OBJIN;
    emsRep("datAlter_3", "Can not alter the size of a slice",
           status);
    return *status;
  }

  /* Get the current dimensions and validate new ones */
  datShape( locator, DAT__MXDIM, curdims, &curndim, status );

  if (curndim != ndim) {
    if (*status == SAI__OK) {
      *status = DAT__DIMIN;
      emsRepf("datAlter_4", "datAlter can not change the dimensionality (%d != %d)",
              status, curndim, ndim);
      return *status;
    }
  }

  if (*status != SAI__OK) return *status;

  for (i=0; i< (ndim-2); i++) {
    if ( dims[i] != curdims[i] ) {
      *status = DAT__DIMIN;
      emsRepf("datAlter_5", "datAlter: Dimension %d (1-based) does not match (%d != %d)",
              status, i+1, dims[i], curdims[i]);
      return *status;
    }
  }

  if ( dat1IsStructure( locator, status) ) {
    hdsdim curcount = 0;
    hdsdim newcount = 0;

    /* The restriction on final dimension being altered simplifies
       the calculation somewhat in that we can work in vectorized
       space. If we have 10 elements now and need to have 8 then
       we know we just delete elements 9 and 10. If we require
       12 we know we just add 11 and 12 at the correct coordinates. */
    curcount = 1;
    newcount = 1;
    for (i=0; i<curndim; i++) {
      curcount *= curdims[i];
      newcount *= dims[i];
    }

    if (newcount > curcount) {
      /* Need to extend */
      char grouptype[DAT__SZTYP+1];
      char groupname[DAT__SZNAM+1];
      datType( locator, grouptype, status );
      datName( locator, groupname, status );

      for (i=curcount+1; i <= newcount; i++) {
        hid_t cellgroup_id = 0;
        cellgroup_id = dat1CreateStructureCell( locator->group_id, i, grouptype, groupname, ndim, dims, status );
        if (cellgroup_id > 0) H5Gclose(cellgroup_id);
      }
    } else if (newcount < curcount) {
      /* Need to shrink - delete each structure and complain if
         the structure is not empty -- use curdims */
      for (i=newcount+1; i<=curcount; i++) {
        hdsdim coords[DAT__MXDIM];
        char cellname[128];
        HDSLoc * cell = NULL;
        int ncomp = 0;
        dat1Index2Coords(i, ndim, curdims, coords, status );
        dat1Coords2CellName( ndim, coords, cellname, sizeof(cellname), status );

        /* Need to peak inside -- datCell would be a bit inefficient but use minimum code/
           Should still work as I remove earlier structures as part of the loop */
        datCell(locator, ndim, coords, &cell, status );
        datNcomp( cell, &ncomp, status );
        datAnnul( &cell, status );
        if (ncomp > 0) {
          if (*status == SAI__OK) {
            *status = DAT__DELIN;
            emsRep("datAlter_6", "datAlter: Can not shrink structure array as some structures"
                   " to be deleted contain components", status );
          }
          goto CLEANUP;
        }
        /* Remove the empty element */
        datErase( locator, cellname, status );
      }
    } else {
      /* Oddly, no change requested so we are done. Should this be an error? */
      goto CLEANUP;
    }

    /* Need to update the dimensions in the attribute */
    dat1SetStructureDims( locator->group_id, ndim, dims, status );

  } else {
    hsize_t h5dims[DAT__MXDIM];
    char primname[DAT__SZNAM+1];
    char tempname[3*DAT__SZNAM+1];
    hdsbool_t state;

    /* Copy dimensions and reorder */
    dat1ImportDims( ndim, dims, h5dims, status );

    if (USE_H5RESIZE) {
      /* This assumes we have chunked storage for datasets */
      CALLHDFQ( H5Dset_extent( locator->dataset_id, h5dims ) );
      /* We need to get a new dataspace from this modified dataset. */
      H5Sclose( locator->dataspace_id );
      locator->dataspace_id = H5Dget_space( locator->dataset_id );
    } else {
      /* We can only use H5Dset_extent if the dataset has
         been created with larger dimensions (usually unlimited).
         Since HDS primitives must always be resizable but would like
         to be mappable we instead go for the inefficient option
         of making a new dataset with the new size and copying the
         data in */

      /* Need enclosing group locator */
      datParen( locator, &parloc, status );

      /* HDF5 data type of the locator */
      CALLHDF( h5type,
               H5Dget_type( locator->dataset_id ),
               DAT__HDF5E,
               emsRep("dat1Type_1", "datType: Error obtaining data type of dataset", status)
               );

      /* Create a new dataset with a name related to this dataset but which
         does not conform to the HDS rules */
      datName( locator, primname, status );
      one_snprintf(tempname, sizeof(tempname), "%s%s", status,
                   "+TEMPORARY_DATASET_", primname);

      dat1NewPrim( parloc->group_id, ndim, h5dims, h5type, tempname,
                   &new_dataset_id, &new_dataspace_id, status );

      /* Nothing to copy if the source locator is not defined */
      datState( locator, &state, status );
      if (state && *status == SAI__OK) {
        size_t numin = 1;
        size_t numout = 1;
        size_t nbperel;
        char type_str[DAT__SZTYP+1];
        void *inpntr = NULL;
        void *outpntr = NULL;
        size_t nbytes = 0;

        /* Type in HDS speak */
        datType(locator, type_str, status );

        /* Number of bytes per element -- should be the same in and out */
        datLen( locator, &nbperel, status );

        /* Easiest to map the the input and output and copy */
        datMapV( locator, type_str, "READ", &inpntr, &numin, status );

        /* Create temporary locator for output */
        temploc = dat1AllocLoc( status );
        temploc->dataset_id = new_dataset_id;
        temploc->dataspace_id = new_dataspace_id;
        temploc->file_id = locator->file_id;

        /* And map the output */
        datMapV( temploc, type_str, "WRITE", &outpntr, &numout, status );

        /* Copy up to numin elements */
        nbytes = nbperel * (numin > numout ? numout : numin);
        memcpy( outpntr, inpntr, nbytes );

        /* Then set the remaining elements to 0 (or bad). We could
           get around this need by specifying the fill value on object creation */
        if ( numout > numin) {
          size_t nextra = nbperel * (numout - numin);
          unsigned char * offpntr = NULL;
          offpntr = &((unsigned char *)outpntr)[nbytes];
          memset( offpntr, 0, nextra );
        }

        /* Unmap and free the temporary locator */
        datUnmap( locator, status );
        datUnmap( temploc, status );
        temploc = dat1FreeLoc( temploc, status );
      }

      /* Delete the source dataset -- free resources in supplied locator */
      H5Sclose( locator->dataspace_id );
      H5Dclose( locator->dataset_id );
      datErase( parloc, primname, status );

      /* Relocate the new dataset */
      CALLHDFQ(H5Lmove( parloc->group_id, tempname,
                        parloc->group_id, primname, H5P_DEFAULT, H5P_DEFAULT));

      /* Update the locator */
      locator->dataspace_id = new_dataspace_id;
      locator->dataset_id = new_dataset_id;

    }
  }

 CLEANUP:
  datAnnul(&parloc, status);
  if (*status != SAI__OK) {
    if (h5type > 0) H5Tclose( h5type );
    if (temploc) temploc = dat1FreeLoc( temploc, status );
  }
  return *status;
}

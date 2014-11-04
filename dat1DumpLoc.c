/*
*+
*  Name:
*     dat1DumpLoc

*  Purpose:
*     Dump useful information about a locator

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     dat1DumpLoc( const HDSLoc * locator, int * status );

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Locator to be summarized.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Write to standard output information about the state of a locator.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  History:
*     2014-08-26 (TIMJ):
*        Initial version
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

#include <stdio.h>

#include "hdf5.h"
#include "hdf5_hl.h"

#include "star/one.h"
#include "ems.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"
#include "sae_par.h"

static void dump_dataspace_info( hid_t dataspace_id, const char * label, int *status);

void dat1DumpLoc( const HDSLoc* locator, int * status ) {
  if (*status != SAI__OK) return;
  char * name_str = NULL;
  char * file_str = NULL;
  ssize_t ll;
  hid_t objid = 0;
  hid_t dspace_id = 0;

  objid = dat1RetrieveIdentifier( locator, status );
  name_str = dat1GetFullName( objid, 0, &ll, status );
  file_str = dat1GetFullName( objid, 1, &ll, status );

  printf("Dump of locator at %s (%s)\n", name_str, file_str);
  printf("- File: %d; Group %d; Dataspace: %d; Dataset: %d; Data Type: %d\n",
         locator->file_id, locator->group_id, locator->dataspace_id,
         locator->dataset_id, locator->dtype);
  printf("- Vectorized: %zu; Bytes mapped: %zu, Array mapped: %p\n",
         locator->vectorized, locator->bytesmapped, locator->pntr );
  printf("- Is sliced: %d; Group name: '%s'\n", locator->isslice, locator->grpname);

  if (locator->dataspace_id > 0) {
    if (locator->vectorized) {
      printf("- Locator is vectorized with bounds (1-based): %zu:%zu\n",
             (size_t)1, (size_t)locator->vectorized );
      if (locator->isslice) {
        hssize_t nelem;
        nelem = H5Sget_select_npoints(locator->dataspace_id);
        printf("    and is sliced with bounds: %zu:%zu (%zu elements)\n",
               (size_t)(locator->slicelower)[0],
               (size_t)(locator->sliceupper)[0],
               (nelem > 0 ? (size_t)nelem : 0) );
      }
    } else {
      dump_dataspace_info( locator->dataspace_id, "Locator associated", status);
    }
    dspace_id = H5Dget_space( locator->dataset_id );
    dump_dataspace_info( dspace_id, "Dataset associated", status );
    H5Sclose( dspace_id );
  }
 CLEANUP:
  if (file_str) MEM_FREE(file_str);
  if (name_str) MEM_FREE(name_str);
  return;
}

static void dump_dataspace_info( hid_t dataspace_id, const char * label, int *status) {
  hsize_t * blockbuf = NULL;

  if (dataspace_id > 0) {
    hsize_t h5dims[DAT__MXDIM];
    hssize_t nblocks = 0;
    int i;
    int rank;
    hsize_t nelem = 1;

    CALLHDFE( int,
              rank,
              H5Sget_simple_extent_dims( dataspace_id, h5dims, NULL ),
              DAT__DIMIN,
              emsRep("datshape_1", "datShape: Error obtaining shape of object",
                     status)
              );
    nblocks = H5Sget_select_hyper_nblocks( dataspace_id );
    if (nblocks < 0) nblocks = 0; /* easier to understand */

    printf("- %s dataspace has rank: %d and %d hyperslab%s\n", label, rank, (int)nblocks,
           (nblocks == 1 ? "" : "s") );
    printf("    Dataspace dimensions (HDF5 order): ");
    for (i=0; i<rank; i++) {
      printf(" %zu", (size_t)h5dims[i]);
      nelem *= h5dims[i];
    }
    printf(" (%zu element%s)\n",(size_t)nelem, (nelem == 1 ? "" : "s"));

    if (nblocks > 0) {
      hssize_t n = 0;
      herr_t h5err = 0;
      hsize_t nelem = 1;

      blockbuf = MEM_MALLOC( nblocks * rank * 2 * sizeof(*blockbuf) );

      CALLHDF( h5err,
               H5Sget_select_hyper_blocklist( dataspace_id, 0, nblocks, blockbuf ),
               DAT__DIMIN,
               emsRep("dat1DumpLoc_2", "dat1DumpLoc: Error obtaining shape of slice", status )
               );

      for (n=0; n<nblocks; n++) {
        /* The buffer is returned in form:
           ndim start coordinates, then ndim opposite corner coordinates
           and repeats for each block
        */
        printf("    Hyperslab #%d (0-based):", (int)n);
        for (i = 0; i<rank; i++) {
          hsize_t start;
          hsize_t opposite;
          size_t offset = n * rank;
          start = blockbuf[offset+i];
          opposite = blockbuf[offset+i+rank];
          /* So update the shape to account for the slice: HDS is 1-based */
          printf(" %zu:%zu", (size_t)start, (size_t)opposite);
          nelem *= (opposite-start+1);
        }
        printf(" (%zu element%s)\n",(size_t)nelem, (nelem == 1 ? "" : "s"));
      }

    }
  }
 CLEANUP:
  if (blockbuf) MEM_FREE(blockbuf);
  return;
}

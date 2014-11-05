/*
*+
*  Name:
*     dat1NewPrim

*  Purpose:
*     Create new dataset

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     dat1NewPrim( hid_t group_id, int ndim, const hsize_t h5dims[], hid_t h5type,
                   const char * name_str, hid_t * dataset_id, hid_t *dataspace_id, int *status);

*  Arguments:
*     group_id = hid_t (Given)
*        Location in HDF5 file to receive the new dataset.
*     ndim = int (Given)
*        Number of dimensions of dataset (0 means scalar).
*     h5dims = const hsize_t [] (Given)
*        Dimensions in HDF5 C order. No more than DAT__MXDIM.
*     h5type = hid_t (Given)
*        HDF5 datatype of new dataset.
*     name_str = const char * (Given)
*        Name of new dataset. Not constrained by HDS rules so can
*        be longer than DAT__SZNAM.
*     dataset_id = hid_t * (Returned)
*        Dataset identifier of new dataset.
*     dataspace_id = hid_t * (Returned)
*        Dataspace identifier of new dataspace.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Creates an HDF5 dataset given HDF5-style arguments.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  History:
*     2014-11-05 (TIMJ):
*        Initial version. Refactored from dat1New
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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "ems.h"
#include "sae_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"

void dat1NewPrim( hid_t group_id, int ndim, const hsize_t h5dims[], hid_t h5type,
                  const char * name_str, hid_t * dataset_id, hid_t *dataspace_id, int *status ) {
  hid_t cparms = 0;
  *dataset_id = 0;
  *dataspace_id = 0;

  if (*status != SAI__OK) return;

  if (ndim == 0) {

    CALLHDF( *dataspace_id,
             H5Screate( H5S_SCALAR ),
             DAT__HDF5E,
             emsRepf("dat1New_0", "Error allocating data space for scalar %s",
                     status, name_str )
             );

    cparms = H5P_DEFAULT;

  } else {

    /* Create a primitive -- HDS assumes you are going to adjust
       the dimensions of any data array so you must create these primitives
       to take that possibility into account. */

    const hsize_t h5max[DAT__MXDIM] = { H5S_UNLIMITED, H5S_UNLIMITED, H5S_UNLIMITED,
                                        H5S_UNLIMITED, H5S_UNLIMITED, H5S_UNLIMITED,
                                        H5S_UNLIMITED };

    /* Create the data space for the dataset */
    CALLHDF( *dataspace_id,
             H5Screate_simple( ndim, h5dims, h5max ),
             DAT__HDF5E,
             emsRepf("dat1New_1", "Error allocating data space for %s",
                     status, name_str )
             );

    /* Since we are trying to be extendible we have to allow chunking.
       HDS gives us no ability to know how to chunk so we guess that
       chunk sizes of the initial creation size are okay */
    CALLHDF( cparms,
             H5Pcreate( H5P_DATASET_CREATE ),
             DAT__HDF5E,
             emsRepf("dat1New_1b", "Error creating parameters for data space %s",
                     status, name_str)
             );
    CALLHDFQ( H5Pset_chunk( cparms, ndim, h5dims ) );

  }

  /* now place the dataset */
  CALLHDF( *dataset_id,
           H5Dcreate2(group_id, name_str, h5type, *dataspace_id,
                      H5P_DEFAULT, cparms, H5P_DEFAULT),
           DAT__HDF5E,
           emsRepf("dat1New_2", "Error placing the data space in the file for %s",
                   status, name_str )
           );

  /* In HDS parlance the primitive data are currently undefined at this point */
  /* We indicate this by setting an attribute */
  {
    int attrval = 0;
    CALLHDFQ( H5LTset_attribute_int( *dataset_id, ".", HDS__ATTR_DEFINED, &attrval, 1 ) );
  }

 CLEANUP:
  if (*status != SAI__OK) {
    /* tidy */
    if (*dataspace_id > 0) {
      H5Sclose( *dataspace_id );
      *dataspace_id = 0;
    }
    if (*dataset_id > 0) {
      H5Dclose( *dataset_id );
      *dataset_id = 0;
    }
  }
  return;
}

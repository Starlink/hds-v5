/*
*+
*  Name:
*     dat1New

*  Purpose:
*     Create a new component in a structure and return a locator

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     HDSLoc * dat1New( const HDSLoc *locator, const char *name_str, const char *type_str,
*                      int ndim, const hdsdim dims[], int * status );

*  Arguments:
*     locator = const HDSLoc * (Given)
*        Locator to structure that will receive the new component.
*     name = const char * (Given)
*        Name of the object in the container.
*     type = const char * (Given)
*        Type of object.  If type matches one of the HDS primitive type names a primitive
*        of that type is created, otherwise the object is assumed to be a structure.
*     ndim = int (Given)
*        Number of dimensions. Use 0 for a scalar. See the Notes for a discussion of
*        arrays of structures.
*     dims = const hdsdim [] (Given)
*        Dimensionality of the object. Should be dimensioned with ndim. The array
*        is not accessed if ndim == 0.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Creates a new component (primitive type or structure) in an existing structure
*     and return the corresponding locator.

*  Returned Value:
*     HDSLoc * = locator associated with newly created structure. NULL on error.

*  Authors:
*     TIMJ: Tim Jenness (Cornell)
*     {enter_new_authors_here}

*  Notes:
*     - Primitive non-scalar types are created with unlimited dimensions to
*       allow resizing but the chunk size is configured to match the supplied
*       dimensions. The HDS API has no means to control whether resizing will
*       be required and how the chunking should be handled.

*  History:
*     2014-08-20 (TIMJ):
*        Initial version
*     2014-09-04 (TIMJ):
*        Unlimited dimensions.
*     2014-09-05 (TIMJ):
*        Add arrays of structures
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

#include <stdlib.h>
#include <strings.h>

#include "hdf5.h"
#include "hdf5_hl.h"
#include "ems.h"
#include "star/one.h"
#include "prm_par.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"
#include "dat_err.h"
#include "sae_par.h"

static void dat1index2coords ( size_t idx, int ndim, hsize_t arraydims[DAT__MXDIM],
                               hsize_t coords[DAT__MXDIM], int *status );

HDSLoc *
dat1New( const HDSLoc    *locator,
        const char      *name_str,
        const char      *type_str,
        int       ndim,
        const hdsdim    dims[],
        int       *status) {

  char cleanname[DAT__SZNAM+1];
  char groupstr[DAT__SZTYP+1];

  hid_t group_id = 0;
  hid_t dataset_id = 0;
  hid_t dataspace_id = 0;
  hid_t cparms = 0;
  hid_t h5type = 0;
  hid_t place = 0;
  int isprim;
  int typcreat = 0;
  HDSLoc * thisloc = NULL;
  hsize_t h5dims[DAT__MXDIM];

  if (*status != SAI__OK) return NULL;

  /* The name can not have "." in it as this will confuse things
     even though HDF5 will be using a "/" */
  dau1CheckName( name_str, 1, cleanname, sizeof(cleanname), status );
  if (*status != SAI__OK) return NULL;

  /* Copy dimensions if appropriate */
  dat1ImportDims( ndim, dims, h5dims, status );

  /* Work out where to place the component */
  place = dat1RetrieveContainer( locator, status );

  /* Convert the HDS data type to HDF5 data type */
  isprim = dau1CheckType( type_str, &h5type, groupstr,
                          sizeof(groupstr), &typcreat, status );

  /* The above routine has allocated resources so from here we can not
     simply return on error but have to ensure we clean up */

  /* Now create the group or dataset at the top level */
  if (isprim) {
    if (ndim == 0) {

      CALLHDF( dataspace_id,
               H5Screate( H5S_SCALAR ),
               DAT__HDF5E,
               emsRepf("dat1New_0", "Error allocating data space for scalar %s",
                       status, cleanname )
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
      CALLHDF( dataspace_id,
               H5Screate_simple( ndim, h5dims, h5max ),
               DAT__HDF5E,
               emsRepf("dat1New_1", "Error allocating data space for %s",
                       status, cleanname )
               );

      /* Since we are trying to be extendible we have to allow chunking.
         HDS gives us no ability to know how to chunk so we guess that
         chunk sizes of the initial creation size are okay */
      CALLHDF( cparms,
               H5Pcreate( H5P_DATASET_CREATE ),
               DAT__HDF5E,
               emsRepf("dat1New_1b", "Error creating parameters for data space %s",
                       status, cleanname)
               );
      CALLHDFQ( H5Pset_chunk( cparms, ndim, h5dims ) );

    }

    /* now place the dataset */
    CALLHDF( dataset_id,
             H5Dcreate2(place, cleanname, h5type, dataspace_id,
                        H5P_DEFAULT, cparms, H5P_DEFAULT),
             DAT__HDF5E,
             emsRepf("dat1New_2", "Error placing the data space in the file for %s",
                     status, cleanname )
             );

  } else {
    /* Create a group */

    CALLHDF( group_id,
             H5Gcreate2(place, cleanname, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
             DAT__HDF5E,
             emsRepf("dat1New_4", "Error creating structure/group '%s'", status, cleanname)
             );

    /* Actual data type of the structure/group must be stored in an attribute */
    CALLHDFQ( H5LTset_attribute_string( group_id, ".", "HDSTYPE", groupstr ) );

    /* Also store the number of dimensions */
    CALLHDFQ( H5LTset_attribute_int( group_id, ".", "HDSDIMS", &ndim, 1 ) );

    if (ndim > 0) {
      /* HDF5 can not define an array of structures so we create a collection
         of groups below the parent group. */
      int i;
      size_t ngroups = 1;
      size_t n;

      /* Structures will always be accessed by their coordinates
         (3,2) or (4) or (1,3,2) etc. It makes sense therefore to
         simply name our structures such that these coordinates
         are embedded directly in the name. This has some advantages:
         - We know how to trivially map from the requested coordinate
           to a group.
         - When the name is requested (e.g. hdsTrace) we already
           know that ROOT.RECORDS.HDSCELL(3,2).SOMEINT will have an
           effective trace of ROOT.RECORDS(3,2).SOMEINT [simply remove
           the ".HDSCELL(3,2)" from the full path.
      */

      for (i = 0; i < ndim; i++) {
        ngroups *= h5dims[i];
      }

      for (n = 1; n <= ngroups; n++) {
        hid_t cellgroup_id = 0;
        char cellname[128];
        hsize_t coords[DAT__MXDIM];

        dat1index2coords(n, ndim, h5dims, coords, status );
        dat1Coords2CellName( ndim, coords, cellname, sizeof(cellname), status );

        CALLHDF( cellgroup_id,
                 H5Gcreate2(group_id, cellname, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT),
                 DAT__HDF5E,
                 emsRepf("dat1New_4", "Error creating structure/group '%s'", status, cleanname)
                 );

        /* Actual data type of the structure/group must be stored in an attribute */
        CALLHDFQ( H5LTset_attribute_string( cellgroup_id, ".", "HDSTYPE", groupstr ) );
      }
    }
  }

  /* We now have to store this in a new locator */
  if (*status == SAI__OK) {
    HDSLoc * thisloc = dat1AllocLoc( status );
    thisloc->dataset_id = dataset_id;
    thisloc->group_id = group_id;
    thisloc->dataspace_id = dataspace_id;
    if (typcreat) thisloc->dtype = h5type;
    return thisloc;
  }

 CLEANUP:
  /* Everything should be freed */
  if (typcreat) H5Tclose( h5type );
  if (dataset_id) H5Dclose(dataset_id);
  if (dataspace_id) H5Sclose(dataspace_id);
  if (cparms > 0 && cparms != H5P_DEFAULT) H5Pclose(cparms);
  if (group_id) H5Gclose(group_id);
  if (thisloc) {
    thisloc = dat1FreeLoc(thisloc, status);
  }
  return NULL;
}

/*
  Given a 1-based index, and the dimensions of the N-D array,
  return the 1-based N-D coordinates. For example, in a 3-D
  array of dimensions (4,3,2), index 19 is element (3,2,2)
  and index 6 is (2,2,1).
*/


static void dat1index2coords ( size_t idx, int ndim, hsize_t arraydims[DAT__MXDIM],
                               hsize_t coords[DAT__MXDIM], int *status ) {

  int curdim;
  int prevdim;

  if (*status != SAI__OK) return;

  /*
    Loop over one fewer dimensions than we actually have
    subtracting the biggest dimensions from idx as we go.
    The final coordinate value is simply the remainder
    when we finish looping
  */

  for (curdim = 1; curdim < ndim; curdim++) {
    size_t intdiv;
    size_t elems_prior = 1;
    /* Calculate how many elements are covered by full
       earlier dimensions */
    for (prevdim = 1; prevdim <= (ndim-curdim); prevdim++) {
      elems_prior *= arraydims[prevdim-1]; /* zero based lookup */
    }

    /* Calculate the coordinate for the current dim by dividing
       by the number of elements prior using integer division. Need to
       subtract one from the result for 1-based counting. */
    intdiv = (idx-1) / elems_prior;

    /* Store the coordinate, starting from the end. The +1 is
       for 1-based counting. */
    coords[ndim-curdim] = intdiv + 1;

    /* And subtract all those elements from the supplied index and go
       round again */
    idx -= intdiv * elems_prior;

  }

  /* The final value for idx is the final coordinate value */
  coords[0] = idx;

}

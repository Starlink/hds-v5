/*
*+
*  Name:
*     datName

*  Purpose:
*     Enquire the object name

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Library routine

*  Invocation:
*     datName( const HDSLoc * locator, char name_str[DAT__SZNAM+1],
*              int * status );

*  Arguments:
*     locator = const HDSLoc * (Given)
*         Object locator
*     name_str = char * (Given and Returned)
*         Buffer to receive the object name. Must be of size
*         DAT__SZNAM+1.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Enquire the object name.

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

#include "hdf5.h"
#include "hdf5_hl.h"

#include "star/one.h"
#include "ems.h"

#include "hds1.h"
#include "dat1.h"
#include "hds.h"

#include "dat_err.h"
#include "sae_par.h"

int
datName(const HDSLoc *locator,
        char name_str[DAT__SZNAM+1],
        int *status) {

  hid_t objid;
  ssize_t lenstr;
  char * tempstr = NULL;

  if (*status != SAI__OK) return *status;

  objid = dat1RetrieveIdentifier( locator, status );
  if (*status != SAI__OK) return *status;

  /* Run first to get the size of the buffer we need to use */
  lenstr = H5Iget_name( objid, NULL, 0 );
  if (lenstr < 0) {
    *status = DAT__HDF5E;
    emsRep( "datName_1", "datName: Error obtaining name of locator",
            status);
    return *status;
  }

  /* Allocate buffer of the right length */
  tempstr = malloc( lenstr + 1 );
  if (!tempstr) {
    *status = DAT__NOMEM;
    emsRep( "datName_2", "datName: Malloc error. Can not proceed",
            status);
    return *status;
  }

  lenstr = H5Iget_name( objid, tempstr, lenstr+1);
  if (lenstr < 0) {
    *status = DAT__HDF5E;
    emsRep( "datName_3", "datName: Error obtaining name of locator",
            status);
  }

  /* Now walk through the string backwards until we find the
     "/" character indicating the parent group */
  if (*status == SAI__OK) {
    ssize_t i;
    ssize_t startpos = 0; /* whole string as default */
    for (i = 0; i <= lenstr; i++) {
      size_t iposn = lenstr - i;
      if ( tempstr[iposn] == '/' ) {
        startpos = iposn + 1; /* want the next character */
        break;
      }
    }

    /* Now copy what we need */
    one_strlcpy( name_str, &(tempstr[startpos]), DAT__SZNAM+1, status );

  }

  free(tempstr);

  if (*status != SAI__OK) {
    emsRep("datName_4", "datName: Error obtaining a name of a locator",
           status );
  }

  return *status;
}

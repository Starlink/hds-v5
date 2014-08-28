/*
*+
*  Name:
*     hdsTest

*  Purpose:
*     Test the C interface to HDS

*  Language:
*     Starlink ANSI C

*  Description:
*     This program tests some of the C API to HDS. It is not meant
*     to be an exhaustive test of all the API (at least not initially).

*  Copyright:
*     Copyright (C) 2005-2006 Particle Physics and Astronomy Research Council.
*     All Rights Reserved.

*  Authors:
*     TIMJ: Tim Jenness (JAC, Hawaii)
*     {enter_new_authors_here}

*  History:
*     04-NOV-2005 (TIMJ):
*        Original.
*     20-DEC-2005 (TIMJ):
*        No longer requires FC_MAIN
*     25-JAN-2006 (TIMJ):
*        Add hdsShow/hdsInfoI
*     {enter_further_changes_here}

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*     MA 02110-1301, USA

*  Bugs:
*     {note_any_bugs_here}

*-
*/

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "hds1.h"
#include "dat1.h"
#include "hds.h"
#include <stdlib.h>
#include "ems.h"
#include "dat_err.h"
#include "sae_par.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

static void cmpstrings( const char * teststr, const char * expectedstr, int *status );

int main (void) {

  /*  Local Variables: */
  const char path[] = "hds_ctest";
  int status = SAI__OK;
  hdsdim dim[] = { 10, 20 };
  hdsdim dimd[1];
  const char * chararr[] = { "TEST1", "TEST2", "Longish String" };
  char *retchararr[4];
  char buffer[1024];  /* plenty large enough */
  double darr[] = { 4.5, 2.5 };
  double retdarr[2];
  void *mapv;    /* Mapped void* */
  double *mapd;  /* Mapped _DOUBLE */
  float  *mapf;  /* Mapped _REAL */
  int *mapi;     /* Mapped _INTEGER */
  int64_t *mapi64; /* Mapped _INT64 */
  HDSLoc * loc1 = NULL;
  HDSLoc * loc2 = NULL;
  HDSLoc * loc3 = NULL;
  size_t actval;
  size_t nel;
  size_t nelt;
  size_t nbytes;
  size_t i;
  int n;
  double sumd;
  int sumi;
  int64_t sumi64;
  int64_t test64;
  int64_t testin64;
  char namestr[DAT__SZNAM+1];
  char typestr[DAT__SZTYP+1];
  const int64_t VAL__BADK = (-9223372036854775807 - 1);

  emsBegin(&status);

  /* Create a new container file */
  hdsNew( path, "HDS_TEST", "NDF", 0, dim, &loc1, &status );

  /* Some components */
  datNew( loc1, "DATA_ARRAY", "_INTEGER", 2, dim, &status );
  datNew1C( loc1, "ONEDCHAR", 14, 3, &status );
  datNew1L( loc1, "BOOLEAN", 3, &status );
  datNew1D( loc1, "ONEDD", 2, &status );
  datNew0K( loc1, "TESTI64", &status );
  datNew0K( loc1, "TESTBADI64", &status );
  datNew( loc1, "TESTSTRUCT", "ASTRUCT", 0, dim, &status );

  datFind( loc1, "TESTSTRUCT", &loc2, &status );
  datType( loc2, typestr, &status );
  cmpstrings( typestr, "ASTRUCT", &status );
  datAnnul( &loc2, &status );

  datFind( loc1, "ONEDD", &loc2, &status );
  datType( loc2, typestr, &status );
  cmpstrings( typestr, "_DOUBLE", &status );
  datAnnul( &loc2, &status );

  datFind( loc1, "BOOLEAN", &loc2, &status );
  datType( loc2, typestr, &status );
  cmpstrings( typestr, "_LOGICAL", &status );
  datAnnul( &loc2, &status );

  /* Now check the type of the root group */
  datType( loc1, typestr, &status );
  cmpstrings( typestr, "NDF", &status );

  /* Confirm size and type */
  if (status == SAI__OK) {
    size_t dsize;
    datFind( loc1, "DATA_ARRAY", &loc2, &status );
    datType( loc2, typestr, &status );
    cmpstrings( typestr, "_INTEGER", &status );

    datSize( loc2, &dsize, &status );
    datAnnul( &loc2, &status );
    if (status == SAI__OK) {
      if ( dsize != ((size_t)dim[0]*(size_t)dim[1])) {
        status = DAT__FATAL;
        emsRepf("", "Size of DATA_ARRAY inconsistent. Got %zu expected %zu.", &status,
                dsize, ((size_t)dim[0]*(size_t)dim[1]));
      }
    }
  }
  if (status == SAI__OK) {
    size_t dsize;
    datFind( loc1, "TESTI64", &loc2, &status );
    datType( loc2, typestr, &status );
    cmpstrings( typestr, "_INT64", &status );

    datSize( loc2, &dsize, &status );
    datAnnul( &loc2, &status );
    if (status == SAI__OK) {
      if ( dsize != 1) {
        status = DAT__FATAL;
        emsRepf("", "Size of TESTI64 inconsistent. Got %zu expected %zu.", &status,
               dsize, 1);
      }
    }
  }

  /* Populate */
  testin64 = 9223372036854775800;
  datFind( loc1, "TESTI64", &loc2, &status );

  /* Verify name */
  datName( loc2, namestr, &status );
  if (status == SAI__OK) {
    if ( strcmp(namestr, "TESTI64") != 0 ) {
      status = DAT__FATAL;
      emsRepf("TESTNAMEI64", "Cf '%s' with '%s' fail", &status,
              "TESTI64", namestr );
    }
  }

  datAnnul(&loc2, &status );

  /* Close the file */
  datAnnul( &loc1, &status );


  if (status == SAI__OK) {
    printf("HDS C installation test succeeded\n");
    emsEnd(&status);
    return EXIT_SUCCESS;
  } else {
    printf("HDS C installation test failed\n");
    emsEnd(&status);
    return EXIT_FAILURE;
  }


}

/* Simple routine to compare to strings and call EMS on the result */
static void cmpstrings( const char * teststr, const char * expectedstr, int *status ) {
  if (*status != SAI__OK) return;
  if (strcmp( teststr, expectedstr ) != 0) {
    *status = DAT__FATAL;
    emsRepf("", "Got string '%s' but expected '%s'", status,
            teststr, expectedstr );
  }
  return;
}

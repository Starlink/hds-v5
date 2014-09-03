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
static void cmpszints( size_t result, size_t expected, int *status );
static void cmpprec ( const HDSLoc * loc1, const char * name, int * status );

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
  char boolarr[] = { 1, 0, 1 };
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

  datFind( loc1, "ONEDCHAR", &loc2, &status );
  datType( loc2, typestr, &status );
  cmpstrings( typestr, "_CHAR*14", &status );
  datClen(loc2, &nel, &status );
  cmpszints( nel, 14, &status );

  /* Now test it is a primitive */
  {
    int prim;
    int struc;
    datPrim( loc2, &prim, &status );
    if (!prim && status == SAI__OK) {
      status = DAT__FATAL;
      emsRep("", "Primitive does not seem to be primitive", &status);
    }
    datStruc( loc2, &struc, &status );
    if (struc && status == SAI__OK) {
      status = DAT__FATAL;
      emsRep("", "Primitive seems to be a structure", &status);
    }
  }

  datAnnul( &loc2, &status );

  /* Now check the type of the root group */
  datType( loc1, typestr, &status );
  cmpstrings( typestr, "NDF", &status );

  {
    int struc;
    int prim;
    int ncomp;
    /* Put a component of each type in test structure */
    datFind( loc1, "TESTSTRUCT", &loc2, &status );

    /* First test it is a structure */
    datPrim( loc2, &prim, &status );
    if (prim && status == SAI__OK) {
      status = DAT__FATAL;
      emsRep("", "Structure seems to be primitive", &status);
    }
    datStruc( loc2, &struc, &status );
    if (!struc && status == SAI__OK) {
      status = DAT__FATAL;
      emsRep("", "Structure does not seem to be a structure", &status);
    }

//    datNew0B( loc2, "BYTE", &status);
//    datNew0UB( loc2, "UBYTE", &status);

    datNew0W( loc2, "WORD", &status);
    datNew0UW( loc2, "UWORD", &status);
    datNew0I( loc2, "INTEGER", &status);
    datNew0K( loc2, "INT64", &status);
    datNew0L( loc2, "LOGICAL", &status);
    datNew0R( loc2, "REAL", &status);
    datNew0D( loc2, "DOUBLE", &status);
    datNew0C( loc2, "CHAR", 12, &status );

    datNcomp( loc2, &ncomp, &status );
    if (status == SAI__OK) {
      if (ncomp != 8) {
        status = DAT__FATAL;
        emsRepf("", "Got %d components in structure rather than %d\n",
                &status, ncomp, 8 );
      }
    }

//    cmpprec( loc2, "BYTE", &status );
//    cmpprec( loc2, "UBYTE", &status );
    cmpprec( loc2, "WORD", &status );
    cmpprec( loc2, "UWORD", &status );
    cmpprec( loc2, "INTEGER", &status );
    cmpprec( loc2, "INT64", &status );
    cmpprec( loc2, "LOGICAL", &status );
    cmpprec( loc2, "REAL", &status );
    cmpprec( loc2, "DOUBLE", &status );
    cmpprec( loc2, "CHAR", &status );
    datAnnul( &loc2, &status );
  }


  /* Confirm size and type */
  if (status == SAI__OK) {
    size_t dsize;
    datFind( loc1, "DATA_ARRAY", &loc2, &status );
    datType( loc2, typestr, &status );
    cmpstrings( typestr, "_INTEGER", &status );

    {
      hdsdim hdims[DAT__MXDIM];
      int actdims;
      datShape( loc2, DAT__MXDIM, hdims, &actdims, &status);
      cmpszints( actdims, 2, &status );
      cmpszints( hdims[0], dim[0], &status );
      cmpszints( hdims[1], dim[1], &status );
    }

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
                dsize, (size_t)1);
      }
    }
  }

  /* Populate */
  testin64 = 9223372036854775800;
  datFind( loc1, "TESTI64", &loc2, &status );

  /* Verify name */
  datName( loc2, namestr, &status );
  cmpstrings( namestr, "TESTI64", &status );
  {
    char path_str[1024];
    char file_str[2048];
    int nlev;
    hdsTrace( loc2, &nlev, path_str, file_str,
              &status, sizeof(path_str),
              sizeof(file_str));
    printf("File: '%s' Path: '%s' Level=%d\n", file_str,
           path_str, nlev);
    cmpstrings( path_str, "HDS_TEST.TESTI64", &status);
    cmpszints( nlev, 2, &status);
  }

  if (status == SAI__OK) {
    /* Do not use MERS in test. We create an error message
       with EMS and then extract it as text */
    int lstat = DAT__FATAL;
    char param[10];
    char opstr[2048];
    int oplen;
    int parlen;
    emsMark();
    datMsg("OBJ", loc2 );
    emsRep("", "^OBJ", &lstat);
    emsEload( param, &parlen, opstr, &oplen, &lstat);
    printf("datMsg: %s\n", opstr);
    emsAnnul(&lstat);
    emsRlse();
  }

  datPut0K( loc2, testin64, &status );
  datGet0K( loc2, &test64, &status );
  datAnnul( &loc2, &status );
  if (status == SAI__OK) {
    if ( test64 != testin64 ) {
      status = DAT__FATAL;
      emsRepf( "TESTI64", "Test _INT64 value %" PRIi64 " did not match expected %"PRIi64,
               &status, test64, testin64 );
    }
  }

  datFind( loc1, "TESTBADI64", &loc2, &status );
  datPut0K( loc2, VAL__BADK, &status );
  datGet0K( loc2, &test64, &status );
  datAnnul( &loc2, &status );
  if (status == SAI__OK) {
    if ( test64 != VAL__BADK ) {
      status = DAT__FATAL;
      emsRepf( "TESTBADI64", "Test _INT64 value %" PRIi64 " did not match expected VAL__BADK",
               &status, test64 );
    }
  }

  datFind( loc1, "BOOLEAN", &loc2, &status );
  datPutVL( loc2, 3, boolarr, &status );
  datName( loc2, namestr, &status );
  cmpstrings( namestr, "BOOLEAN", &status );
  datType( loc2, typestr, &status );
  cmpstrings( typestr, "_LOGICAL", &status );
  /* Annul */
  datAnnul( &loc2, &status );


  datFind( loc1, "ONEDCHAR", &loc2, &status );
  datPutVC( loc2, 3, chararr, &status );

  /* Check contents */
  datGetVC(loc2, 3, 1024, buffer, retchararr, &actval, &status);
  if (status == SAI__OK) {
    if (actval == 3) {
      for (i = 0; i < 3; i++ ) {
        if (strncmp( chararr[i], retchararr[i], strlen(chararr[i]) ) ) {
           status = DAT__DIMIN;
           emsSetc( "IN", chararr[i]);
           emsSetc( "OUT", retchararr[i] );
           emsRep( "GET1C","Values from Get1C differ (^IN != ^OUT)", &status);
           break;
         }
      }
    } else {
      status = DAT__DIMIN;
      emsRep( "GET1C","Did not get back as many strings as put in", &status);
    }
  }

  datAnnul(&loc2, &status );


  datFind( loc1, "ONEDD", &loc2, &status );
  datPutVD( loc2, 2, darr, &status );

  /* Check contents */
  datGetVD( loc2, 2, retdarr, &actval, &status);
  if (status == SAI__OK) {
    if (actval == 2) {
      for (i = 0; i < 2; i++ ) {
         if (darr[i] != retdarr[i]) {
           status = DAT__DIMIN;
           emsRep( "GETVD","Values from getVD differ", &status);
           break;
         }
      }
    } else {
      status = DAT__DIMIN;
      emsRep( "GETVD","Did not get back as many values as put in", &status);
    }
  }

  /* Try mapping - _DOUBLE */
  dimd[0] = 2;
  datMapD(loc2, "READ", 1, dimd, &mapd, &status);
  if (status == SAI__OK) {
      for (i = 0; i < 2; i++ ) {
         if (darr[i] != mapd[i]) {
           status = DAT__DIMIN;
           emsRep( "MAPD","Values from MapD differ", &status);
           break;
         }
      }
  }
  datUnmap(loc2, &status);

  /* Try mapping - _FLOAT */
  datMapR(loc2, "READ", 1, dimd, &mapf, &status);
  if (status == SAI__OK) {
      for (i = 0; i < 2; i++ ) {
         if ( (float)darr[i] != mapf[i]) {
           status = DAT__DIMIN;
           emsRep( "MAPR","Values from MapR differ", &status);
           break;
         }
      }
  }
  datUnmap(loc2, &status);

  /* Find and map DATA_ARRAY */
  datFind( loc1, "DATA_ARRAY", &loc2, &status );
  datMapV( loc2, "_REAL", "WRITE", &mapv, &nel, &status );
  mapf = mapv;
  if (status == SAI__OK) {
    nelt = dim[0] * dim[1];
    if ( nelt != nel) {
      status = DAT__FATAL;
      emsSeti( "NEL", (int)nel );
      emsSeti( "NORI", (int)nelt );
      emsRep( "SIZE","Number of elements originally (^NORI) not the same as now (^NEL)", &status);
    }
  }
  sumd = 0.0;
  for (i = 1; i <= nel; i++) {
    mapf[i-1] = (float)i;
    sumd += (double)i;
  }
  datUnmap( loc2, &status );
  datAnnul( &loc2, &status );

  /* Close the file */
  datAnnul( &loc1, &status );


  /* Re-open */
  hdsOpen( path, "UPDATE", &loc1, &status );

  /* Look for the data array and map it */
  datFind( loc1, "DATA_ARRAY", &loc2, &status );
  /*
  datVec( loc2, &loc3, &status );
  datSize( loc3, &nel, &status);
  if (status == SAI__OK) {
    nelt = dim[0] * dim[1];
    if ( nelt != nel) {
      status = DAT__FATAL;
      emsSeti( "NEL", (int)nel );
      emsSeti( "NORI", (int)nelt );
      emsRep( "SIZE","Number of elements before (^NORI) not the same as now (^NEL)", &status);
    }
  }

  datAnnul( &loc3, &status );
  */

  datPrec( loc2, &nbytes, &status );
  if (status == SAI__OK) {
    if ( nbytes != 4) {
      status = DAT__FATAL;
      emsSeti( "NB", nbytes );
      emsRep( "PREC","Precision for _REAL not 4 bytes but ^NB", &status);
    }
  }

  datMapV( loc2, "_INTEGER", "READ", &mapv, &nel, &status );
  mapi = mapv;
  if (status == SAI__OK) {
    nelt = dim[0] * dim[1];
    if ( nelt != nel) {
      status = DAT__FATAL;
      emsSeti( "NEL", (int)nel );
      emsSeti( "NORI", (int)nelt );
      emsRep( "SIZE","Number of elements originally (^NORI) not the same as now (^NEL)", &status);
    }
  }
  sumi = 0;
  for (i = 0; i < nel; i++) {
    sumi += mapi[i];
  }
  datUnmap( loc2, &status );

  if (status == SAI__OK) {
    if (sumi != (int)sumd) {
      status = DAT__FATAL;
      emsSeti( "I", sumi );
      emsSeti( "D", (int)sumd );
      emsRep("SUM","Sum was not correct. Got ^I rather than ^D", &status );
    }
  }

  /* _INT64 test */
  datMapV( loc2, "_INT64", "READ", &mapv, &nel, &status );
  mapi64 = mapv;
  if (status == SAI__OK) {
    nelt = dim[0] * dim[1];
    if ( nelt != nel) {
      status = DAT__FATAL;
      emsSeti( "NEL", (int)nel );
      emsSeti( "NORI", (int)nelt );
      emsRep( "SIZE","Number of elements originally (^NORI) not the same as now (^NEL)", &status);
    }
  }
  sumi64 = 0;
  for (i = 0; i < nel; i++) {
    sumi64 += mapi64[i];
  }
  datUnmap( loc2, &status );

  if (status == SAI__OK) {
    if (sumi64 != (int)sumd) {
      status = DAT__FATAL;
      emsSeti( "I", (int)sumi64 );
      emsSeti( "D", (int)sumd );
      emsRep("SUM","Sum was not correct. Got ^I rather than ^D", &status );
    }
  }

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

static void cmpszints( size_t result, size_t expected, int *status ) {
  if (*status != SAI__OK) return;
  if ( result != expected ) {
    *status = DAT__FATAL;
    emsRepf("", "Got int '%zu' but expected '%zu'", status,
            result, expected );
  }
  return;
}

static void cmpprec ( const HDSLoc * loc1, const char * name, int * status ) {
    HDSLoc * locator = NULL;
    size_t complen = 0;
    size_t compprec = 0;

    datFind( loc1, name, &locator, status);
    datPrec( locator, &compprec, status);
    datLen( locator, &complen, status);
    datAnnul(&locator, status );
    if ( compprec != complen ) {
      *status = DAT__FATAL;
      printf("%s precision: %zu length: %zu\n", name, compprec, complen);
    }
}

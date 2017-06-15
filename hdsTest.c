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

static void traceme (const HDSLoc * loc, const char * expected, int explev,
                     int *status);
static void cmpstrings( const char * teststr, const char * expectedstr, int *status );
static void cmpszints( size_t result, size_t expected, int *status );
static void cmpprec ( const HDSLoc * loc1, const char * name, int * status );
static void cmpintarr( size_t nelem, const int result[],
                       const int expected[], int *status );
static void testSliceVec();

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
  const hdsbool_t boolarr[] = { 1, 0, 1 };
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

  /* Create a 2-D array that we can test slice and vectored slices */
  if (status == SAI__OK) {
    const hdsdim vdim[] = { 5, 6 };
    int * ipntr = NULL;
    datNew( loc1, "VEC_TEST", "_INTEGER", 2, vdim, &status );
    datFind( loc1, "VEC_TEST", &loc2, &status );
    /* Fill sequentially */
    datMapI( loc2, "WRITE", 2, vdim, &ipntr, &status );
    if (status == SAI__OK) {
      int i;
      int nelem = vdim[0] * vdim[1];
      for (i = 0; i<nelem; i++) {
        ipntr[i] = i+1;
      }
      datUnmap( loc2, &status );
    }
    if (status == SAI__OK) {
      /* First we get a slice */
      const hdsdim lower[] = { 3, 3 };
      const hdsdim upper[] = { 4, 4 };
      const hdsdim outdims[] = { 2, 2 };
      const int expected[] = { 13, 14, 18, 19 };
      int outdata[4];
      datSlice(loc2, 2, lower, upper, &loc3, &status );
      datGetI(loc3, 2, outdims, outdata, &status);
      cmpintarr( 4, outdata, expected, &status );
      datAnnul( &loc3, &status );
    }
    if (status == SAI__OK) {
      /* Vectorize and slice */
      const hdsdim lower[] = { 13 };
      const hdsdim upper[] = { 16 };
      int outdata[4];
      const int expected[] = { 13, 14, 15, 16 };
      HDSLoc * loc4 = NULL;
      size_t actvals;
      datVec(loc2, &loc3, &status );
      datSlice( loc3, 1, lower, upper, &loc4, &status );
      dat1DumpLoc( loc4, &status );
      datGet1I( loc4, 4, outdata, &actvals, &status );
      cmpintarr( actvals, outdata, expected, &status);
      datAnnul(&loc4, &status);
      datAnnul(&loc3, &status);
    }
    datAnnul( &loc2, &status );
  }

  /* Now create an array of structures */
  /* Create structure array */
  {
    hdsdim histdim[] = { 5, 2 };
    hdsdim subs[] = { 3, 2 };
    char namestr[DAT__SZNAM+1];
    char opstr[2048];
    HDSLoc * loc4 = NULL;
    datNew( loc1, "RECORDS", "HIST_REC", 2, histdim, &status );
    datFind( loc1, "RECORDS", &loc2, &status );
    datCell( loc2, 2, subs, &loc3, &status );
    datNew0I( loc3, "INTINCELL", &status );
    datFind( loc3, "INTINCELL", &loc4, &status );
    datName( loc2, namestr, &status );
    cmpstrings( namestr, "RECORDS", &status );
    datName( loc3, namestr, &status );
    cmpstrings( namestr, "RECORDS(3,2)", &status );
    datRef( loc2, opstr, sizeof(opstr), &status);
    if (status == SAI__OK) printf("datRef structure array: %s\n", opstr);
    datRef( loc3, opstr, sizeof(opstr), &status);
    if (status == SAI__OK) printf("datRef cell: %s\n", opstr);
    traceme(loc3, "HDS_TEST.RECORDS(3,2)", 2, &status);
    traceme(loc4, "HDS_TEST.RECORDS(3,2).INTINCELL", 3, &status);
    datAnnul( &loc4, &status );
    datAnnul( &loc3, &status );
    datAnnul( &loc2, &status );
  }

  /* Now check the type of the root group */
  datType( loc1, typestr, &status );
  cmpstrings( typestr, "NDF", &status );

  {
    int struc;
    int prim;
    int ncomp;
    int defined;
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

    datClone( loc2, &loc3, &status );
    datName( loc3, namestr, &status );
    cmpstrings( namestr, "TESTSTRUCT", &status );
    datAnnul( &loc3, &status );

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
    datNew0D( loc2, "UNDEFINED", &status );
    datNew0D( loc2, "NEVERWRITE", &status );

    datNcomp( loc2, &ncomp, &status );
    if (status == SAI__OK) {
      const int nexpected = 10;
      if (ncomp != nexpected) {
        status = DAT__FATAL;
        emsRepf("", "Got %d components in structure rather than %d\n",
                &status, ncomp, nexpected );
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

    /* Fill in some info so that we know it copies correctly
       later - use CHAR type throughout */
    datFind( loc2, "CHAR", &loc3, &status );
    datPut0C( loc3, "a test", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "DOUBLE", &loc3, &status );
    datPut0C( loc3, "55.6", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "INT64", &loc3, &status );
    datPut0C( loc3, "42", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "INTEGER", &loc3, &status );
    datPut0C( loc3, "21", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "LOGICAL", &loc3, &status );
    datPut0C( loc3, "T", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "REAL", &loc3, &status );
    datPut0C( loc3, "3.141", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "UWORD", &loc3, &status );
    datPut0C( loc3, "32", &status );
    datAnnul( &loc3, &status );
    datFind( loc2, "WORD", &loc3, &status );
    datPut0C( loc3, "-32", &status );
    datAnnul( &loc3, &status );

    datFind( loc2, "UNDEFINED", &loc3, &status );
    datState(loc3, &defined, &status );
    if (status == SAI__OK && defined) {
      status = SAI__ERROR;
      emsRep("","Should not have been defined", &status );
    }
    datPut0C( loc3, "55.678", &status );
    datState(loc3, &defined, &status );
    if (status == SAI__OK && !defined) {
      status = SAI__ERROR;
      emsRep("","Should have been defined", &status );
    }
    datReset(loc3, &status);
    datState(loc3, &defined, &status );
    if (status == SAI__OK && defined) {
      status = SAI__ERROR;
      emsRep("","Should not have been defined after reset", &status );
    }
    datAnnul( &loc3, &status );

    /* Now ask whether the component we never wrote to is defined */
    datFind(loc2, "NEVERWRITE", &loc3, &status );
    datState(loc3, &defined, &status );
    if (status == SAI__OK && defined) {
      status = SAI__ERROR;
      emsRep("","Should not have been defined as we never wrote", &status );
    }
    datAnnul( &loc3, &status );
    datAnnul( &loc2, &status );
  }

  /* Add a couple of locators to a group and free it */
  {
    char grpnam[DAT__SZGRP+1];
    datFind( loc1, "TESTSTRUCT", &loc2, &status );
    hdsLink( loc2, "TEST", &status );
    datFind( loc2, "WORD", &loc3, &status );
    hdsLink( loc3, "TEST", &status );
    datFind( loc2, "DOUBLE", &loc3, &status );
    hdsLink( loc3, "TEST", &status );
    hdsGroup( loc3, grpnam, &status );
    cmpstrings( grpnam, "TEST", &status );
    hdsFlush( "TEST", &status );
  }

  /* Check that we can not ask for the parent of the
     root locator */
  if (status == SAI__OK) {
    int trigger = 0;
    emsMark();
    datParen( loc1, &loc3, &status );
    if (status == SAI__OK) {
      trigger = 1;
    } else {
      emsAnnul( &status );
    }
    emsRlse();
    if (trigger) {
      status = DAT__FATAL;
      emsRep("", "Was able to obtain parent locator of root locator!",
             &status );
    }
  }

  /* Confirm size and type */
  if (status == SAI__OK) {
    size_t dsize;
    datFind( loc1, "DATA_ARRAY", &loc2, &status );
    datParen( loc2, &loc3, &status );
    datName( loc3, namestr, &status );
    datAnnul( &loc3, &status );
    cmpstrings( namestr, "HDS_TEST", &status );

    datClone( loc2, &loc3, &status );
    datName( loc3, namestr, &status );
    cmpstrings( namestr, "DATA_ARRAY", &status );
    datAnnul( &loc3, &status );

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
  traceme( loc2, "HDS_TEST.TESTI64", 2, &status );

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

    /* Now for datRef */
    datRef( loc2, opstr, sizeof(opstr), &status);
    printf("datRef: %s\n", opstr);
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

  /* Copy the primitive */
  datCcopy( loc2, loc1, "ONEDCHARCPY", &loc3, &status);
  {
    char type2str[DAT__SZTYP];
    datType( loc2, typestr, &status );
    datType( loc3, type2str, &status );
    cmpstrings(type2str, type2str, &status );
  }
  datAnnul(&loc3, &status);

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
           emsRepf( "MAPD","Values from MapD differ (e.g. element %d : %f != %f)",
                    &status, (int)i, darr[i], mapd[i] );
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
  datAnnul(&loc2, &status);

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
  if (status == SAI__OK) {
    sumd = 0.0;
    for (i = 1; i <= nel; i++) {
      mapf[i-1] = (float)i;
      sumd += (double)i;
    }
  }
  datUnmap( loc2, &status );
  datAnnul( &loc2, &status );

  /* See if we can rename something and copy it*/
  datFind( loc1, "TESTSTRUCT", &loc2, &status );
  datRenam( loc2, "STRUCT2", &status );
  datCcopy( loc2, loc1, "STRUCT3", &loc3, &status);
  {
    char type2str[DAT__SZTYP];
    datType( loc2, typestr, &status );
    datType( loc3, type2str, &status );
    cmpstrings(type2str, type2str, &status );
  }
  datAnnul( &loc3, &status);
  datFind( loc2, "CHAR", &loc3, &status );
  datRenam( loc3, "CHAR*12", &status );
  datName( loc3, namestr, &status );
  cmpstrings( namestr, "CHAR*12", &status );
  datAnnul( &loc3, &status );

  /* Copy the structure to a new location */
  datCopy( loc2, loc1, "COPIEDSTRUCT", &status );
  datFind( loc1, "COPIEDSTRUCT", &loc3, &status );
  datName( loc3, namestr, &status );
  cmpstrings( namestr, "COPIEDSTRUCT", &status );
  datAnnul( &loc3, &status );

  datAnnul( &loc2, &status);

  /* Close the file */
  datAnnul( &loc1, &status );

  printf("Query file status:\n");
  hdsShow("FILES", &status);
  printf("Query Locator status:\n");
  hdsShow("LOCATORS", &status);

  /* Re-open */
  hdsOpen( path, "UPDATE", &loc1, &status );

  /* Look for the data array and map it */
  datFind( loc1, "DATA_ARRAY", &loc2, &status );
  printf("Query files after reopen:\n");
  hdsShow("FILES", &status);
  printf("Query locators after 2 locators created:\n");
  hdsShow("LOCATORS", &status);

  /* Count the number of primary locators */
  {
    int refct = 0;
    hdsbool_t prmry = 1;
    datRefct( loc2, &refct, &status );
    cmpszints( refct, 1, &status );
    datPrmry( 1, &loc2, &prmry, &status );
    datRefct( loc2, &refct, &status );
    cmpszints( refct, 2, &status );
    prmry = 0;
    datPrmry( 1, &loc2, &prmry, &status );
    datRefct( loc2, &refct, &status );
    cmpszints( refct, 1, &status );
  }
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

/* Test slicing and vectorising. */
  testSliceVec();

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

static void cmpintarr( size_t nelem, const int result[],
                       const int expected[], int *status ) {
  size_t j;
  if (*status != SAI__OK) return;
  for (j=0; j<nelem; j++) {
    if (result[j] != expected[j]) {
      *status = SAI__ERROR;
      emsRepf("","Error in integer array (element %zu: %d != %d)\n",
              status, j, result[j], expected[j]);
      break;
    }
  }
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

    if (*status != SAI__OK) return;
    datFind( loc1, name, &locator, status);
    datPrec( locator, &compprec, status);
    datLen( locator, &complen, status);
    datAnnul(&locator, status );
    if ( compprec != complen ) {
      *status = DAT__FATAL;
      printf("%s precision: %zu length: %zu\n", name, compprec, complen);
    }
}

static void traceme (const HDSLoc * loc, const char * expected, int explev,
                     int *status) {
  char path_str[1024];
  char file_str[2048];
  int nlev;
  hdsTrace( loc, &nlev, path_str, file_str,
            status, sizeof(path_str),
            sizeof(file_str));
  if (*status == SAI__OK) {
    printf("File: '%s' Path: '%s' Level = %d\n", file_str,
           path_str, nlev);
  }
  if (expected) cmpstrings( path_str, expected, status);
  if (explev > 0) cmpszints( nlev, explev, status);
}








#define SIZE 10

static  void testSliceVec(){
   int status_value = SAI__OK;
   int *status = &status_value;
   HDSLoc *loc1 = NULL;
   HDSLoc *loc2 = NULL;
   HDSLoc *loc3 = NULL;
   HDSLoc *loc4 = NULL;
   HDSLoc *loc5 = NULL;
   int invals[SIZE*SIZE];
   int outvals[SIZE*SIZE];
   hdsdim dims[2];
   hdsdim lo[2], hi[2];
   int i;
   size_t size;
   int *ip;

/* Create a 2-dimensional 10x10 int array. */
   dims[0] = SIZE;
   dims[1] = SIZE;
   datTemp( "_INTEGER", 2, dims, &loc1, status );

/* Set the value in each element of the array equal to the element's
   one-based index. */
   for( i = 0; i < SIZE*SIZE; i++ ) invals[ i ] = i + 1;
   datPut( loc1, "_INTEGER", 2, dims, invals, status );

/* Check the size is right. */
   datSize( loc1, &size, status );
   if( size != 100 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 1: Got %zu but expected 100", status,
              size );
   }

/* Extract a contiguous slice. */
   lo[0] = 1;
   hi[0] = 10;
   lo[1] = 2;
   hi[1] = 9;
   datSlice( loc1, 2, lo, hi, &loc2, status );
   datSize( loc2, &size, status );
   if( size != 80 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 2: Got %zu but expected 80", status,
              size );
   }

/* Vectorise it. */
   datVec( loc2, &loc3, status );
   datSize( loc3, &size, status );
   if( size != 80 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 3: Got %zu but expected 80", status,
              size );
   }

/* Check the values in the vectorised slice. */
   dims[0] = 1;
   datCell( loc3, 1, dims, &loc4, status );
   datGet0I( loc4, outvals, status );
   if( outvals[0] != 11 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 4: Got %zu but expected 11", status,
              outvals[0] );
   }
   datAnnul( &loc4, status );

   dims[0] = 80;
   datCell( loc3, 1, dims, &loc4, status );
   datGet0I( loc4, outvals, status );
   if( outvals[0] != 90 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 5: Got %zu but expected 90", status,
              outvals[0] );
   }
   datAnnul( &loc4, status );

/* Try mapping the vectorised array. */
   dims[ 0 ] = 80;
   datMapI( loc3, "Read", 1, dims, &ip, status );
   for( i = 0; i < dims[ 0 ]; i++ ) {
      if( ip[ i ] != i + 11 && *status == SAI__OK ) {
         *status = DAT__FATAL;
         emsRepf("", "testSliceVec error 6: Got %zu but expected %zu for "
                 "element %zu", status, ip[ i ], i + 11, i );
         break;
      }
   }

/* Take a 1D slice of the vectorised slice. */
   lo[0] = 2;
   hi[0] = 10;
   datSlice( loc3, 2, lo, hi, &loc4, status );
   datSize( loc4, &size, status );
   if( size != 9 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 7: Got %zu but expected 9", status,
              size );
   }

/* Check the values. */
   dims[0] = 1;
   datCell( loc4, 1, dims, &loc5, status );
   datGet0I( loc5, outvals, status );
   if( outvals[0] != 12 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 8: Got %zu but expected 12", status,
              outvals[0] );
   }
   datAnnul( &loc5, status );

   dims[0] = 9;
   datCell( loc4, 1, dims, &loc5, status );
   datGet0I( loc5, outvals, status );
   if( outvals[0] != 20 && *status == SAI__OK ) {
      *status = DAT__FATAL;
      emsRepf("", "testSliceVec error 8: Got %zu but expected 20", status,
              outvals[0] );
   }
   datAnnul( &loc5, status );


/* Tidy up. */
   datAnnul( &loc4, status );
   datAnnul( &loc3, status );
   datAnnul( &loc2, status );
   datAnnul( &loc1, status );
}

/*
 * $Id: postgres.c,v 1.16 2004/09/29 22:38:43 rodrigo_moreno Exp $
 *
 * xHarbour Project source code:
 * PostgreSQL RDBMS low level (client api) interface code.
 *
 * Copyright 2003 Rodrigo Moreno rodrigo_moreno@yahoo.com
 * www - http://www.xharbour.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the Harbour Project gives permission for
 * additional uses of the text contained in its release of Harbour.
 *
 * The exception is that, if you link the Harbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the Harbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the Harbour
 * Project under the name Harbour.  If you copy code from other
 * Harbour Project or Free Software Foundation releases into a copy of
 * Harbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for Harbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 * See doc/license.txt for licensing terms.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <hbapi.h>
#include <hbapifs.h>
#include <hbapiitm.h>
#include <hbfast.h>
#include "libpq-fe.h"

#define _CLIPDEFS_H
#if defined(HB_OS_WIN_32_USED)
   #include <windows.h>
#endif

#define VARHDRSZ                4
#define BOOLOID	                16
#define INT8OID			20
#define INT2OID			21
#define INT4OID			23
#define TEXTOID			25
#define FLOAT4OID               700
#define FLOAT8OID               701
#define CASHOID                 790                                                                
#define BPCHAROID		1042
#define VARCHAROID		1043
#define DATEOID			1082
#define TIMEOID			1083
#define TIMESTAMPOID	        1114
#define TIMESTAMPTZOID	        1184
#define TIMETZOID		1266
#define BITOID	                1560
#define VARBITOID	        1562
#define NUMERICOID		1700

HB_FUNC(PQCONNECT)
{
    char        conninfo[128];
    PGconn      *conn;

    if (hb_pcount() == 5)
        sprintf(conninfo, "dbname = %s host = %s user = %s password = %s port = %i",
                                           hb_parcx(1), hb_parcx(2), hb_parcx(3), hb_parcx(4), (int) hb_parni(5) );

    conn = PQconnectdb(conninfo);
    hb_retptr( conn );
}

HB_FUNC(PQCLOSE)
{
    if (hb_parinfo(1))
        PQfinish(( PGconn * ) hb_parptr(1));
}

HB_FUNC(PQRESET)
{
    if (hb_parinfo(1))
        PQreset(( PGconn * ) hb_parptr(1));
}

HB_FUNC(PQCLEAR)
{
    if (hb_parinfo(1))
        PQclear(( PGresult * ) hb_parptr(1));
}

HB_FUNC(PQEXEC)
{
    PGresult   *res = NULL;

    if (hb_pcount() == 2)
        res = PQexec(( PGconn * ) hb_parptr(1), hb_parcx(2));

    hb_retptr( res );        
}

HB_FUNC(PQEXECPARAMS)
{
    PGresult   *res = NULL;
    const char **paramvalues;
    int        i;
    long       n;

    PHB_ITEM   aParam;

    if (hb_pcount() == 3)
    {
        aParam = hb_param(3,HB_IT_ARRAY);

        n = hb_arrayLen(aParam);

        paramvalues = (const char **) hb_xgrab( sizeof( char *) * n );

        for (i=0;i < n;i++)
            paramvalues[i] = hb_arrayGetCPtr( aParam, i + 1 );

        res = PQexecParams(( PGconn * ) hb_parptr(1), hb_parcx(2), n, NULL, paramvalues, NULL, NULL, 1);

        hb_xfree(paramvalues);
    }
    hb_retptr( res );        
}

HB_FUNC(PQFCOUNT)
{
    PGresult   *res;
    int nFields = 0;

    if (hb_parinfo(1))
    {
        res = ( PGresult * ) hb_parptr(1);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
                nFields = PQnfields(res);
    }

    hb_retni(nFields);
}

HB_FUNC(PQLASTREC)
{
    PGresult   *res;
    int nRows = 0;

    if (hb_parinfo(1))
    {
        res = ( PGresult * ) hb_parptr(1);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
            nRows = PQntuples(res);
    }
    hb_retni(nRows);
}

HB_FUNC(PQGETVALUE)
{
    PGresult   *res;
    int         nRow, nCol;

    if (hb_pcount() == 3)
    {
        res = ( PGresult * ) hb_parptr(1);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            nRow = hb_parni(2) - 1;
            nCol = hb_parni(3) - 1;

            if (! PQgetisnull(res, nRow, nCol))
                hb_retc(PQgetvalue(res, nRow, nCol));
        }
    }
}

HB_FUNC(PQGETLENGTH)
{
    PGresult   *res;
    int         nRow, nCol;
    int         result = 0;

    if (hb_pcount() == 3)
    {
        res = ( PGresult * ) hb_parptr(1);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            nRow = hb_parni(2) - 1;
            nCol = hb_parni(3) - 1;

            result = PQgetlength(res, nRow, nCol);
        }
    }
    hb_retni(result);
}

HB_FUNC(PQMETADATA)
{
    PGresult   *res;
    int         nFields, i;
    
    HB_ITEM aTemp;
    HB_ITEM aNew;
    HB_ITEM temp;

    aTemp.type = HB_IT_NIL;
    aNew.type = HB_IT_NIL;
    temp.type = HB_IT_NIL;

    if (hb_parinfo(1))
    {
        res = ( PGresult * ) hb_parptr(1);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
        {
            nFields = PQnfields(res);

            hb_arrayNew( &aNew, 0 );

            for (i=0; i < nFields; i++ )
            {
                char    buf[256];
                Oid     type_oid = PQftype( res, i );
                int     typemod = PQfmod( res, i );
                int     length = 0;
                int     decimal = 0;

                switch (type_oid)
                {
                        case BITOID:
                                if (typemod >= 0)
                                    length = (int) typemod;
                                    
                                strcpy(buf, "bit");
                                break;
        
                        case BOOLOID:
                                length = 1;
                                strcpy(buf, "boolean");                                
                                break;
        
                        case BPCHAROID:
                                if (typemod >= 0)
                                    length = (int) (typemod - VARHDRSZ);
                                    
                                strcpy(buf, "character");
                                break;
        
                        case FLOAT4OID:
                                strcpy(buf, "real");
                                break;
        
                        case FLOAT8OID:
                                strcpy(buf, "double precision");
                                break;
        
                        case INT2OID:
                                strcpy(buf, "smallint");
                                break;
        
                        case INT4OID:
                                strcpy(buf, "integer");
                                break;
        
                        case INT8OID:
                                strcpy(buf, "bigint");
                                break;
        
                        case NUMERICOID:
                                length = ((typemod - VARHDRSZ) >> 16) & 0xffff;
                                decimal = (typemod - VARHDRSZ) & 0xffff;
                                strcpy(buf, "numeric");
                                break;
                
                        case DATEOID:
                                strcpy(buf, "date");
                                break;
        
                        case TIMEOID:
                        case TIMETZOID:
                                strcpy(buf, "timezone");
                                break;
                                        
                        case TIMESTAMPOID:
                        case TIMESTAMPTZOID:
                                strcpy(buf, "timestamp");
                                break;
        
                        case VARBITOID:
                                if (typemod >= 0)
                                        length = (int) typemod;
                                        
                                strcpy(buf, "bit varying");
                                break;
        
                        case VARCHAROID:
                                if (typemod >= 0)
                                        length = (int) (typemod - VARHDRSZ);
                                
                                strcpy(buf, "character varying");
                                break;

                        case TEXTOID:
                                strcpy(buf, "text");
                                break;

                        case CASHOID:
                                strcpy(buf, "money");
                                break;
                                            
                        default:
                                strcpy(buf, "not supported");
                                break;                                
                }
                
                hb_arrayNew( &aTemp, 6 );
                
                hb_itemPutC( &temp, PQfname( res, i ) );
                hb_arraySetForward( &aTemp, 1, &temp );
                
                hb_itemPutC( &temp, buf );
                hb_arraySetForward( &aTemp, 2, &temp);

                hb_itemPutNI( &temp, length );
                hb_arraySetForward( &aTemp, 3, &temp);

                hb_itemPutNI( &temp, decimal );
                hb_arraySetForward( &aTemp, 4, &temp);

                hb_itemPutNL( &temp, PQftable( res, i ) );
                hb_arraySetForward( &aTemp, 5, &temp);

                hb_itemPutNI( &temp, PQftablecol( res, i ) );
                hb_arraySetForward( &aTemp, 6, &temp);

                hb_arrayAddForward(&aNew, &aTemp);
            }

            hb_itemForwardValue( &(HB_VM_STACK).Return, &aNew);
        }
    }
}

HB_FUNC(PQTRANSACTIONSTATUS)
{
    if (hb_parinfo(1))
        hb_retni(PQtransactionStatus(( PGconn * ) hb_parptr(1) ));
}

HB_FUNC(PQERRORMESSAGE)
{
    if (hb_parinfo(1))
        hb_retc(PQerrorMessage(( PGconn * ) hb_parptr(1) ));
}

HB_FUNC(PQSTATUS)
{
    if (hb_parinfo(1))
        hb_retni(PQstatus(( PGconn * ) hb_parptr(1) ));
}

HB_FUNC(PQRESULTERRORMESSAGE)
{
    if (hb_parinfo(1))
        hb_retc(PQresultErrorMessage(( PGresult * ) hb_parptr(1)));
}

HB_FUNC(PQRESULTSTATUS)
{
    if (hb_parinfo(1))
        hb_retni(PQresultStatus(( PGresult * ) hb_parptr(1) ));
}


HB_FUNC(PQCMDSTATUS)
{
    if (hb_parinfo(1))
        hb_retc(PQcmdStatus( (PGresult *) hb_parptr(1) ));
}


HB_FUNC(PQCMDTUPLES)
{
    if (hb_parinfo(1))
        hb_retc(PQcmdTuples( (PGresult *) hb_parptr(1) ));
}



/* Asynchronous functions 
 * ----------------------
 *
 * With this functions, we can send multiples queries using the PQsendQuery, just separate by ";".
 * Use PQgetResult to return result pointer, but use PQconsumeInput once and PQisbusy to check if there is result
 *
*/

HB_FUNC(PQSENDQUERY)
{
    int res = 0;        

    if (hb_pcount() == 2)
        res = PQsendQuery(( PGconn * ) hb_parptr(1), hb_parcx(2));

    hb_retl( res );        
}

HB_FUNC(PQGETRESULT)
{
    PGresult   *res = NULL;

    if (hb_parinfo(1))
        res = PQgetResult(( PGconn * ) hb_parptr(1));

    /* when null, no more result to catch */
    if (res)
        hb_retptr( res );        
}

HB_FUNC(PQCONSUMEINPUT)
{
    int res = 0;        

    if (hb_parinfo(1))
        res = PQconsumeInput(( PGconn * ) hb_parptr(1));

    hb_retl( res );        
}

HB_FUNC(PQISBUSY)
{
    int res = 0;        

    if (hb_parinfo(1))
        res = PQisBusy(( PGconn * ) hb_parptr(1));

    hb_retl( res );        
}

HB_FUNC(PQREQUESTCANCEL)
{
    int res = 0;        

    if (hb_parinfo(1))
        res = PQrequestCancel(( PGconn * ) hb_parptr(1));

    hb_retl( res );        
}


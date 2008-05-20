/*
 * $Id: crc32.h,v 1.1 2008/04/14 06:06:22 andijahja Exp $
 */

/*
 * << Haru Free PDF Library 2.0.0 >> -- hpdf_info.c
 *
 * Copyright (c) 1999-2006 Takeshi Kanno <takeshi_kanno@est.hi-ho.ne.jp>
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 * It is provided "as is" without express or implied warranty.
 *
 */


#ifndef _HPDF_INFO_H
#define _HPDF_INFO_H

#include "hpdf_objects.h"

#ifdef __cplusplus
extern "C" {
#endif


HPDF_STATUS
HPDF_Info_SetInfoAttr (HPDF_Dict        info,
                       HPDF_InfoType    type,
                       const char  *value,
                       HPDF_Encoder     encoder);


const char*
HPDF_Info_GetInfoAttr (HPDF_Dict      info,
                       HPDF_InfoType  type);


HPDF_STATUS
HPDF_Info_SetInfoDateAttr (HPDF_Dict      info,
                           HPDF_InfoType  type,
                           HPDF_Date      value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HPDF_INFO_H */


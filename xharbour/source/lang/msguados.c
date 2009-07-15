/*
 * $Id: msguados.c,v 1.1 2008/11/19 05:24:52 ptsarenko Exp $
 */

/*
 * Harbour Project source code:
 * Language Support Module (UA1125)
 *
 * Copyright 2009 Pavel Tsarenko <tpe2@mail.ru>
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
 */

/* Language name: Ukrainian */
/* ISO language code (2 chars): UA */
/* Codepage: 1125 */

#include "hbapilng.h"

static HB_LANG s_lang =
{
   {
      /* Identification */

      "UA1125",                    /* ID */
      "Ukrainian",                 /* Name (in English) */
      "������쪠",                /* Name (in native language) */
      "UA",                        /* RFC ID */
      "1125",                      /* Codepage */
      "$Revision: 1.1 $ $Date: 2009/07/14 05:24:52 $",         /* Version */

      /* Month names */

      "��祭�",
      "��⨩",
      "��१���",
      "���⥭�",
      "�ࠢ���",
      "��ࢥ��",
      "��⨩",
      "��௥��",
      "���ᥭ�",
      "���⥭�",
      "���⮯��",
      "��㤥��",

      /* Day names */

      "������",
      "���������",
      "����ப",
      "��।�",
      "��⢥�",
      "�'�⭨��",
      "�㡮�",

      /* CA-Cl*pper compatible natmsg items */

      "����� �����       # �����     ��⠭�� ��.     ������",
      "�������� � �ਪ���� ?",
      "���.N",
      "** Subtotal **",
      "* Subsubtotal *",
      "*** Total ***",
      "Ins",
      "   ",
      "����ୠ ���",
      "��������: ",
      " - ",
      "�/�",
      "������� ���������",

      /* Error description names */

      "�������� �������",
      "����୨� ��㬥��",
      "��९������� ��ᨢ�",
      "��९������� ��ப�",
      "��९������� �᫠",
      "������� �� ���",
      "��᫮�� �������",
      "���⠪�筠 �������",
      "�㦥 ᪫���� �������",
      "",
      "",
      "�� ������ ���'���",
      "�������� �㭪���",
      "��⮤ �� ��ᯮ�⮢��",
      "������ �� ����",
      "����� �� ����",
      "������ �� ��ᯮ�⮢���",
      "�������⨬� ᨬ���� � ����� �����",
      "����� ��� ������⮢������",
      "",
      "������� �⢮७��",
      "������� ��������",
      "������� �������",
      "������� �⠭��",
      "������� �����",
      "������� ����",
      "",
      "",
      "",
      "",
      "������� �� ����ਬ������",
      "����� ��ॢ�饭",
      "����� �誮������",
      "������� ⨯� �����",
      "������� ஧���� �����",
      "���� �� ������",
      "���� �� �����ᮢ��",
      "�������� ��᪫���� �����",
      "�������� �����஢��",
      "����� �� ��������",
      "���� ����㢠��� �� �������",
      "����㢠��� �� �������",
      "",
      "",
      "",
      "",
      "����ୠ �������� ��㬥����",
      "����� �� ��ᨢ�",
      "��᢮��� ��ᨢ�",
      "�� ��ᨢ",
      "������ﭭ�",
      "Invalid self",


      /* Internal error names */

      "�����ࠢ�� ������� %lu: ",
      "������� �� �����������",
      "�� �����祭 ERRORBLOCK() ��� �������",
      "��ॢ�饭� ���� ४��ᨢ��� ������� ��஡���� �������",
      "�� ������� �����⠦�� RDD",
      "����୨� ⨯ ��⮤� %s",
      "hb_xgrab �� ���� ஧������� ���'���",
      "hb_xrealloc �������� � NULL �����������",
      "hb_xrealloc �������� � ����୨� �����������",
      "hb_xrealloc �� ���� ���஧������� ���'���",
      "hb_xfree �������� � ����୨� �����������",
      "hb_xfree �������� � NULL �����������",
      "�� �������� ���⮢� ��楤��: \'%s\'",
      "������� ���⮢� ��楤��",
      "VM: ��������� ���",
      "%s: ����㢠��� ᨬ���",
      "%s: ����୨� ⨯ ᨬ���� ��� self",
      "%s: ����㢠��� ���� ����",
      "%s: ����୨� ⨯ ������� �� ���設� �⥪�",
      "����� �� ���� �⥪�",
      "%s: �஡� ����஢�� ����� �� ᥡ�",
      "%s: ����୥ ��'� �������",
      "��९������� ���� ���'���",
      "hb_xgrab requested to allocate zero bytes",
      "hb_xrealloc requested to resize to zero bytes",
      "hb_xalloc requested to allocate zero bytes",

      /* Texts */

      "����/��/��",
      "�",
      "�"
   }
};

HB_LANG_ANNOUNCE( UA1125 );

HB_CALL_ON_STARTUP_BEGIN( hb_lang_Init_UA1125 )
   hb_langRegister( &s_lang );
HB_CALL_ON_STARTUP_END( hb_lang_Init_UA1125 )

#if defined(HB_PRAGMA_STARTUP)
   #pragma startup hb_lang_Init_UA1125
#elif defined(HB_MSC_STARTUP)
   #if defined( HB_OS_WIN_64 )
      #pragma section( HB_MSC_START_SEGMENT, long, read )
   #endif
   #pragma data_seg( HB_MSC_START_SEGMENT )
   static HB_$INITSYM hb_vm_auto_hb_lang_Init_UA1125 = hb_lang_Init_UA1125;
   #pragma data_seg()
#endif


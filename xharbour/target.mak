#===============================================================================
#
# $Id: target.mak,v 1.1 2008/04/27 14:00:45 andijahja Exp $
#
# FILE : target.mak
# NOTES: This file is used by all C/C++ compilers under Windows Platform whose
#        batch files are available in the root directory.
#
#===============================================================================
ST_PROJECT=\
	$(COMMON_LIB)\
	$(PPGEN_EXE)\
	$(PP_LIB)\
	$(HARBOUR_EXE)\
	$(VM_LIB)\
	$(FMSTAT_LIB)\
	$(RTL_LIB)\
	$(MACRO_LIB)\
	$(RDD_LIB)\
	$(TIP_LIB)\
	$(DBFFPT_LIB)\
	$(DBFNTX_LIB)\
	$(DBFCDX_LIB)\
	$(BMDBFCDX_LIB)\
	$(SIXCDX_LIB)\
	$(BMSIXCDX_LIB)\
	$(HBSIX_LIB)\
	$(HSX_LIB)\
	$(USRRDD_LIB)\
	$(RDDS_LIB)\
	$(CT_LIB)\
	$(PCREPOS_LIB)\
	$(HB_GT_LIBS)\
	$(DEBUG_LIB)\
	$(LANG_LIB)\
	$(NULSYS_LIB)\
	$(CODEPAGE_LIB)\
	$(ZLIB_LIB)\
	$(DLL_MAIN_LIB)\
	$(ODBC_LIB)\
	$(MISC_LIB)\
	$(HBPP_EXE)\
	$(HBDOC_EXE)\
	$(HBMAKE_EXE)\
	$(XBSCRIPT_EXE)\
	$(HBTEST_EXE)\
	$(HBRUN_EXE)

MT_PROJECT=\
	$(PP_LIB)\
	$(VM_LIB)\
	$(FMSTAT_LIB)\
	$(RTL_LIB)\
	$(MACRO_LIB)\
	$(RDD_LIB)\
	$(TIP_LIB)\
	$(DBFFPT_LIB)\
	$(DBFNTX_LIB)\
	$(DBFCDX_LIB)\
	$(BMDBFCDX_LIB)\
	$(SIXCDX_LIB)\
	$(BMSIXCDX_LIB)\
	$(HBSIX_LIB)\
	$(HSX_LIB)\
	$(USRRDD_LIB)\
	$(RDDS_LIB)\
	$(CT_LIB)\
	$(HBTEST_EXE)\
	$(HBRUN_EXE)

DLL_PROJECT=\
	$(HARBOUR_DLL)\
	$(HBDOCDLL_EXE)\
	$(HBRUNDLL_EXE)\
	$(HBTESTDLL_EXE)\
	$(HBMAKEDLL_EXE)\
	$(XBSCRIPTDLL_EXE)

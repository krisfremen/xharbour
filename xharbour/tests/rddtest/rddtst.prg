//#define _TEST_CREATE_

#ifndef N_LOOP
  #define N_LOOP 15
#endif

#ifndef EOL
  #define EOL chr(13)+chr(10)
#endif

#command ? => outstd(EOL)
#command ? <xx,...> => outstd(<xx>, EOL)
#command ?? =>
#command ?? <xx,...> => outstd(<xx>)

#command RDDTEST <x> => rdd_test( <x> )
#command RDDTEST <f>, <r>, <x> => rdd_test( #<f>, <{f}>, <r>, <x> )

#ifdef _TEST_CREATE_
  #command RDDTESTC <*x*> => <x>; rddtst_wr( #<x> )
  #command RDDTESTF <x> => rddtst_wr( #<x>, <x> )
  #command RDDTEST  <*x*> => RDDTESTC <x>
  #command RDDTEST  <x> => RDDTESTF <x>
#else
  #command RDDTESTC <s>, <*x*> => <x>; rddtst_tst( #<x>, <s> )
  #command RDDTESTF <r>, <s>, <*x*> => rddtst_tst( #<x>, <s>, <x>, <r> )
  //#command RDDTEST  <s>, <*x*> => RDDTESTC <x>
#endif
#define _DBNAME "_tst"

REQUEST DBFCDX
field FSTR, FNUM

#ifdef _TEST_CREATE_
  static hMake := -1
#endif
static nTested := 0
static nErrors := 0

#include "ord.ch"
#include "dbinfo.ch"

REQUEST DBSEEK, DBGOTO, DBGOTOP, DBGOBOTTOM, ORDSETFOCUS, ORDSCOPE

#ifdef _TEST_CREATE_
  function main(cOutFile, rdd)
  test_init(rdd,cOutFile)
  test_main()
  test_close()
  return nil
#else
  function main(rdd)
  test_init(rdd)
  test_main()
  test_close()
  return nil
#endif

static function test_init(rdd,cOutFile)
local n, cOut, aDb:={{"FSTR", "C", 10, 0},{"FNUM", "N", 10, 0}}

if empty(rdd)
  #ifdef _TESTRDD
    rdd:=_TESTRDD
  #else
    rdd:="DBFCDX"
  #endif
endif
rddSetDefault(rdd)
#ifdef _TEST_CREATE_
  if empty(cOutFile)
    ? "Syntax: <outfile.prg> [<rddname>]"
    quit
  elseif (hMake:=fcreate(cOutFile))==-1
    ? "Cannot create file: ", cOutFile
    quit
  endif
  cOut:=;
        'REQUEST '+rdd+EOL+;
        '#define _TESTRDD "'+rdd+'"'+EOL+;
        '#include "rddtst.prg"'+EOL+;
        EOL+;
        'FUNCTION test_main()'+EOL+;
        EOL
  if !fwrite(hMake, cOut)==len(cOut)
    ? "write error."
    quit
  endif
#endif

aeval(directory("./"+_DBNAME+".??x"),{|x|ferase(x[1])})
ferase("./"+_DBNAME+".dbf")
? "RDD: "+rdd
? "creating databse and index..."
dbcreate(_DBNAME, aDb)
/*
use _DBNAME shared

for n:=1 to N_LOOP
  dbappend()
  replace FNUM with int((n+2)/3)
  replace FSTR with chr(FNUM+48)
  //? FNUM, FSTR, recno(), eof(), bof()
next
dbcommit()
dbunlock()
*/
return nil


static function test_close()
local cOut
#ifdef _TEST_CREATE_
  if hMake != -1
    cOut:=EOL+;
          'RETURN NIL'+EOL
    if !fwrite(hMake, cOut)==len(cOut)
      ? "write error."
      quit
    endif
    fclose(hMake)
  endif
#else
  ?
  ? "Number of tests: "+ltrim(str(nTested))
  ? "Number of errors: "+ltrim(str(nErrors))
#endif
dbclosearea()
aeval(directory("./"+_DBNAME+".??x"),{|x|ferase(x[1])})
ferase("./"+_DBNAME+".dbf")
?
return nil

static procedure rdd_retval()
return

static function rdd_state()
return {recno(), bof(), eof(), found()}


static function itm2str(itm)
local cStr:="", i
if itm==NIL
  cStr+="NIL"
elseif valtype(itm)=="C"
  cStr+='"'+strtran(itm,'"','"+chr(34)+"')+'"'
elseif valtype(itm)=="N"
  cStr+=ltrim(str(itm))
elseif valtype(itm)=="L"
  cStr+=iif(itm,".t.",".f.")
elseif valtype(itm)=="D"
  cStr+="CTOD("+DTOC(itm)+")"
elseif valtype(itm)=="B"
  cStr+="{||"+itm2str(eval(itm))+"}"
elseif valtype(itm)=="A"
  cStr+="{"
  for i:=1 to len(itm)
    cStr+=iif(i==1,"",",")+itm2str(itm[i])
  next
  cStr+="}"
endif
return cStr


#ifdef _TEST_CREATE_
  static function rddtst_wr(cAction, xRet)
  local aState, cOut
  aState:=rdd_state()
  if pcount()>1
    cOut:="RDDTESTF "+itm2str(xRet)+", "+itm2str(aState)+", "+cAction+EOL
  else
    cOut:="RDDTESTC "+itm2str(aState)+", "+cAction+EOL
  endif
  if !fwrite(hMake, cOut)==len(cOut)
    ? "write error."
    quit
  endif
  return nil
#else
  //rddtst_tst( #<x>, <s>, <x>, <r> )
  static function rddtst_tst(cAction, aExState, xRet, xExRet)
  local aState, lOK:=(.t.), s1, s2, i

  aState:=rdd_state()
  if pcount()>=4
    if !valtype(xRet)==valtype(xExRet) .or.;
       !iif(valtype(xRet)=="B", eval(xRet)==eval(xExRet), xRet==xExRet)
      lOK:=(.f.)
    endif
    s1:=itm2str(xRet)
    s2:=itm2str(xExRet)
    s1:=padr(s1, max(len(s1),len(s2))+1)
    s2:=padr(s2, len(s1))
  else
    s1:=s2:=""
  endif
  if !empty(aExState) .and. lOK
    for i:=1 to len(aExState)
      if !valtype(aState[i])==valtype(aExState[i]) .or. !aState[i]==aExState[i]
        lOK:=(.f.)
        exit
      endif
    next
  endif
  ?
  ?? iif(lOK,"OK  ", "ERR ")+cAction+" => "+s1+itm2str(aState)
  if !lOK
    ?
    ?? "    "+cAction+" => "+s2+itm2str(aExState)
    nErrors++
  endif
  nTested++
  return nil
#endif


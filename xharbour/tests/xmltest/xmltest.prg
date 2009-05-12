************************************************************
* xmltest.prg
* $Id: xmltest.prg,v 1.10 2005/09/21 01:55:54 ronpinkas Exp $
*
* Test for XML routines of Xharbour rtl (MXML/HBXML)
*
* USAGE:  xmltext <cFileName> <cNode> <cAttrib> <cValue> <cData>
*   cFileName: the XML to parse (defaults to xmltest.xml)
*   cNode: if you want to test regex match on node name
*   cNode: if you want to test if a node has an attribute
*   cValue: if you want to test if a node has an attribute with a given value
*   cData: if you want to test regex match on node content
*   (You can pass NIL/unused elements by setting them to "" on the command line)
*
* (C) Giancarlo Niccolai
*

#include "fileio.ch"
#include "hbxml.ch"

PROCEDURE Main( cFileName, cNode, cAttrib, cValue, cData )
   LOCAL hFile, cXml
   LOCAL xmlDoc, xmlNode, lFind := .f.

   SET EXACT OFF
   CLS

   ? "X H A R B O U R - XML Test "

   IF cFileName == NIL
      cFileName := "xmltest.xml"
   ENDIF

   // this can happen if I call xmltest filename "" cdata
   IF ValType( cNode ) == "C" .and. Len( cNode ) == 0
      cNode := NIL
   ENDIF

   // this can happen if I call xmltest filename "" cdata
   IF ValType( cAttrib ) == "C" .and. Len( cAttrib ) == 0
      cAttrib := NIL
   ENDIF

   // this can happen if I call xmltest filename "" cdata
   IF ValType( cValue ) == "C" .and. Len( cValue ) == 0
      cValue := NIL
   ENDIF

   if ! file( cFileName )
      @3, 10 SAY "File not found: " + cFileName
      @4,10 SAY "Terminating, press any key to continue"
      Inkey( 0 )
      RETURN
   ENDIF

   lFind := (cNode != NIL .or. cAttrib != NIL .or. cValue != NIL .or. cData != NIL )

   ? "Processing "+cFileName+"..."

   oDoc := TXmlDocument():New( cFileName )
   //oNode := oDoc:oRoot:oChild
   oNode := oDoc:CurNode

   IF oDoc:nStatus != HBXML_STATUS_OK
      @4,10 SAY "Error While Processing File: "
      @5,10 SAY "On Line: " + AllTrim( Str( oDoc:nLine ) )
      @6,10 SAY "Error: " + oDoc:ErrorMsg
      @10,10 SAY "Program Terminating, press any key"
      Inkey( 0 )
      RETURN
   ENDIF

if ! lfind

   ? "XML Dump beginning here"
   ? "-----------------------"
   ? ""

   cXml := oDoc:ToString( HBXML_STYLE_NOINDENT )
   ? cXml
   ? "--- Press any key for next test"
   Inkey(0)

   ? "-----------------------"
   ? "Navigating all nodes"
   ? ""

   DO WHILE oNode != NIL
      cXml := oNode:Path()
      IF cXml == NIL
         cXml :=  "(Node without path)"
      ENDIF

      ? Alltrim( Str( oNode:nType ) ), ", ", oNode:cName, " = ", ;
            ValToPrg( oNode:aAttributes ), ", ", oNode:cData, ": ", cXml

      oNode := oDoc:Next()
   ENDDO

else

   ?
   ? "Searching for node named", cNode, ",", cAttrib, "=", cValue,;
      " with data having", cData
   ? ""

   IF cNode != NIL
      cNode := HB_RegexComp( cNode )
   ENDIF
   IF cAttrib != NIL
      cAttrib := HB_RegexComp( cAttrib )
   ENDIF
   IF cValue != NIL
      cValue := HB_RegexComp( cValue )
   ENDIF
   IF cData != NIL
      cData := HB_RegexComp( cData )
   ENDIF

   oNode := oDoc:FindFirstRegex( cNode, cAttrib, cValue, cData )

   WHILE oNode != NIL
      ? "Found node ", oNode:Path() , ValToPrg( oNode:ToArray() )
      oNode := oDoc:FindNext()
   ENDDO

endif

   ? 
   ? "Terminated. Press any key to continue"
   Inkey( 0 )
   ?

RETURN

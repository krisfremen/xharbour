
/****
*
*     oList.prg
*
*     Generates Javascript & DHTML list menus
*     (see the website/jList dir for an example)
*
*     Uses list.js and resize.js (heavily modified) found at
*     developer.Netscape.com
*
*
*     (c) 1999-2000 Manos Aspradakis, Greece
*     eMail : maspr@otenet.gr
*
*/

#include "hbclass.ch"
#include "html.ch"
#include "default.ch"

PROC TestJList()

   LOCAL o

   o := ncList():new(, .T., 200, 22 )   //, "#336699",, "white" )
   o:newNode( "node1" )                 //,,,,"lightblue"  )
   o:AddLink( "test", "test" )
   o:AddLink( "test", "test" )
   o:AddLink( "test", "test" )
   o:endNode( "node1", "NODE #1" )
   o:fontColor := "white"
   o:newNode( "node2" )                 //,,,,"lightblue" )
   o:AddLink( "test", "test" )
   o:AddLink( "test", "test" )
   o:AddLink( "test", "test" )
   o:AddLink( "Test Link", "www.test.com" )                 //, "lightblue")
   o:AddLink( "Test Link", "www.test.com" )                 //,, "lightblue")
   o:AddLink( "Test Link", "www.test.com" )                 //,, "lightblue")
   o:AddItem( "Test Link", "www.test.com" )                 //, "lightblue")
   o:AddItem( "Test Link", "www.test.com" )                 //, "lightblue")
   o:endNode( "node2", "NODE #2" )
   o:build()
   o:put( "test.htm" )
   RETURN

   /****
*     oList.prg
*
*     Implementation of the Javascript colapsible list object.
*
*     Not finished yet.
*/

CLASS NcList
   DATA nH INIT STD_OUT
   DATA aScript INIT {}
   DATA aItems INIT {}
   DATA cScript INIT ""
   DATA nTimes INIT 0
   DATA nItems INIT 0
   DATA cMainNode INIT ""
   DATA cCurrentNode INIT ""
   DATA Style INIT _WHITE_BLACK_STYLE
   DATA FONT INIT "Verdana"
   DATA Size INIT 2
   DATA BGCOLOR INIT "white"
   DATA FontColor INIT "black"

   METHOD New( name, lOpen, width, height, bgColor, ;
   FONT, fntColor, fntSize, cMinusIg, cPlusImg )

   METHOD NewNode( name, lOpen, width, height, bgColor )

   METHOD SetFont( name, font, fntColor, fntSize )

   METHOD AddItem( name, url, bgColor )

   METHOD AddLink( name, url, img, bgColor )

   METHOD EndNode( name, caption )

   METHOD Build( xPos, yPos )

   METHOD Put( cFile )

ENDCLASS

   /****
*     Create main node
*
*
*
*
*/

METHOD New( name, lOpen, width, height, bgColor, ;
               FONT, fntColor, fntSize, cMinusImg, cPlusImg ) CLASS NcList

   LOCAL cStr

   DEFAULT name := "l"
   DEFAULT lOpen := .F.
   DEFAULT WIDTH := 200
   DEFAULT HEIGHT := 22
   DEFAULT BGCOLOR := "white"
   DEFAULT FONT := "Verdana"
   DEFAULT fntColor := "black"
   DEFAULT fntSize := 2
   DEFAULT cMinusImg := "minus.gif"
   DEFAULT cPlusImg := "plus.gif"

   ::font      := FONT
   ::size      := fntSize
   ::fontColor := fntColor
   ::bgColor   := BGCOLOR

   ::nItems  := 0
   ::aSCript := {}

   cStr := "<HTML>" + CRLF() + "<HEAD>" + CRLF() + ;
           "<STYLE>" + ::Style + "</STYLE>" + CRLF() + ;
           '<SCRIPT LANGUAGE="JavaScript1.2" SRC="resize.js"></SCRIPT>' + CRLF() + ;
           CRLF() + ;
           '<SCRIPT LANGUAGE="JavaScript1.2" SRC="list.js"></SCRIPT>' + CRLF() + ;
           CRLF() + ;
           '<SCRIPT LANGUAGE="JavaScript">' + CRLF() + ;
           "<!--" + crlf() + ;
           "var " + name + ";" + CRLF() + CRLF() + ;
           "function listInit() {" + CRLF() + ;
           "var width =" + NTRIM( width ) + ";" + ;
           "var height=" + NTRIM( height ) + ";" + CRLF() + ;
           'listSetImages( "' + cMinusImg + '", "' + cPlusImg + '" );' + CRLF() + CRLF()

   ::cMainNode := name

   cStr += ""       //SPACE(10)
   cStr += name + " = new List("
   cStr += IF( lOpen, "true,", "false," )
   cStr += NTRIM( width ) + ","
   cStr += NTRIM( height ) + ","
   cStr += '"' + BGCOLOR + '"' + ");" + CRLF()
   cStr += ""       //SPACE(10)
   cStr += name + [.setFont("<FONT FACE='] + FONT + [' SIZE=] + NTRIM( fntSize ) + [' COLOR='] + fntColor + ['>","</FONT>");] + CRLF()

   ::nItems ++
   Aadd( ::aScript, cStr )

RETURN Self

/****
*
*
*
*     Add a new sub-node
*
*/

METHOD newNode( name, lOpen, width, height, bgColor ) CLASS NcList

   LOCAL cStr := ""
   DEFAULT lOpen := .F.
   DEFAULT WIDTH := 200
   DEFAULT HEIGHT := 22
   DEFAULT BGCOLOR := "white"
   cStr += ""       //SPACE(10)
   cStr += name + "= new List("
   cStr += IF( lOpen, "true,", "false," )
   cStr += NTRIM( width ) + ","
   cStr += NTRIM( height ) + ","
   cStr += '"' + BGCOLOR + '"' + ");" + CRLF()

   ::cCurrentNode := name
   ::nItems ++
   Aadd( ::aScript, cStr )

   ::setFont()

RETURN Self

/****
*
*
*
*     Set the font for an item or node
*
*/

METHOD SetFont( name, font, fntColor, fntSize ) CLASS NcList

   LOCAL cStr := ""

   DEFAULT name := ::cCurrentNode
   DEFAULT FONT := ::font
   DEFAULT fntColor := ::fontColor
   DEFAULT fntSize := ::Size

   cStr += name + [.setFont("<FONT ] + ;
      [ FACE = '] + font + [' ] + ;
      [ SIZE = ] + NTRIM( fntSize ) + ['] + ;
      [ COLOR = '] + fntColor + [' ] + ;
      [ > "," < / FONT >
   Aadd( ::aScript, cStr )
RETURN self

/****
*
*
*
*     Add a menu item
*
*/

METHOD AddItem( name, url, bgColor ) CLASS NcList

   LOCAL cStr := ""
   LOCAL cUrl := ""
   DEFAULT name := "o"
   DEFAULT url := ""
   cUrl := [<A HREF='] + url + ['>] + htmlSpace( 2 ) + name + htmlSpace( 2 )
   cStr += ::cCurrentNode + '.addItem( "' + cUrl + '"' + IF( bgColor != NIL, ',"' + bgColor + '"', "" ) + ');' + CRLF()
   ::nItems ++
   Aadd( ::aScript, cStr )
RETURN self

/****
*
*
*
*     Add a menu item
*
*/

METHOD AddLink( name, url, img, bgColor ) CLASS NcList

   LOCAL cStr := ""
   LOCAL cUrl := ""
   DEFAULT name := "o"
   DEFAULT url := ""
   DEFAULT img := "webpage.jpg"
   cUrl := "<A HREF='" + url + "'><IMG SRC='" + img + "' border=0 align=absmiddle>" + htmlSpace( 2 ) + name + htmlSpace( 2 )
   cStr += ::cCurrentNode + '.addItem( "' + curl + '"' + IF( bgColor != NIL, ',"' + bgColor + '"', "" ) + ');' + CRLF()
   ::nItems ++
   Aadd( ::aScript, cStr )
RETURN self

/****
*
*
*
*
*
*/

METHOD endNode( name, caption ) CLASS NcList

   LOCAL cStr := ""

   ::cCurrentNode := ::cMainNode
   cStr           += ::cMainNode + ".addList( " + name + ", '<B>" + caption + "</B>' );" + CRLF()

   ::nItems ++
   Aadd( ::aScript, cStr )
RETURN self

/****
*
*
*
*
*
*/

METHOD Build( xPos, yPos ) CLASS NcList

   LOCAL i    := 0
   LOCAL cStr := ""

   DEFAULT xPos := 5
   DEFAULT yPos := 5

   cStr += ::cMainNode + ".build(" + NTRIM( xPos ) + "," + NTRIM( yPos ) + ");" + CRLF()
   cStr += "}" + CRLF()
   CsTR += "// -->" + crlf()
   cStr += "</SCRIPT>" + CRLF()
   cStr += '<STYLE TYPE="text/css">' + CRLF()
   cStr += "#spacer { position: absolute; height: 1120; }" + CRLF()
   cStr += "</STYLE>" + CRLF()
   cStr += '<STYLE TYPE="text/css">' + CRLF()
   FOR i := 0 TO ::nItems + 6
      cStr += "#" + ::cMainNode + "Item" + NTRIM( i ) + " { position:absolute; }" + CRLF()
   NEXT
   cStr += "</STYLE>" + CRLF()

   Aadd( ::aScript, cStr )
   cStr := ""

   cStr += "<TITLE>Collapsable Lists: Basic Example</TITLE>" + CRLF()
   cStr += "</HEAD>" + CRLF()
   cStr += '<BODY ONLOAD="listInit();" BGCOLOR="#FFFFFF">' + CRLF()
   cStr += '<DIV ID="spacer"></DIV>' + CRLF()
   //cStr += '<DIV ID="'+::cMainNode+'Item0" NAME="'+::cMainNode+"Item0"></DIV>'+CRLF()

   FOR i := 0 TO ::nItems
      cStr += '<DIV ID="' + ::cMainNode + 'Item' + NTRIM( i ) + '" NAME="' + ::cMainNode + 'Item' + NTRIM( i ) + '"></DIV>' + CRLF()
   NEXT
   cStr += "</BODY></HTML>" + CRLF()

   Aadd( ::aScript, cStr )

RETURN Self

/****
*
*
*
*
*
*/

METHOD Put( cFile ) CLASS NcList

   IF cFile == NIL
      ::nH := STD_OUT
   ELSE
      ::nH := Fcreate( cFile )
   ENDIF

   Aeval( ::aScript, { | e | Fwrite( ::nH, e ) } )

   Fclose( ::nH )

RETURN Self

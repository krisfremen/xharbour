#include "xwt.ch"

GLOBAL oOtherBox

PROCEDURE MAIN()
   LOCAL oWindow, oButton, oMenuItem, oMenu, oMenuHelp
   LOCAL oMenuSec, oPane, oTextbox, oLabel, oViewPort
   LOCAL oImg, oHlay, oVLay, oFrame
   LOCAL oGrid

   XwtInit()
   altd()

   /*** Window creation ****/
   oWindow:= XwtFrameWindow():New("Hello World")
   oWindow:AddEventListener( XWT_E_DESTROY_REQ, @XwtQuit() )

   /*** Horizontal layout( our new main widget) ***/
   oVLay := XwtLayout():New( XWT_LM_VERT )
   oVLay:SetPadding( 5 )
   oVLay:SetBorder( 2 )
   oWindow:Add( oVLay )

   /*** Text And Button ***/
   oButton := XwtButton():New( "Hello" )
   oButton:AddListener( @DumpEvent() )
   oLabel := XwtLabel():New( "Text: " )

   // inside an horiz. layout
   oHLay := XwtLayout():New( XWT_LM_HORIZ )

   oHLay:Add( oLabel )
   oHLay:SetFill( .T. )
   oHLay:SetExpand( .T. )
   oHLay:Add( oButton )
   oHLay:SetBox( .T., "Horiz Box" )

   oVlay:Add( oHLay )

   /* A couple of Textboxes in a pane, inside a scrollable */
   oPane := XwtPane():New()

   oTextbox := XwtTextbox():New("A text", 10, 10 )
   oTextbox:AddEventListener(XWT_E_UPDATED, @BoxModified())
   oPane:Add( oTextbox  )
   oOtherBox := XwtTextbox():New("Another box", 10, 40 )
   oPane:Add( oOtherBox )
   oPane:SetBox( .T.,"A fixed pane" )
   oVLay:Add( oPane )

   /* A beautiful GRID */
   oGrid := XwtGrid():New(2,3)
   oLabel := XwtLabel():New("Field label")

   oGrid:setPadding( 2, 10 )
   oGrid:SetFill( .T. )

   oGrid:Attach( oLabel, 1, 2 )
   oGrid:Attach( XwtLabel():New("Field 2"), 2, 2 )

   oGrid:SetExpand( .T. )
   oGrid:Attach( XwtTextBox():New("Data 1"), 1, 3 )
   oGrid:Attach( XwtTextBox():New("Data 2"), 2, 3 )

   oGrid:SetBox( .T.,"A Grid " )

   oVLay:Add( oGrid )

   /*** IMAGE ***/
   oImg := XwtImage():New( "icon.png" )
   oImg:SetSensible()
   oVLay:add( oImg )

   /*** An input mask ***/
   aInputs := {;
      { "Variable 1", "Default value"}, ;
      { "Variable 2", "Default value 2"}, ;
      { "Variable 3", "Default value 3"} ;
   }
   oInput := XWTInputMask():New( aInputs )
   oInput:AddEventListener( XWT_E_CHANGED, @InputChanged() )

   oFrame := XwtLayout():New( XWT_LM_VERT )
   oViewPort := XwtViewPort():new(75, 50)
   oFrame:add( oViewPort )
   oViewPort:SetContent( oInput )
   oFrame:SetBox( .T., "Inside a scrollable..." )
   oVLay:Add( oFrame )


   /***  Radio buttons ***/
   oRadioPanel := XWTLayout():New( XWT_LM_VERT )
   oRadioPanel:add( XWTRadioButton():New( "Option 1" ) )
   oRadioPanel:add( XWTRadioButton():New( "Option 2" ) )
   oRadioPanel:add( XWTRadioButton():New( "Option 3" ) )

   oVLay:Add( oRadioPanel )
   /***** MENU design *****/
   oMenu := XwtMenu():New( "File" )

   oMenuItem := XwtMenuItem():New( "Op_en", 1 , @FileEvent())
   oMenuItem:SetIcon( "valley.png" )

   oMenu:Add( oMenuItem )
   oMenu:Add( XwtMenuItem():New( "Close", 2 , @FileEvent()))
   oMenu:Add( XwtMenuItem():New( "QUIT", 3 , @FileEvent()))

   oMenuSec := XwtMenu():New( "Submenu" )
   oMenuSec:Add( XwtMenuItem():New( "Opt1", 10 , @FileEvent()))
   oMenuSec:Add( XwtMenuItem():New( "Opt2", 11 , @FileEvent()))
   oMenu:Add( oMenuSec )

   oMenuHelp := XwtMenu():New( "Help" )
   oMenuHelp:Add( XwtMenuItem():New( "About", 5 , @FileEvent()) )
   oMenuHelp:Add( XwtMenuItem():New( "Help", 6 , @FileEvent()) )

   oWindow:SetMenuBar( { oMenu, oMenuHelp } )
   /*** Showing window ***/
   oWindow:Resize( 200, 200 )
   oWindow:Show()
   oWindow:AddListener( @WindowReceive() )

   /*** Main LOOP ***/
   XwtMainLoop()

   /*** Going to terminate */
   //oButton:Destroy()  // the button might or might not be deteached from window
   oWindow:Destroy()
RETURN



FUNCTION WindowReceive( oEvent )
   ? "Received event at top level ", oEvent:nType, " from ", oEvent:oSender:GetText()
RETURN .F.

FUNCTION DumpEvent( oEvent )
   ? "Event type: ", oEvent:nType
   IF oEvent:nType == XWT_E_CLICKED
      IF XWT_MsgBox( "Are you really, really, sure?", ;
         XWT_MSGBOX_YES + XWT_MSGBOX_NO, XWT_MSGBOX_QUESTION) == XWT_MSGBOX_YES
            oEvent:oSender:Hide()
      ENDIF
   ENDIF
RETURN .F.

FUNCTION FileEvent( oEvent )
   ? "Menu activated: ", oEvent:oSender:nId
RETURN .F.

FUNCTION BoxModified( oEvent )
   ? "Text entered in box: ", oEvent:oSender:getText()
   oEvent:oSender:SetText( "Reset" )
   oOtherBox:SetFocus()
RETURN .F.


FUNCTION InputChanged( oEvent )
   Local aField
   ? "Input has been set:"

   FOR EACH aField IN oEvent:oSender:aInputFields
      ? aField[1], ": ", aField[2]
   NEXT

   ? ""
RETURN
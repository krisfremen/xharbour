Procedure Main()

   LOCAL Counter, nStart, nEnd, nVar := 0, nOther

   nStart := Seconds()
   nTemp := Seconds()

   ? nStart
   FOR Counter := 1 TO 1000000
   NEXT
   ? "Empty Loops:", Seconds() - nStart
   ?

   nTemp := Seconds()
   FOR Counter := 1 TO 1000000
      Eval( {|p| nVar := p }, Counter )
   NEXT
   ? nVar
   ? "Eval():", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1 TO 1000000
      nVar := 777
   NEXT
   ? nVar
   ? ":= numeral:", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1 TO 1000000
      nOther := nVar + 1
   NEXT
   ? nVar, nOther
   ? ":= + Loops:", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1 TO 1000000
      nOther := nVar += 1
   NEXT
   ? nVar, nOther
   ? ":= += Loops:", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1000000 TO 1 STEP -1
      nVar -= 1
   NEXT
   ? nVar, nOther
   ? "-= Loops:", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1000000 TO 1 STEP -1
      cVar := Left( "This is some line of text about nothing what so ever - just a simple test...", 5 )
   NEXT
   ? "'" + cVar + "'"
   ? "Left() Loops:", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1000000 TO 1 STEP -1
      TestProc( cVar )
   NEXT
   ? "'" + cVar + "'"
   ? "Proc Loops:", Seconds() - nTemp
   ?

   nTemp := Seconds()
   FOR Counter := 1000000 TO 1 STEP -1
      cVar := TestFunc( cVar )
   NEXT
   ? "'" + cVar + "'"
   ? "Proc Loops:", Seconds() - nTemp
   ?

   nEnd := Seconds()
   ? nEnd
   ? "     ========"
   ? nEnd - nStart
   ?

RETURN

Procedure TestProc( x )

Return

Procedure TestFunc( x )

Return "Func"

PROCEDURE Main()

    LOCAL aaContact  := TAssociativeArray()
    LOCAL aaSalary   := TAssociativeArray()
    LOCAL aaEmployee := TAssociativeArray()
    LOCAL cKey, cSubKey, oErr

    // Array Syntax.
    aaSalary[ "Hourly" ] := 12.00
    aaSalary[ "Tax" ]  := 30.00

    // Litral Syntax.
    aaContact  := { "First" => "John", "Last" => "Doe", "Phone" => "555-1212" }

    // OOP Syntax:
    aaEmployee:Contact := aaContact
    aaEmployee:Salary  := aaSalary

    TRY
       ? aaContact[ "Hourly" ] // Error! - Does not exist in THIS Array.
    CATCH oErr
       ? "Caught:", oErr:Description, oErr:Operation
       ?
    END

    // Mixed Syntax.
    ? aaEmployee:Contact["First"]
    ? aaEmployee:Contact:Last
    ?

    // Keys extraction.
    FOR EACH cKey IN aaEmployee:Keys

       IF aaEmployee[ cKey ]:ClassName == "TASSOCIATIVEARRAY"
          ?
          ? cKey
          ? "-------"

          FOR EACH cSubKey IN aaEmployee[ cKey ]:Keys
             ? "     Key:", cSubKey
             ? "   Value:", aaEmployee[ cKey][ cSubKey ]
          NEXT
       ELSE
          ? "  Key:", cKey
          ? "Value:", aaEmployee[ cKey ]
       ENDIF
    NEXT
    ?

RETURN

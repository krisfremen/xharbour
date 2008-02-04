NAMESPACE MyNameSpace

   PROCEDURE Main()

     ? ProcName()

     // No qualifier needed for SAME level memebers.
     ? Str(1)
     // Explicit qualifier to the Global namespace
     ? *.Str(1)
     // Or for code section
     WITH NAMESPACE *
        ? Str(2)
        ? Str(3)
     END

     // No qualifier needed for SAME level memebers.
     StaticProc()
     // Or explicit.
     MyNamespace.StaticProc() // Can only be called in same compilation unit!!!

     // Sub level entities must be called explicitly!
     MyNamespace.Sub.SomeSub()

     // Call to member defined EXTERNALly by means of EXTERNAL NAMESPACE below!
     // WARNING 1: You can NOT use USING for Namespaces defined in SAME source because you'll create a cyclic dependancy!!!
     // WARNING 2: You MUST compile the EXTERNAL module FIRST!!!
     MyNameSpace.SubExtern.Ext1()

     // STATIC members of EXTERNAL namespaces can not be accessed because they belong to a different compilation unit!
     #ifdef SHOW_COMPILE_ERR
        MyNameSpace.SubExtern.SubSubExtern.SubExtStatic()
     #endif

   RETURN

   FUNCTION Str(x)
      // Using *. to call the global Str() function.
   RETURN "!" + AllTrim( *.Str(x) ) + "!"

   PROCEDURE SomeProc()
      ? ProcName()
   RETURN

   // STATIC members can not be called from OUTSIDE this compilation unit!
   STATIC PROCEDURE StaticProc()
      ? ProcName()
   RETURN

   NAMESPACE Sub

      PROCEDURE SomeSub()
         ? ProcName()

          // No qualifier needed for OUTER level memebers.
          SomeProc()
          // Or explicit.
          MyNamespace.SomeProc()

      RETURN

   END

   // Complete sub level[s] definition at external compilation unit!
   //NOTE: More than 1 namespace can use the *same* EXTERNAL NAMESPACE unit!
   // WARNING: You MUST compile the extern module FIRST!!!
   EXTERNAL NAMESPACE SubExtern

END

NAMESPACE MyNameSpace2

   PROCEDURE ProcOf2()
      ? ProcName()
   RETURN

   FUNCTION FunOf2()
   RETURN ProcName()


   FUNCTION Str(x)
      // Using *. to call the global Str() function.
   RETURN "**" + AllTrim( *.Str(x) ) + "**"

END

// Members of OPTIONAL NAMESPACE can be called WITH or withOUT the namespace qualifier!
OPTIONAL NAMESPACE MyOptional
  PROCEDURE SomeOptional()
     ? ProcName()
  RETURN
END


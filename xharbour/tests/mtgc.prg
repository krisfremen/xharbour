***********************************************************
* Multitheraded garbage collection test
*
* This program is a everlasting "work in progres", to be
* used by xHarbour core developers to test new features in
* fields covered by the library calls appearing in this
* source.
*
* (C) 2003 Giancarlo Niccolai & Ron Pinkas
*
* $Id: mtgc.prg,v 1.12 2003/03/02 15:22:31 jonnymind Exp $
*
* This programs allocates Garbage Collectable objects in
* subthreads, and force the collection in a crossed thread
* fashon (thread 1 can collect the garbage generated by
* thread 3 etc.)
*
* This is both a speed and stability stress test for all
* the MT system.
*

#include "hbclass.ch"
#include "hbmemory.ch"

PROCEDURE Main()
  LOCAL nStart;

  set color to w+/b
  CLEAR SCREEN
  @2,15 SAY "X H A R B O U R - Multithreading / Garbage collecting test"

  @4,20 SAY "TEST 1 - Test with idle fence"

  nStart := Seconds()

  // 1st param is the Startup Function, 2nd. is Self if 1st param
  // is a Method or NIL otherwise,
  // rest are paramaters to be passed to the Function/Method.
  StartThread ( @MyThreadFunc(),  6, " 1st Thread:",    0,  5000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(),  7, " 2nd Thread:",  5000, 10000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(),  8, " 3rd Thread:", 10000, 15000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(),  9, " 4th Thread:", 15000, 20000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(), 10, " 5th Thread:", 20000, 25000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(), 11, " 6th Thread:", 25000, 30000 )
  StartThread ( @MyThreadFunc(), 12, " 7th Thread:", 30000, 35000 )
  StartThread ( @MyThreadFunc(), 13, " 8th Thread:", 35000, 40000 )
  StartThread ( @MyThreadFunc(), 14, " 9th Thread:", 40000, 45000 )
  StartThread ( @MyThreadFunc(), 15, "10th Thread:", 45000, 50000 )


  WaitForThreads()
  @ 17, 2 SAY "Threads Time:" + Str( Seconds() - nStart )
  @ 18, 2 SAY "Preparing for second test - Press a key to continue"
  Inkey(0)

  CLEAR SCREEN
  ThreadIdleFence( .F. )
  @2,15 SAY "X H A R B O U R - Multithreading / Garbage collecting test"
  @4,20 SAY "TEST 2 - Test without idle fence"

  nStart := Seconds()

  StartThread ( @MyThreadFunc(),  6, " 1st Thread:",    0,  5000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(),  7, " 2nd Thread:",  5000, 10000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(),  8, " 3rd Thread:", 10000, 15000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(),  9, " 4th Thread:", 15000, 20000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(), 10, " 5th Thread:", 20000, 25000 )
  HB_GCALL( .T. )
  StartThread ( @MyThreadFunc(), 11, " 6th Thread:", 25000, 30000 )
  StartThread ( @MyThreadFunc(), 12, " 7th Thread:", 30000, 35000 )
  StartThread ( @MyThreadFunc(), 13, " 8th Thread:", 35000, 40000 )
  StartThread ( @MyThreadFunc(), 14, " 9th Thread:", 40000, 45000 )
  StartThread ( @MyThreadFunc(), 15, "10th Thread:", 45000, 50000 )

  WaitForThreads()
  @ 17, 2 SAY "Threads Time:" + Str( Seconds() - nStart )
  @ 18, 2 SAY "Program terminated - Press a key to continue"
  Inkey(0)

RETURN

PROCEDURE MyThreadFunc( nRow, cName, nStart, nMax )
  LOCAL i, aVar

  FOR i := nStart TO nMax
     @ nRow, 10 SAY cName + Str( i )

     aVar := { 1 }
     aVar[1] := Array( 50 )
     aVar[1][1] := aVar
     aVar := NIL

     @ nRow, 40 SAY "Before:" + Str( Memory( HB_MEM_USED ) )
     HB_GCALL( .T. )
     @ nRow, 60 SAY "After:" + Str( Memory( HB_MEM_USED ) )

  NEXT

RETURN


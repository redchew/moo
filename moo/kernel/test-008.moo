
#include 'Moo.moo'.

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

// TODO: use #define to define a class or use #class to define a class.
//       use #extend to extend a class
//       using #class for both feels confusing.

extend Apex
{

}

extend SmallInteger
{
	method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	method inc
	{
		^self + 1.
	}
}

class TestObject(Object)
{
	var(#class) Q, R.
	var(#classinst) a1, a2.
}


class MyObject(TestObject)
{
	var(#classinst) t1, t2, t3.
	method(#class) xxxx
	{
		| g1 g2 |
		t1 dump.
		t3 value.
		t2 := [ g1 := 50. g2 := 100. ^g1 + g2 ].
		(t1 < 10) ifFalse: [ ^self ].
		t1 := t1 + 1. 
		^self xxxx.
	}

	method(#class) main
	{
		t3 := ['1111' dump. ^20.].
		t1 := 1.
		self xxxx.
		'END OF XXX' dump.
		t2 := t2 value.  
		'END OF t2 value' dump.
		t2 dump.
	}

}

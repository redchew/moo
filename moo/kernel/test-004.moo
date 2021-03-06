//
// TEST CASES for basic methods
//

#include 'Moo.moo'.

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

class MyObject(Object)
{
	var(#class) t1 := 20.

	method(#class) test_terminate
	{
		| a s |
		s := Semaphore new.
		a := [ self.t1 := self.t1 * 9. 
		       s signal.
		       Processor activeProcess terminate.
		       self.t1 := self.t1 + 20 ] fork.
		s wait.
		^self.t1
	}

	method(#class) test_sg
	{
		| sg s1 s2 s3 |

		s1 := Semaphore new.
		s2 := Semaphore new.
		s3 := Semaphore new.

		sg := SemaphoreGroup new.

		sg addSemaphore: s1.
		sg addSemaphore: s2.
		sg addSemaphore: s3.

		s1 signalOnInput: 0.
		s2 signalOnInput: 0. // this should raise an exception as the same file descriptor is added to a different semaphore
		s3 signalOnInput: 0.

		[ sg wait. ] fork.
		[ sg wait. ] fork.
		[ sg wait. ] fork.

System sleepForSecs: 1.
		sg wait.
sg removeSemaphore: s1.
		"********** END OF TESTER *************" dump.
	}

	method(#class) main
	{
		| tc limit |

		tc := ##(
			// 0 - 4
			[self test_terminate == 180],
			[self test_sg == nil]
		).

		limit := tc size.

		0 priorTo: limit by: 1 do: [ :idx |
			| tb |
			tb := tc at: idx.
			System log(System.Log.INFO, idx asString, (if (tb value) { " PASS" } else { " FAIL" }), "\n").
		].

		"********** END OF MAIN PROGRAM *************" dump.
	}
}

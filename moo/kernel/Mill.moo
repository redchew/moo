#include 'Moo.moo'.

class Mill(Object)
{
	dcl registrar.
dcl obj.

	method initialize
	{
		self.registrar := Dictionary new.
	}

	method register: name call: callable
	{
		| k | 
		(k := self.registrar at: name) isNil ifTrue: [
			self.registrar at: name put: [
				self.obj isNil ifTrue: [
					self.obj := callable value.
				].
				self.obj
			].
		].

		^k
	}

	method make: name
	{
		| k |

		(k := self.registrar at: name) notNil ifTrue: [
			^k value.
		].

		^nil.
	}
}

class MyObject(Object)
{
	method(#class) main
	{
		| d a |

		(*k := Mill new.
		k register: #abc call: [ Dictionary new ].
		a := k make: #abc.
		##a dump.*)
		
		d := Dictionary new.
		d at: #abc put: 20.
		d at: #dddd put: 40.
		d at: #jjjj put: 'hello world'.
		d at: #moo put: 'good?'.
		d at: #moo put: 'not good?'.

		(* (d at: #abc) dump.
		(* (d at: #dddd) dump. *)
		(*d do: [:v | v dump].*)

		d keysAndValuesDo: [:k :v| System logNl: (k asString) & ' => ' & (v asString) ].

		##(d includesAssociation: (Association key: #moo value: 'not good?')) dump.
		'-------------------------' dump.
		##(System at: #MyObject) dump.
		(d removeKey: #moo) dump.
		d removeKey: #abc ifAbsent: [System logNl: '#moo not found'].
		d keysAndValuesDo: [:k :v| System logNl: (k asString) & ' => ' & (v asString) ].

		'-------------------------' dump.
		(d at: #jjjj) dump.
		(d at: #jjjja) dump.
		
		(* 
		System keysAndValuesDo: [:k :v| 
				System logNl: (k asString) & ' => ' & (v asString) 
		].
		*)
		
		##System keysDo: [:k| System logNl: (k asString)]
		
		(*[
			[Exception hash dump] ensure: ['xxxx' dump].
		] on: Exception do: [:ex | ('Exception caught - ' & ex asString) dump ].*)
	}
}
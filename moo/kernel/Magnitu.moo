class Magnitude(Object)
{
	method <  aMagnitude { self subclassResponsibility: #<	}
	method >  aMagnitude { ^aMagnitude < self }
	method <= aMagnitude { ^(aMagnitude < self) not }
	method >= aMagnitude { ^(self < aMagnitude) not }
    
	method between: min and: max 
	{
		^self >= min and self <= max
	}

	method min: aMagnitude
	{
		//^self < aMagnitude ifTrue: [self] ifFalse: [aMagnitude]
		^if (self < aMagnitude) { self } else { aMagnitude }.
	}

	method max: aMagnitude
	{
		//^self > aMagnitude ifTrue: [self] ifFalse: [aMagnitude]
		^if (self > aMagnitude) { self } else { aMagnitude }.
		
	}
}

class Association(Magnitude)
{
	var key, value.

	method(#class) key: key value: value
	{
		^self new key: key value: value
	}

	method key: key value: value
	{
		self.key := key.
		self.value := value.
	}

	method value: value
	{
		self.value := value
	}

	method key
	{
		^self.key
	}

	method value
	{
		^self.value
	}

	method = ass
	{
		^(self.key = ass key) and (self.value = ass value)
	}

	method hash
	{
		^(self.key hash) + (self.value hash)
	}
}

class(#limited) Character(Magnitude)
{
//	method(#primitive,#class) codePoint: code.
//	method(#primitive) codePoint.

	method asCharacter { ^self }
	method(#primitive) asError.
	method(#primitive) asInteger.

	method(#primitive) < char.
	method(#primitive) > char.
	method(#primitive) <= char.
	method(#primitive) >= char.

	method digitValue: anInteger
	{
		^'0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ' at: anInteger
	}

	method digitValue
	{
		//<primitive: #Character_digitValue>

		if ((self >= $0) and (self <= $9))
		{
			^self asInteger - $0 asInteger
		}
		elif ((self >= $A) and (self <= $Z))
		{
			^self asInteger - $A asInteger + 10
		}
		elif ((self >= $a) and (self <= $z))
		{
			^self asInteger - $a asInteger + 10
		}.

		//Exception signal: 'not a digit character'.
		^-1
	}
}

class(#limited) Number(Magnitude)
{
	method + aNumber
	{
		<primitive: #_number_add>
		self primitiveFailed.
	}

	method - aNumber
	{
		<primitive: #_number_sub>
		self primitiveFailed.
	}

	method * aNumber
	{
		<primitive: #_number_mul>
		self primitiveFailed.
	}

	method mul: aNumber
	{
		<primitive: #_number_mul>
		self primitiveFailed.
	}

	method mlt: aNumber
	{
		<primitive: #_number_mlt>
		self primitiveFailed.
	}

	method div: aNumber
	{
		// integer division rounded toward zero
		<primitive: #_number_div>
		self primitiveFailed.
	}

	method rem: aNumber
	{
		// integer remainder rounded toward zero
		<primitive: #Integer_rem>
		self primitiveFailed.
	}

	method mdiv: aNumber
	{
		// integer division quotient
		<primitive: #_number_mdiv>
		self primitiveFailed.
	}
	
	method mod: aNumber
	{
		// integer division remainder
		<primitive: #Integer_mod>
		self primitiveFailed.
	}
	
	//method / aNumber
	//{
	//	// fraction? fixed-point decimal? floating-point?
	//}

	method = aNumber
	{
		<primitive: #_number_eq>
		self primitiveFailed.
	}

	method ~= aNumber
	{
		<primitive: #_number_ne>
		self primitiveFailed.
	}

	method < aNumber
	{
		<primitive: #_number_lt>
		self primitiveFailed.
	}

	method > aNumber
	{
		<primitive: #_number_gt>
		self primitiveFailed.
	}

	method <= aNumber
	{
		<primitive: #_number_le>
		self primitiveFailed.
	}

	method >= aNumber
	{
		<primitive: #_number_ge>
		self primitiveFailed.
	}

	method negated
	{
		<primitive: #_number_negated>
		^0 - self.
	}

	method bitAt: index
	{
		<primitive: #Integer_bitat>
		^(self bitShift: index negated) bitAnd: 1.
	}

	method(#variadic) bitAnd(operand)
	{
		<primitive: #Integer_bitand>
		self primitiveFailed.
	}

	method bitAnd: aNumber
	{
		<primitive: #Integer_bitand>
		self primitiveFailed.
	}

	method(#variadic) bitOr(operand)
	{
		<primitive: #Integer_bitor>
		self primitiveFailed.
	}

	method bitOr: aNumber
	{
		<primitive: #Integer_bitor>
		self primitiveFailed.
	}

	method(#variadic) bitXor(operand)
	{
		<primitive: #Integer_bitxor>
		self primitiveFailed.
	}

	method bitXor: aNumber
	{
		<primitive: #Integer_bitxor>
		self primitiveFailed.
	}

	method bitInvert
	{
		<primitive: #Integer_bitinv>
		^-1 - self.
	}

	method bitShift: aNumber
	{
		/* positive number for left shift. 
		 * negative number for right shift */

		<primitive: #Integer_bitshift>
		self primitiveFailed.
	}

	method asString
	{
		^self printStringRadix: 10
	}

	method printStringRadix: aNumber
	{
		<primitive: #_number_numtostr>
		self primitiveFailed.
	}

	method to: end by: step do: block
	{
		| i |
		i := self.
		/*
		(step > 0) 
			ifTrue: [
				[ i <= end ] whileTrue: [ 
					block value: i.
					i := i + step.
				].
			]
			ifFalse: [
				[ i >= end ] whileTrue: [
					block value: i.
					i := i - step.
				].
			].
		*/
		if (step > 0)
		{
			while (i <= end)
			{
				block value: i.
				i := i + step.
			}
		}
		else
		{
			while ( i => end)
			{
				block value: i.
				i := i - step.
			}
		}
	}

	method to: end do: block
	{
		^self to: end by: 1 do: block.
	}

	method priorTo: end by: step do: block
	{
		| i |
		i := self.
		/*
		(step > 0) 
			ifTrue: [
				[ i < end ] whileTrue: [ 
					block value: i.
					i := i + step.
				].
			]
			ifFalse: [
				[ i > end ] whileTrue: [
					block value: i.
					i := i - step.
				].
			].
		*/
		if (step > 0)
		{
			while (i < end)
			{
				block value: i.
				i := i + step.
			}
		}
		else
		{
			while ( i > end)
			{
				block value: i.
				i := i - step.
			}
		}
	}
	
	method priorTo: end do: block
	{
		^self priorTo: end by: 1 do: block.
	}

	method abs
	{
		/*self < 0 ifTrue: [^self negated].
		^self.*/
		^if (self < 0) { self negated } else { self }
	}

	method sign
	{
		/* self < 0 ifTrue: [^-1].
		self > 0 ifTrue: [^1].
		^0.*/
		^if (self < 0) { -1 } elif (self > 0) { 1 } else { 0 }
	}
}

class(#limited) Integer(Number)
{
	method timesRepeat: aBlock
	{
		// 1 to: self by: 1 do: [ :count | aBlock value ].
		| count |
		count := 0.
		while (count < self) { aBlock value. count := count + 1 }
	}
	
	method asInteger { ^self }

	// integer has the scale of 0.
	method scale { ^0 }

	// non-zero positive scale converts integer to fixed-point decimal
	method(#primitive) scale: ndigits.
}

class(#limited) SmallInteger(Integer)
{
	method(#primitive) asCharacter.
	method(#primitive) asError.
}

class(#limited,#immutable,#liword) LargeInteger(Integer)
{
}

class(#limited,#immutable,#liword) LargePositiveInteger(LargeInteger)
{
	method abs { ^self }
	method sign { ^1 }
}

class(#limited,#immutable,#liword) LargeNegativeInteger(LargeInteger)
{
	method abs { ^self negated }
	method sign { ^-1 }
}

class(#limited,#immutable) FixedPointDecimal(Number)
{
	var value.
	var(#get) scale.

	method(#primitive) scale: ndigits.
}

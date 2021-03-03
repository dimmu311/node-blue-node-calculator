# node-blue-node-calculator
a node to do simple math operations

This is a node to do simple math operations. It can handle up to 4 Integer or Float Variables. If you need any constant values e.g. pi or something you can enter this in the formula.

To use the calculator just enter your formula in the corresponding parameter field.
You can enter `*/+-` and also `()` and nested `(())`. The calculator follows the math rules `()` before `*/` before `+-`.

At the place where you want 1 of the 4 Variables write `$` followed by the Input nr. E.g. `$2` to get the value from the variable connected to input 2

See this examples:

* Add Input 0 and Input 1: `$0+$1`
* Subtract Input 0 from Input 3: `$3-$0`
* Add Input 0 to Input 3 then Multiple with Input 2: `($0+$3)*$2`
* Device Input 0 by Constant Value 2: `$0/2`
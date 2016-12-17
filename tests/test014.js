// Variable creation and scope from http://en.wikipedia.org/wiki/JavaScript_syntax
//Adapted to 'AsyncScript' variable declaration semantics.

var x = 0; // A global variable
var y = 'Hello!'; // Another global variable
var z = 0; // yet another global variable
var twenty; //All globals must be declared

function f(){
  var z = 'foxes'; // A local variable
  twenty = 20; // Write to global variable.
  return x; // We can use x here because it is global
}
// The value of z is no longer available


// testing
var blah = f();
result = blah==0 && z!='foxes' && twenty==20;

// Array length test

var myArray = [ 1, 2, 3, 4, 5 ];
var myArray2 = [ 1, 2, 3, 4, 5 ];
myArray2[8] = 42;

assert (myArray.length == 5, "'myArray' length: " + myArray.length);
assert (myArray2.length == 9, "'myArray2' length: " + myArray2.length);

result = 1;

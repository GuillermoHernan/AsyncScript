// built-in functions

var foo = "foo bar stuff";
var r = Math.rand();

var parsed = parseInt("42");

var aStr = "ABCD";
var aChar = aStr.charAt(0);

var obj1 = new Object();
obj1.food = "cake";
obj1.desert = "pie";

var obj2 = obj1;
//obj2 = obj1.clone();
//obj2.food = "kittens";

result = foo.length==13 && foo.indexOf("bar")==4 && foo.substring(8,13)=="stuff" && parsed==42 && 
         Integer.valueOf(aChar)==65 && obj1.food=="cake" && obj2.desert=="pie";

result = foo.length==13;

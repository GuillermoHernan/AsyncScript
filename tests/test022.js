/* Javascript eval */

var mystructure = { a:39, b:3, addStuff : function(c,d) { return c+d; } };

var mystring = JSON.stringify(mystructure); 

var mynewstructure_a = eval(mystring);
var mynewstructure_b = eval("{ a:139, b:33, addStuff : function(c,d) { return c+d; } }");

result = mynewstructure_b.addStuff(mynewstructure_a.a, mynewstructure_a.b) == 42;

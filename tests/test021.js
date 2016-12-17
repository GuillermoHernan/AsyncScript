/* Javascript eval */

var myfoo = eval("{ foo: 42 }");

result = eval("4*10+2")==42 && myfoo.foo==42;

//eval shall not alter calling global scope
var myfoo2 = eval("var myfoo; myfoo = {x:'test'}");
assert (myfoo.foo == 42, "myfoo should not be altered");
assert (myfoo2.x == "test", "myfoo2.x == \"test\"");

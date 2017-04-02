// references with functions

var a = 42;
var b = [];
b[0] = 43;

function foo(myarray) {
  myarray[0]++;
}

function bar(myvalue) {
  myvalue++;
}

foo(b);
bar(a);

assert (a==42, "'a' value: " + a);
assert (b[0]==42, "'b[0]' value: " + b[0]);

result = 1;

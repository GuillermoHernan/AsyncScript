
function Foo (x,y)
{
    this.x = x;
    this.y = y;
}

Foo.prototype.value = function() { return this.x + this.y; }

var a = new Foo(1,2);
var b = new Foo(2,3); 

var result1 = a.value();
var result2 = b.value();
result = result1==3 && result2==5;

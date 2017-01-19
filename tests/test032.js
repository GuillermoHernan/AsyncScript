class Foo (x,y)
{
    function value ()
    {
        return this.x + this.y;
    }
}

//function Foo (x,y)
//{
//    this.x = x;
//    this.y = y;
//}

//Foo.prototype.value = function() { return this.x + this.y; }

var a = Foo(1,2);
var b = Foo(2,3); 

var result1 = a.value();
var result2 = b.value();
result = result1==3 && result2==5;

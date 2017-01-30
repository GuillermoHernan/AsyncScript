// 'for (... in ...)' Test

//const a = [4];
const a = [4,9,11,3];
var i = 0;

enableCallLog();

for (x in a)
{
    assert (x == a[i], "x=" + x + " i=" + i);
    ++i;
}

assert (i == a.length, "'i' value: " + i);

class list (h,t)
{
    function head() {return this.h;}
    function tail() {return this.t;}
}

function reverse (sequence)
{
    //TODO: This part of the test still does not work, because 'head' 
    //and 'tail' functions are not exported for all values.
//    if (sequence.tail() == null)
//        return sequence;
    
    var result = null;
    
    for (x in sequence)
        result = list(x, result);
    
    return result;
}

function toList(sequence)
{
    reverse (reverse(sequence));
}

const b = toList(a);
i = 0;

for (x in b)
{
    assert (x == a[i], "List compare. x=" + x + " i=" + i);
    ++i;
}

result = 1;
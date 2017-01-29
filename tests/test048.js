// Just tests that call log can be enabled

function myMultiply (a, b)
{
    return a*b;
}

function myPower (base, exp)
{
    var acc = 1;
    var i;
    
    for (i=0; i < exp; ++i)
        acc = myMultiply (acc, base);
    
    return acc;
}

function test ()
{
    enableCallLog();
    return myPower (2,8);
}

const a = 2 ** 8;
const b = 2*2*2*2*2*2*2*2;
const c = test();

assert (a==b, "a=" + a + " b=" + b);
assert (a==c, "a=" + a + " c=" + c);

result = 1;
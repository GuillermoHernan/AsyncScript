// Polynomic function: Test 'call', 'indexedRead', and 'indexedWrite' methods override.

class PolynomicFunction(coefficients)
{
    function call(t)
    {
        var i = 0;
        var result = 0;
        
        for (c in this.coefficients)
        {
            result += c * t**i;
            ++i;
        }
        
        return result;
    }
}

const f = PolynomicFunction ([8, -2, 1]);

assert (f(0) == 8, "f(0)=" + f(0));
assert (f(-2) == 16, "f(-2)=" + f(-2));
assert (f(1) == 7, "f(1)=" + f(1));
assert (f(4.5) == 19.25, "f(4.5)=" + f(4.5));

result = 1

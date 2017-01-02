// Global scope sharing test

actor Echo()
{
    input ping ()
    {
        this.pong(global1 + 3);
    }
    
    output pong (value);
}

actor EchoTest()
{
    var echoActor = Echo();
    
    result <- this.echoActor.pong;
    
    input begin()
    {
        this.echoActor.ping();
    }
    
    input result(value)
    {
        const c = global1 + 3;
        assert (15 == c, "Incorrect global1: " + global1);
        assert (value == c, "Expected value check");
    }
}

var global1 = 12;
var a = EchoTest();
var global1 = 25;

a.begin();

result = true;

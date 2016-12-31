// First actor system test

actor Echo()
{
    input ping (text)
    {
        this.pong("pong: " + text);
    }
    
    output pong (msg);
}

actor EchoTest()
{
    var echoActor = Echo();
    
    result <- this.echoActor.pong;
    
    input begin()
    {
        printLn ("Begin!");
        echoActor.ping("first actor test!");
    }
    
    input result(text)
    {
        assert (text == "pong: first actor test!", "Matching message text");
        result = true;
    }
}

printLn ("\nhello");
var a = EchoTest();

a.begin();
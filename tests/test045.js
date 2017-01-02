// First actor system test

actor Echo()
{
    input ping (text)
    {
        this.pong("Echo: " + text);
    }
    
    output pong (msg);
}

actor EchoTest()
{
    var echoActor = Echo();
    
    result <- this.echoActor.pong;
    
    input begin()
    {
        this.echoActor.ping("first actor test!");
    }
    
    input result(text)
    {
        assert (text == "Echo: first actor test!", "Matching message text");
    }
}

var a = EchoTest();

a.begin();

result = true;
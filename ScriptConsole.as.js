/*
 * Author: ghernan
 * 
 * Script console for AsyncScript.
 *
 * The first ever AsyncScript source file!
 * In fact, it was written before AsyncScript had a working implementation.
 * 
 * Created on December 18, 2016, 13:33 PM
 */

actor ScriptConsole
{
    const reader = spawn IO.StdinLineReader;
    
    lineIn <- reader.lineOut; 
    
    input lineIn (cmd)
    {
        const result = safeEval (line);
        
        //TODO: Missing global scope management!
        
        if (result.ok)
            Console.log ("> " + result.value);
        else
            Console.error ("! " + result.error);
    }
    
    input childStopped(actor, result, error)
    {
        if (actor === this.lineIn)
            this.stop(result, error);
        else if (error == null)
            Console.log ("> Actor '" + actor.name + "' finished: " + result);
        else
            Console.error ("! Actor '" + actor.name + "' failed: " + result);
    }
}

spawn ScriptConsole;
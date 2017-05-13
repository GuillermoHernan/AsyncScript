class Person(name) {
  function kill() 
  { 
      this.name += " is dead"; 
  };
}

var a = Person("Kenny");
a.kill();

assert (a.name == "Kenny is dead", "a.name=" + a.name);
result = 1;


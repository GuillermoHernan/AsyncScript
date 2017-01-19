class Person(name) {
  function kill() 
  { 
      this.name += " is dead"; 
  };
}

var a = Person("Kenny");
a.kill();
result = a.name == "Kenny is dead";


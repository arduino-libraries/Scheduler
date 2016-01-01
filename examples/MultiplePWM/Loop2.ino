
// Task no.2

void loop2 () {
 analogWrite(led1, counter1);
 counter1 += 50;

  // When multiple tasks are running, 'delay' passes control to
  // other tasks while waiting and guarantees they get executed
  // It is not necessary to call yield() when using delay() 
  delay(1000);
}

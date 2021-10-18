# Mouse library

## Methods

### `Scheduler.startLoop()`

Adds a function to the scheduler that will run concurrently with `loop()`.

#### Syntax 

```
Scheduler.startLoop(loopName);
```

#### Parameters

* `loopName`: the named function to run.

#### Returns

None.

#### Example

```
#include <Scheduler.h>

int counter = 0;
int counter1 = 0;

void setup() {
  Scheduler.startLoop(loop1);
}

void loop () {
 analogWrite(9, counter);
 counter++;

 if (counter > 255){
  counter = 0;
 }

 delay(33);
}

void loop1 () {
 analogWrite(10, counter1);
 counter1 = counter1 + 5;

 if (counter1 > 255) {
  counter1 = 0;
 }

 delay(10);
 yield();
}
```

#### See also

* [yield()](#yield)

### `yield()`

Passes control to other tasks when called. Ideally `yield()` should be used in functions that will take awhile to complete.

#### Syntax 

```
yield();
```

#### Parameters

None.

#### Returns

None.

#### Example

```
#include <Scheduler.h>

int counter = 0;
int counter1 = 0;

void setup() {
  Serial.begin(9600);
  Scheduler.startLoop(loop1);
}

void loop () {
 analogWrite(9, counter);
 counter++;

 if (counter > 255){
  counter = 0;
 }

 delay(33);
}

void loop1 () {
 if (Serial.available()) {
    char c = Serial.read();

    if (c=='0') {
      digitalWrite(2, LOW);
      Serial.println("Led turned OFF!");
    }

    if (c=='1') {
      digitalWrite(2, HIGH);
      Serial.println("Led turned ON!");
    }
  }

 yield();
}
```

#### See also

* [Scheduler.startLoop()](#schedulerstartloop)
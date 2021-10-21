# Scheduler library

The Scheduler library enables an Arduino based on SAM and SAMD architectures (i.e Zero, MKRZero, MKR1000, Due boards) to run multiple functions at the same time. This allows tasks to happen without interrupting each other. This is a cooperative scheduler in that the CPU switches from one task to another. The library includes methods for passing control between tasks.

To use this library:

```
#include <Scheduler.h>
```

## Notes and warnings

The Scheduler library and associated functions are experimental. While it is not likely the API will change in future releases, it is still under development.

## Examples

* [MultipleBlinks](https://www.arduino.cc/en/Tutorial/MultipleBlinks): Blink multiple LEDs in their own loops.
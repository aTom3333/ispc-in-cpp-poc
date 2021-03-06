# Proof of concept/demo SPMD code ala ISPC in C++
This repo is a demo to showcase how operator overloading and clever macros can be used (or rather abused)
to let the programmer write code similar to ISPC code that bears the same semantic in standard C++.

Familiarity with ISPC is assumed in the following explanations.
If you are not familiar with it or want more context, go read about ISPC before:
[https://ispc.github.io/index.html](https://ispc.github.io/index.html).

Note: this demo only focuses on getting a syntax as close as possible as ISPC with identical semantics
without any concern over performance. This is not to be used
in an actual application as it would probably be slower than a simple scalar version.

## Equivalence to ISPC constructs

### Variables

`varying` variables in ISPC are replaced by the use of the `iic::varying` template.
Uniform variables are just normal C++ variables but the `iic::uniform` template
is available for consistency. Note that ISPC variables without `varying`/`uniform`
qualifier are `varying` by default ; in C++ they will be `uniform` by default.

ISPC code:
```ispc
varying float a = 0;
uniform int b = 4;
uniform int c = 5 + b;
float d = c * 1.5f;
```
Equivalent C++ code:
```cpp
iic::varying<float> a = 0;
iic::uniform<int> b = 4;
int c = 5 + b;
iic::varying<float> d = c * 1.5f;
```

Most operators are overloaded to make varying variables
work nicely. Like in ISPC, uniform variables are implicitly convertible
to varying variables (values are broadcast), but not the other way around.

Like in ISPC, there is an implicit mask denoting which lanes are active
that is being passed around without manual handling.
Almost every operation on varying variables uses this mask to act only on active lanes.
The mask can be modified by some control flow structure, similar to ISPC.

### `if` statement

First is the `if` statement. Due to having multiple lanes working at once,
both branches of the same statement can be taken by distinct lanes. The following ISPC code:
```ispc
if(number % 2 == 0)
    number /= 2;
else
    number = number * 3 + 1;
```
is semantically equivalent to the following code (where `current_mask` represents
the mask that is used to control which lanes are active):
```ispc
{
    varying bool old_mask = current_mask;
    varying bool condition = number % 2 == 0;
    current_mask = old_mask && condition;
    number /= 2; // if body
    current_mask = old_mask && !condition;
    number = number * 3 + 1; // else body
    current_mask = old_mask;
}
```
With this demo, the way you write that is very similar to the ISPC code,
just replace the usual `if` by the custom `iic_if`:
```cpp
iic_if(number % 2 == 0)
    number /= 2;
else
    number = number * 3 + 1;
```

### `while` statement

Similarly, the `while` statement is available and is spelt `iic_while`.
Its semantic is to continue the loop while at least one of the lane is active,
eventually disabling lanes every time the condition is evaluated and finally restoring the initial mask
once the loop is over.

ISPC code:
```ispc
while(number != 1)
{
    // do stuff
}
``` 
Equivalent C++ code:
```cpp
iic_while(number != 1)
{
    // do stuff
}
```

### `for` statement

Note: the `for` loop is not available for now.

### Control flow

Next come the ISPC specific control flow structure with the
first one being the `foreach` loop that can be used to iterate
over uniform data with varying variables.
The provided `iic_foreach` can only iterate over one variable
and with a syntax a bit different from the native ISPC one.

ISPC code:
```
void sum(const float* uniform a, const float* uniform b, float* uniform c, uniform int n)
{
    foreach(i = 0...n)
    {
        c[i] = a[i] + b[i];
    }
}
```
Equivalent C++ code:
```cpp
void sum(const float* a, const float* b, float* c, int n)
{
    iic_foreach(i : iic::range(0, n))
    {
        c[i] = a[i] + b[i];
    }
}
```

As of now, two other ISPC control flow structures are available: `unmasked` and `foreach_active`.

ISPC code:
```ispc
unmasked
{
    // do stuff while mask is all true
}

foreach_active(index)
{
    // index is an uniform variable which takes the 
    // values of the index of every active lane one after another
}
```
Equivalent C++ code:
```cpp
iic_unmasked
{
    // do stuff while mask is all true
}

iic_foreach_active(index)
{
    // index is uniform variable which takes the 
    // values of the index of every active lane one after another
}
```


## How it works

The current mask is kept in a thread local variable, so it can always be accessible
without needing to pass around everywhere.
The overloaded operators of the `iic::varying` class are accessing the mask
to only apply their effect on active lanes.
The implementation of the `iic::varying` template can be found in the file `include/varying.hpp`.

The custom control flow structure are implemented as macro, following the approach described
in [https://www.chiark.greenend.org.uk/~sgtatham/mp/](https://www.chiark.greenend.org.uk/~sgtatham/mp/).
In fact the reading of this article is what motivated this quickly put together demo.
The custom control flow structure are responsible to change and restore the current mask
in a way to provide the desired semantics.
The implementation can be found in the file [`include/control_flow.hpp`](./include/control_flow.hpp)

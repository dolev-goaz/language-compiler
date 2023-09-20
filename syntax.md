# Language Name Here

### Data Types
```
int_8   // one byte
int_16  // two bytes
int_32  // four bytes
int_64  // eight bytes

char    // int_8 alias
```
### Variable Declaration
```
struct my_struct_name = {
    int_8 foo;
    int_64 bar;
    int_16 baz;
};

int_8 x = 5;
my_struct_name y;
```
### Scopes
```
{
    // ...
}
```

### Conditional Flow Control

```
if (numeral_condition) {
    // ...
}
while (numeral_condition) {
    // ...
}
```

### Function
Fucntions by default return a 64bit number.
```
func function_name(int_8 first, int_32 seconds, ...) {

}
exit(exit_code);
```

### Memory Access
```
int_8 data = mem(memory_address);
```
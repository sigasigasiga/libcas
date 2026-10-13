# Design

## Error handling

### Exceptions

Exceptions are not forbidden but at the same time their usage is limited.

There's a rule of thumb: if you think that the best way to handle an error on the user side is to terminate the program, then throw an exception.
Otherwise, use `cas::result::result`.

### Constructors

Same rules apply to the error handling in constructors. If some field has to be initialized by a function which may fail, but the user might want to handle the error, then the constructor should not initialize the field at all, and the value of the field should rather be passed to it.

Example:
```cpp
// BAD
struct foo {
    foo(char const *path) {
        h = fopen(path, "r");
        if (!h) {
            throw "i'm a bad constructor";
        }
    }

    FILE *h;
};

// GOOD
struct foo {
    foo(FILE &h) : h(&h) {}

    FILE *h;
};

result<foo> make_foo(char const *path) {
    auto h = fopen(path, "r");
    if (h) {
        return foo(h);
    } else {
        return std::unexpected(std::runtime_error("..."));
    }
}
```

### Preconditions

Generally we should `assert` on preconditions. If they are not met, then it is allowed for the behavior to be undefined.

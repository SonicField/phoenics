Please design a system which sits between the pre-processor and the main C compiler in a standard C compiler chain ( we can start with clang but it will need to work with gcc eventually).

This processor will parse C and output C.
It will be C11 complient.

However it will make tagged unions (sum types) a first class type.
- descr -> will be a new key word which will mark a distriminated uniom.
- The new type will compile down to a standard C tagged union approach ( struct with a tag and an union instide).

The key property will be that this new 'descr' type will be compile time enforced.
It will allow proper switch/case dispatch with compiler enforced descrimination.
Other use cases for descrimination will be supported.
The code itself will initailly be written in C.

Write this implementation here in this directory.
The approach will be pure TDD - create integration and unit tests and the framworks around them then code.


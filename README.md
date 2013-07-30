Bike stuff
====================

Different stuff. Currently consists of one library — s11n.

s11n (unstable)
---------------------
C++ serialization library with Boost.Serialization-like interface with such improvements:
- Header-only. 2 files and less 1k lines of ubertemplatesgodkillkittens
- Decreased compiler error lines per one mistake (It's more useful to read 50 lines of errors than 100, yep?)

Common features:
- Non-default constructors support
- Serialization of inherited classes
- Serialization of shared classes

[Read how to use s11n](docs/using-s11n.md)

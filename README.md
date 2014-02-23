Bike stuff
====================

Different stuff. Currently consists of one library — s11n (serialization).

s11n (unstable)
---------------------
C++ serialization library with Boost.Serialization-like interface with such improvements:
- Header-only. 2 files and less 2k lines of ubertemplatesgodkillskittenscode per one serializer format. Plus one file for STL support.
- Serialization of objects, which code can't be changed (out of class serialization with few limitations)
- Decreased compiler error lines per one mistake (it's more useful to read 50 lines of errors than 100, yep?)

Other features:
- Non-default constructors support
- Serialization of inherited objects
- Serialization of shared objects
- Support for optional data with default values
- C++03 compatible, C++11 ready

Has formats:
- XML
- Binary

[Read how to use s11n](docs/using-s11n.md)

Acknowledgements
====================
This library uses pugixml library (http://pugixml.org).
pugixml is Copyright (C) 2006-2012 Arseny Kapoulkine.

This library uses Google Test library (https://code.google.com/p/googletest/).
Google Test is Copyright 2005, Google Inc.

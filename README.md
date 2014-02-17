freddy
======

Thin C++11 STL wrapper for reading/writing JSON


## License

This library is free software, released under the zlib license.


## Brief

From json.org:

  "JSON is built on two structures:
  
  - A collection of name/value pairs. In various languages, 
  this is realized as an object ...
  - An ordered list of values. In most languages, this is 
  realized as an array ...
  
  These are universal data structures. Virtually all modern 
  programming languages support them in one form or another."

It seems many JSON libraries overcomplicate things by re-implementing
these standard data types, causing them to be bloated and infectuous.

By contrast, this library aims to be non-intrusive, wrapping transparently 
around these data structures as they already exist in the Standard Library.

In fact, these components are just aliases for their std counterparts:

- json::string => std::string
- json::object => std::map&lt;json::string, json::value&gt;
- json::array => std::vector&lt;json::value&gt;

json::value is the special type that can hold any of the following JSON types:

  - array
  - bool
  - null
  - number
  - object
  - string

std::shared_ptr is used internally for sane memory management.

Parsing works on streams as well as strings.

To make your class act as a json object, array or string, implement json::objectlike,
json::arraylike or json::stringlike.

Currently, the library is header-only, but this is likely to change soon.


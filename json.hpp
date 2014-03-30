#ifndef JSON_H
#define JSON_H
/*

freddy

A thin C++11 STL wrapper for reading/writing JSON


Copyright (c) 2014 Chris Bush

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.


*/
/*! @mainpage Freddy vs. JSON
 *
 * This documentation is generated by doxygen.
 *
 */


#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <iterator>


/**
 * @brief contains classes and functions for handling json
 **/
namespace json {


class value;


/**
 * @brief The string type is really a std::string
 **/
using string = std::string;


/**
 * @brief The json object is really a std::map<string, @ref value>
 **/
using object = std::map<string, value>;


/**
 * @brief The array is really a std::vector<@ref value>
 **/
using array = std::vector<value>;


/**
 * @brief The number is really a double
 * @warning This will probably change soon
 **/
using number = double;


std::ostream& operator<<(std::ostream&, object const&);
std::ostream& operator<<(std::ostream&, array const&);


/**
 * @brief Create an escaped string from an unescaped string.
 * @param value an unescaped string
 * @return an escaped string
 **/
inline
string escape(string const& value) {
  std::stringstream ss;
  for(auto& c : value){
    if(c == '"' // TODO: other escapes
    || c == '\\')
      ss << '\\';
    ss << c;
  }
  return ss.str();
}


/**
 * @brief Create an unescaped string from an escaped string.
 * @param value an escaped string
 * @return an unescaped string
 **/
inline
string unescape(string const& value) {
  std::stringstream ss;
  for(const char * c = value.data(); *c != '\0'; ++c) {
    if(*c == '\\')
      ++c;
    ss << *c;
  }
  return ss.str();
}


/**
 * @brief Interface of json string-like classes.
 **/
class stringlike {
  public:
    virtual ~stringlike(){}

    /**
     * @return an unescaped string representation of this instance.
     **/
    virtual string to_json_string() const =0;
};


/**
 * @brief Interface of json object-like classes.
 **/
class objectlike {
  public:
    virtual ~objectlike(){}

    /**
     * @return a representation of this instance as a json::object (i.e. std::map<string, json::value>).
     **/
    virtual object const& to_json_object() const =0;
};


/**
 * @brief Interface of json array-like classes.
 **/
class arraylike {
  public:
    virtual ~arraylike(){}

    /**
     * @return a representation of this instance as a json::array (i.e. std::vector<json::value>).
     **/
    virtual array const& to_json_array() const =0;
};


/**
 * @brief basic exception related to json operations
 **/
class exception : public std::exception {
  protected:
    std::string msg;

  public:
    /**
     * @brief construct an empty exception (not recommended, but meh)
     **/
    exception(){}

    /**
     * @brief construct a general json exception with a given message
     * @param msg detail about the exception
     **/
    exception(const char * msg)
      : msg(msg)
      {}

    /**
     * @brief construct a general json exception with a given message
     * @param msg detail about the exception
     **/
    exception(std::string const& msg)
      : msg(msg)
      {}

    virtual ~exception(){}

    /**
     * @return An message describing the exception.
     **/
    virtual const char * what() const noexcept {
      return msg.c_str();
    }

};


/**
 * @brief an exception for malformed json
 **/
class bad_json : public exception {
  public:
    /**
     * @brief construct a default bad json exception
     **/
    bad_json()
      : exception("bad json exception: malformed json")
      {}

    /**
     * @brief construct a bad json exception with a given message
     * @param msg detail about the exception
     **/
    bad_json(const char * msg)
      : exception(msg)
      {}

    /**
     * @brief construct a bad json exception with a given message
     * @param msg detail about the exception
     **/
    bad_json(std::string const& msg)
      : exception(msg)
      {}

    ~bad_json(){}

};


/**
 * @cond detail
 **/
namespace detail {

  static const char * const bool_branch[] { "false", "true" };
  static const char * const comma_branch[] { "", ", " };

  class value_t {
    public:
      value_t(){}
      virtual ~value_t(){}
      virtual string json() const =0;

      #define JSON_IS(type) \
        virtual bool is_##type() const { return false; } \
        virtual type const& get_##type() const { throw exception("invalid cast"); } \
        virtual type& get_##type() { throw exception("invalid cast"); }
      JSON_IS(array);
      JSON_IS(bool);
      JSON_IS(number);
      JSON_IS(string);
      JSON_IS(object);
      virtual bool is_null() const { return false; }
      #undef JSON_IS
  };

  class number_wrapper : public value_t {
    private:
      number *_value;

    public:
      number_wrapper(int v):_value(new number(v)){}
      number_wrapper(double v):_value(new number(v)){}
      ~number_wrapper(){
        delete _value;
      }

    public:
      bool is_number() const { return true; }
      number const& get_number() const { return *_value; }
      number& get_number() { return *_value; }
      string json() const {
        std::stringstream ss;
        ss << *_value;
        return ss.str();
      }

  };

  class string_wrapper : public value_t {
    private:
      string *_value;

    public:
      string_wrapper(const char* v):_value(new string(v)){}
      string_wrapper(string const& v):_value(new string(v)){}
      string_wrapper(stringlike const& v):_value(new string(v.to_json_string())){}

      ~string_wrapper(){
        delete _value;
      }
    
    public:
      bool is_string() const { return true; }
      string const& get_string() const { return *_value; }
      string& get_string() { return *_value; }
      string json() const {
        std::stringstream ss;
        ss << "\"" << escape(*_value) << "\"";
        return ss.str();
      }
  };

  class array_wrapper : public value_t {
    private:
      array *_value;

    public:
      array_wrapper(array const& v):_value(new array(v)){}
      array_wrapper(arraylike const& v):_value(new array(v.to_json_array())){}
      ~array_wrapper(){
        delete _value;
      }

    public:
      bool is_array() const { return true; }
      array const& get_array() const { return *_value; }
      array& get_array() { return *_value; }
      inline string json() const;
  };

  class object_wrapper : public value_t {
    private:
      object *_value;

    public:
      object_wrapper(object const& v):_value(new object(v)){}
      object_wrapper(objectlike const& v):_value(new object(v.to_json_object())){}
      ~object_wrapper(){
        delete _value;
      }

    public:
      bool is_object() const { return true; }
      inline object const& get_object() const;
      inline object& get_object();
      inline string json() const;
  };

  class bool_wrapper : public value_t {
    private:
      bool _value;

    public:
      bool_wrapper(bool v):_value(v){}
      ~bool_wrapper(){}

    public:
      bool const& get_bool() const { return _value; }
      bool& get_bool() { return _value; }
      bool is_bool() const { return true; }
      string json() const {
        return bool_branch[_value];
      }
  };

  class null_wrapper : public value_t {
    public:
      null_wrapper(){}
      ~null_wrapper(){}

    public:
      bool is_null() const { return true; }
      string json() const {
        return "null";
      }
  };

} // namespace detail
/**
 * @endcond detail
 **/


/**
 * @brief enumeration of the possible json value types
 **/
enum ValueType {
  JSON_ARRAY,
  JSON_BOOL,
  JSON_NULL,
  JSON_NUMBER,
  JSON_OBJECT,
  JSON_STRING,
};


/**
 * @brief Represents any json data type.
 **/
class value {
  protected:
    std::shared_ptr<detail::value_t> _value { new detail::null_wrapper() };
    ValueType _type { JSON_NULL };

  public:
    /**
     * Construct an empty value
     **/
    value(){}


    ////////////////////////////////////////// Array
    /**
     * Construct a value with an array (or std::vector<value>)
     * @param x the array to store in the value
     **/
    value(array const& x)
      : _value(new detail::array_wrapper(x))
      , _type(JSON_ARRAY)
      {}

    /**
     * Construct a value with an @ref arraylike object
     * @param x the array to store in the value
     **/
    value(arraylike const& x)
      : _value(new detail::array_wrapper(x))
      , _type(JSON_ARRAY)
      {}


    ////////////////////////////////////////// Bool
    /**
     * Construct a value with a bool
     * @param x the bool to store in the value
     **/
    value(bool x)
      : _value(new detail::bool_wrapper(x))
      , _type(JSON_BOOL)
      {}


    ////////////////////////////////////////// Number
    /**
     * Construct a value with a floating-point value
     * @param x The floating-point value to store as a number
     **/
    value(double x)
      : _value(new detail::number_wrapper(x))
      , _type(JSON_NUMBER)
      {}

    /**
     * Construct a value with an integral value
     * @param x The integral value to store as a number
     **/
    value(int x)
      : _value(new detail::number_wrapper(x))
      , _type(JSON_NUMBER)
      {}

    ////////////////////////////////////////// Object
    /**
     * Construct a value with a json object (or std::map<string, value>)
     * @param x The object to store in the value
     **/
    value(object const& x)
      : _value(new detail::object_wrapper(x))
      , _type(JSON_OBJECT)
      {}

    /**
     * Construct a value with an @ref objectlike object
     * @param x The object to store in the value
     **/
    value(objectlike const& x)
      : _value(new detail::object_wrapper(x))
      , _type(JSON_OBJECT)
      {}


    ////////////////////////////////////////// String
    /**
     * Construct a value with a string
     * @param x The string to store in the value
     **/
    value(string const& x)
      : _value(new detail::string_wrapper(x))
      , _type(JSON_STRING)
      {}

    /**
     * Construct a value with a null-terminated C-string
     * @param x The string to store in the value
     **/
    value(const char * const& x)
      : _value(new detail::string_wrapper(x))
      , _type(JSON_STRING)
      {}

    /**
     * Construct a value with a @ref stringlike object
     * @param x The string to store in the value
     **/
    value(stringlike const& x)
      : _value(new detail::string_wrapper(x))
      , _type(JSON_STRING)
      {}

  public:
    virtual ~value(){}

    /**
     * Copy constructor
     * @param v the value to copy
     **/
    value(value const& v)
      : _value(v._value)
      , _type(v._type)
      {}

    /**
     * Copy assignment operator
     * @param v the value to copy
     * @return a reference to this object
     **/
    inline value& operator=(value const& v) {
      _value = v._value;
      _type = v._type;
      return *this;
    }

  public:
    /**
     * @return the value represented in json string form
     **/
    inline string json() const {
      return _value->json();
    }

    /**
     * @return the json @ref ValueType of the value
     */
    inline ValueType type() const {
      return _type;
    }

    /**
     * @return true if the value represents an array
     **/
    inline bool is_array() const { return _value->is_array(); }
    array const& get_array() const { return _value->get_array(); }
    array& get_array() { return _value->get_array(); }
    explicit operator array&() { return _value->get_array(); } 

    /**
     * @return true if the value represents a bool
     **/
    inline bool is_bool() const { return _value->is_bool(); }
    bool const get_bool() const { return _value->get_bool(); }
    bool get_bool() { return _value->get_bool(); }

    /**
     * @return true if the value is null
     **/
    inline bool is_null() const { return _value->is_null(); }

    /**
     * @return true if the value represents a number
     **/
    inline bool is_number() const { return _value->is_number(); }
    number const& get_number() const { return _value->get_number(); }
    number& get_number() { return _value->get_number(); }

    /**
     * @return true if the value represents an object
     **/
    inline bool is_object() const { return _value->is_object(); }
    object const& get_object() const { return _value->get_object(); }
    object& get_object() { return _value->get_object(); }

    /**
     * @return true if the value represents a string
     **/
    inline bool is_string() const { return _value->is_string(); }
    string const& get_string() const { return _value->get_string(); }
    string& get_string() { return _value->get_string(); }

};


/**
 * @brief Put a json @ref value to an ostream object as json
 **/
inline 
std::ostream& operator<<(std::ostream& o, value const& v){
  o << v.json();
  return o;
}


/**
 * @brief Put a json @ref array to an output stream as json
 **/
inline
std::ostream& operator<<(std::ostream& o, array const& v){

  size_t size { v.size() }, count { 0 };  
  o << "[";
  for(auto const& i : v) {
    o << i.json();
    o << detail::comma_branch[++count < size];
  }
  o << "]";

  return o;
}


/**
 * @brief Put a json @ref object to an output stream as json
 */
inline
std::ostream& operator<<(std::ostream& o, object const& v){

  size_t size { v.size() }, count { 0 };

  o << "{";
  for(auto const& i : v) {
    o << "\"" << escape(i.first) << "\": ";
    o << i.second;
    o << detail::comma_branch[++count < size];
  }
  o << "}";

  return o;
}


/**
 * @cond detail
 **/
namespace detail {

inline bool is_whitespace(char c) {
  return c == ' '
      || c == '\r'
      || c == '\n'
      || c == '\t';
}

template<typename Iterator>
value parse(Iterator&, Iterator const&);

template<typename Iterator>
array parse_array(Iterator& cur, Iterator const& end) {
  array rv;
  bool accept = true, has_elements = false;
  do {
    if(is_whitespace(*cur)) {
      continue;
    } else if(*cur == ']' && (has_elements ^ accept)) {
      // !accept = strict (no trailing commas)
      return rv;
    } else if(accept) {
      rv.push_back(parse(cur, end));
      has_elements = true;
      accept = false;
    } else if(*cur == ',') {
      accept = true;
    } else {
      break;
    }
  } while(++cur != end);
  throw exception("bad array");
}

template<typename Iterator>
value parse_number(Iterator& cur, Iterator const& end) {
  std::stringstream io;
  do {
    io << *cur;
    auto next = cur + 1;
    if (!isdigit(*next) && *next != '-' && *next != '.' && *next != 'e' && *next != 'E' && *next != '+') {
      break;
    }
  } while (++cur != end);
  io.seekg(0);
  double d = 0;
  io >> d;
  return d;
}

template<typename Iterator>
string parse_string(Iterator& cur, Iterator const& end) {
  string rv;
  for(bool esc = false; cur != end; ++cur){
    if(*cur == '"' && !esc)
      return rv; 
    esc = *cur == '\\' && !esc;
    if (!esc)
      rv += *cur;
  }
  throw exception(string("bad string: ") + rv);
}

template<typename Iterator>
object parse_object(Iterator& cur, Iterator const& end) {
  object rv;
  string key;
  bool got_key = false, got_elements = false;
  do {
    if (*cur == '}' && !got_key) {
      return rv;
    } else if(is_whitespace(*cur)) {
      continue;
    } else if(*cur == '"') {
      key = parse_string(++cur, end);
      got_key = true;
    } else if(got_key && *cur == ':') {
      rv[key] = parse(++cur, end);
      got_key = false;
      got_elements = true;
    } else if(got_elements && !got_key && *cur == ',') {
      continue;
    } else {
      break;
    }
  } while(++cur != end);
  throw exception("bad object");
}

template<typename Iterator>
value parse(Iterator& cur, Iterator const& end) {
  do {
    switch(*cur){
      case '"': return parse_string(++cur, end);
      case '[': return parse_array(++cur, end);
      case '{': return parse_object(++cur, end);
      case '0' ... '9': case '-': return parse_number(cur, end);
      case 'n': if(*++cur=='u' && *++cur=='l' && *++cur=='l') return value(); throw exception("bad json: null");
      case 't': if(*++cur=='r' && *++cur=='u' && *++cur=='e') return true; throw exception(string("bad json: ") + *cur);
      case 'f': if(*++cur=='a' && *++cur=='l' && *++cur=='s' && *++cur=='e') return false; throw exception("bad json: false");
      case ' ': case '\r': case '\n': case '\t':
        ++cur;
        continue;
      default:
        throw exception("bad json");
    }
  } while(cur != end);
  throw exception("bad json");
}

template<typename T>
class peek {
  private:
    T v;

  public:
    peek(T v):v(v){}
    T operator*() const {
      return v;
    }
};

template<typename T>
class iterator {
  private:
    std::basic_istream<T>& istream;
    T cur_val;

  public:
    iterator(std::basic_istream<T>& is):istream(is), cur_val(is.get()){}

    iterator& operator++(){
      istream.get(cur_val);
      return *this;
    }

    T operator*() const {
      return cur_val;
    }

    peek<T> operator+(int i) const {
      // Assumes you are peeking
      return istream.peek();
    };

    bool operator!=(iterator const& other) const {
      // Assumes you are checking against the end iterator
      return istream.good();
    }
};


inline
string array_wrapper::json() const
{
  std::stringstream ss;
  ss << *_value;
  return ss.str();
}

inline
string object_wrapper::json() const
{
  std::stringstream ss;
  ss << *_value;
  return ss.str();
}

inline object const& object_wrapper::get_object() const {
  return *_value;
}

inline object& object_wrapper::get_object() {
  return *_value;
}

} // namespace detail
/**
 * @endcond detail
 **/


/**
 * @brief parse json into a @ref value
 * @param s the well-formed json to parse
 * @return the parsed json as a @ref value
 * @throw @ref exception if parsing failed
 **/
inline
value parse(string const& s){
  auto i = s.begin(), e = s.end();
  auto v = detail::parse(i, e);

  while(++i != e) {
    if(!detail::is_whitespace(*i))
      throw exception(string("garbage at end of input: ") + *i);
  }

  return v;
}


/**
 * @brief parse json from an input stream
 * @param istream the input stream from which to parse json
 * @return the parsed json as a @ref value
 * @throw @ref exception if parsing failed
 **/
template<typename T>
value parse(std::basic_istream<T>& istream){
  detail::iterator<T> i (istream);
  detail::iterator<T>& e = i;
  auto v = detail::parse(i, e);

  while(++i != e) {
    if(!detail::is_whitespace(*i))
      throw exception(string("garbage at end of input: ") + *i);
  }

  return v;
}


} // namespace json


#endif

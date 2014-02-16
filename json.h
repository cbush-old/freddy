#ifndef JSON_H
#define JSON_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <iterator>

namespace json {

class value;

using string = std::string;
using object = std::map<string, value>;
using array = std::vector<value>;
using number = double;

std::ostream& operator<<(std::ostream&, object const&);
std::ostream& operator<<(std::ostream&, array const&);

class stringlike {
  public:
    virtual ~stringlike(){}
    virtual string json() const { 
      return string("\"").append(to_json_string()).append("\"");
    }
    virtual string const& to_json_string() const =0;
};

class objectlike {
  public:
    virtual ~objectlike(){}
    virtual object const& to_json_object() const =0;
};

class arraylike {
  public:
    virtual ~arraylike(){}
    virtual array const& to_json_array() const =0;
};

struct exception : public std::exception {
  private:
    string msg;
  
  public:
    exception(const char * msg):msg(msg){}
    exception(std::string const& msg):msg(msg){}
    const char * what() const noexcept {
      return msg.c_str();
    }
};

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

namespace detail {

  static const char * const bool_branch[] { "false", "true" };
  static const char * const comma_branch[] { "", ", " };

  class null_t {};

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
      number _value;

    public:
      number_wrapper(number v):_value(v){}
      ~number_wrapper(){}

    public:
      bool is_number() const { return true; }
      number const& get_number() const { return _value; }
      number& get_number() { return _value; }
      string json() const {
        std::stringstream ss;
        ss << _value;
        return ss.str();
      }

  };

  class string_wrapper : public value_t {
    private:
      std::shared_ptr<string> _value;

    public:
      string_wrapper(const char* v):_value(new string(v)){}
      string_wrapper(string const& v):_value(new string(v)){}
      string_wrapper(stringlike const& v):_value(new string(v.to_json_string())){}
      ~string_wrapper(){}
    
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
      std::shared_ptr<array> _value;

    public:
      array_wrapper(array const& v):_value(new array(v)){}
      array_wrapper(arraylike const& v):_value(new array(v.to_json_array())){}
      ~array_wrapper(){}

    public:
      bool is_array() const { return true; }
      array const& get_array() const { return *_value; }
      array& get_array() { return *_value; }
      inline string json() const;
  };

  class object_wrapper : public value_t {
    private:
      std::shared_ptr<object> _value;

    public:
      object_wrapper(object const& v):_value(new object(v)){}
      object_wrapper(objectlike const& v):_value(new object(v.to_json_object())){}
      ~object_wrapper(){}

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

const detail::null_t null;

enum JSON_TYPE {
  JSON_ARRAY,
  JSON_BOOL,
  JSON_NULL,
  JSON_NUMBER,
  JSON_OBJECT,
  JSON_STRING,
};

class value {
  private:
    std::shared_ptr<detail::value_t> _value;
    JSON_TYPE _type;

  public:
    value(){}

    // Array
    value(array const& x):_value(new detail::array_wrapper(x)), _type(JSON_ARRAY){}
    value(arraylike const& x):_value(new detail::array_wrapper(x)), _type(JSON_ARRAY){}

    // Bool
    value(bool x):_value(new detail::bool_wrapper(x)), _type(JSON_BOOL){}

    // Null
    value(detail::null_t const& null):_value(new detail::null_wrapper()), _type(JSON_NULL){}

    // Number
    value(double x):_value(new detail::number_wrapper(x)), _type(JSON_NUMBER){}
    value(int x):_value(new detail::number_wrapper(x)), _type(JSON_NUMBER){}

    // Object
    value(object const& x):_value(new detail::object_wrapper(x)), _type(JSON_OBJECT){}
    value(objectlike const& x):_value(new detail::object_wrapper(x)), _type(JSON_OBJECT){}

    // String
    value(string const& x):_value(new detail::string_wrapper(x)), _type(JSON_STRING){}
    value(const char * const& x):_value(new detail::string_wrapper(x)), _type(JSON_STRING){}
    value(stringlike const& x):_value(new detail::string_wrapper(x)), _type(JSON_STRING){}

  public:
    ~value(){}
    value(value const& v):_value(v._value), _type(v._type){}
    inline value& operator=(value const& v) {
      _value = v._value;
      _type = v._type;
      return *this;
    }

  public:
    inline string json() const {
      return _value->json();
    }

    inline JSON_TYPE type() const { return _type; }
    #define JSON_IS_(type) \
      inline bool is_##type() const { return _value->is_##type(); } \
      type const& get_##type() const { return _value->get_##type(); } \
      type& get_##type() { return _value->get_##type(); }
    JSON_IS_(bool);
    JSON_IS_(array);
    JSON_IS_(object);
    JSON_IS_(number);
    JSON_IS_(string);
    #undef JSON_IS_

};

inline
std::ostream& operator<<(std::ostream& o, value const& v){
  o << v.json();
  return o;
}

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

inline
std::ostream& operator<<(std::ostream& o, object const& v){

  size_t size { v.size() }, count { 0 };

  o << "{";
  for(auto const& i : v) {
    o << "\"" << i.first << "\": ";
    o << i.second;
    o << detail::comma_branch[++count < size];
  }
  o << "}";

  return o;
}

namespace detail {

template<typename Iterator>
value parse(Iterator&, Iterator const&);

template<typename Iterator>
array parse_array(Iterator& cur, Iterator const& end) {
  array rv;
  bool accept = true, has_elements = false;
  do {
    if(*cur == ' ' || *cur == '\n' || *cur == '\t') {
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
      return unescape(rv);
    esc = !esc && *cur == '\\';
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
    } else if(*cur == ' ' || *cur == '\n' || *cur == '\t') {
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
      case 'n': if(*++cur=='u' && *++cur=='l' && *++cur=='l') return null; throw exception("bad json: null");
      case 't': if(*++cur=='r' && *++cur=='u' && *++cur=='e') return true; throw exception(string("bad json: ") + *cur);
      case 'f': if(*++cur=='a' && *++cur=='l' && *++cur=='s' && *++cur=='e') return false; throw exception("bad json: false");
      case ' ': case '\n': case '\t':
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

inline object const& object_wrapper::get_object() const { return *_value; }
inline object& object_wrapper::get_object() { return *_value; }

} // namespace detail


inline
value parse(string const& s){
  auto i = s.begin(), e = s.end();
  auto v = detail::parse(i, e);
  return v;
}

template<typename T>
value parse(std::basic_istream<T>& istream){
  detail::iterator<T> i (istream);
  detail::iterator<T>& e = i;
  return detail::parse(i, e);
}

} // namespace json


#endif

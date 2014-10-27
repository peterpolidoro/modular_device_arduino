// ----------------------------------------------------------------------------
// JsonPrinter.h
//
//
// Authors:
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#ifndef JSON_PRINTER_H
#define JSON_PRINTER_H
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Streaming.h"
#include "StandardCplusplus.h"
#include "vector"
#include "Constants.h"


namespace RemoteDevice
{
class JsonDepthTracker
{
public:
  JsonDepthTracker();
  JsonDepthTracker(boolean first_item, boolean inside_object);
  boolean first_item_;
  boolean inside_object_;
};

class JsonPrinter
{
public:
  JsonPrinter(Stream &stream);
  void setStream(Stream &stream);
  void startObject();
  void stopObject();
  void startArray();
  void stopArray();
  void setCompactPrint();
  void setPrettyPrint();
  void addKey(const char *key);
  template<typename T>
  void add(T value);
  template<typename T>
  void add(const char *key, T value)
  {
    addKey(key);
    add(value);
  }
  void addNull(const char *key)
  {
    addKey(key);
    addNull();
  }
  void addNull();
private:
  Stream *stream_ptr_;
  boolean pretty_print_;
  int indent_level_;
  std::vector<JsonDepthTracker> jdt_vector_;
  void indent();
  void stopItem();
  void stopArrayItem();
};
}
#endif
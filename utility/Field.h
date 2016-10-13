// ----------------------------------------------------------------------------
// Field.h
//
//
// Authors:
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#ifndef _MODULAR_SERVER_FIELD_H_
#define _MODULAR_SERVER_FIELD_H_
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "SavedVariable.h"
#include "JsonStream.h"
#include "Vector.h"
#include "Functor.h"
#include "ArduinoJson.h"

#include "Parameter.h"
#include "Constants.h"


namespace modular_server
{
class Field
{
public:
  Field();

  void setUnits(const ConstantString & name);
  void setRange(const long min, const long max);
  void setRange(const double min, const double max);
  template <size_t MAX_SIZE>
  void setSubset(const constants::SubsetMemberType (&subset)[MAX_SIZE], size_t size=MAX_SIZE);
  void setSubset(const constants::SubsetMemberType * subset, size_t max_size, size_t size);
  void addValueToSubset(const constants::SubsetMemberType & value);

  void attachPreSetValueCallback(const Functor0 & callback);
  void attachPreSetElementValueCallback(const Functor1<const size_t> & callback);
  void attachPostSetValueCallback(const Functor0 & callback);
  void attachPostSetElementValueCallback(const Functor1<const size_t> & callback);

  template <typename T>
  bool getValue(T & value);
  bool getValue(const ConstantString * & value);
  template <size_t N>
  bool getValue(long (&value)[N]);
  template <size_t N>
  bool getValue(double (&value)[N]);
  template <size_t N>
  bool getValue(bool (&value)[N]);
  template <typename T>
  bool getValue(T * value, const size_t N);
  template <typename T>
  bool getElementValue(const size_t element_index, T & value);
  template <typename T>
  bool getDefaultValue(T & value);
  bool getDefaultValue(const ConstantString * & value);
  template <size_t N>
  bool getDefaultValue(long (&value)[N]);
  template <size_t N>
  bool getDefaultValue(double (&value)[N]);
  template <size_t N>
  bool getDefaultValue(bool (&value)[N]);
  template <typename T>
  bool getDefaultValue(T * value, const size_t N);
  template <typename T>
  bool getDefaultElementValue(const size_t element_index, T & value);

  template <typename T>
  bool setElementValue(const size_t element_index, const T & value);
  template <typename T>
  bool setValue(const T & value);
  template <size_t N>
  bool setValue(const long (&value)[N]);
  template <size_t N>
  bool setValue(const double (&value)[N]);
  template <size_t N>
  bool setValue(const bool (&value)[N]);
  bool setValue(ArduinoJson::JsonArray & value);
  template <typename T>
  bool setValue(T * value, const size_t N);
  template <typename T>
  bool setAllElementValues(const T & value);
  template <typename T>
  bool setDefaultValue(T & default_value);
  void setValueToDefault();

  bool valueIsDefault();
  size_t getArrayLength();
  size_t getStringLength();

private:
  Parameter parameter_;
  SavedVariable saved_variable_;
  Functor0 pre_set_value_callback_;
  Functor1<const size_t> pre_set_element_value_callback_;
  Functor0 post_set_value_callback_;
  Functor1<const size_t> post_set_element_value_callback_;
  bool string_saved_as_char_array_;

  template <typename T>
  Field(const ConstantString & name,
        const T & default_value);
  template <size_t N>
  Field(const ConstantString & name,
        const long (&default_value)[N]);
  template <size_t N>
  Field(const ConstantString & name,
        const double (&default_value)[N]);
  template <size_t N>
  Field(const ConstantString & name,
        const bool (&default_value)[N]);
  template <size_t N>
  Field(const ConstantString & name,
        const char (&default_value)[N]);

  Parameter & parameter();
  const ConstantString & getName();
  const ConstantString &  getFirmwareName();
  bool firmwareNameInArray(ArduinoJson::JsonArray & firmware_name_array);
  JsonStream::JsonTypes getType();
  JsonStream::JsonTypes getArrayElementType();
  bool stringIsSavedAsCharArray();
  int findSubsetValueIndex(const long value);
  int findSubsetValueIndex(const char * value);
  Vector<const constants::SubsetMemberType> & getSubset();
  void preSetValueCallback();
  void preSetElementValueCallback(const size_t element_index);
  void postSetValueCallback();
  void postSetElementValueCallback(const size_t element_index);
  friend class Server;

};
}
#include "FieldDefinitions.h"

#endif

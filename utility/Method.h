// ----------------------------------------------------------------------------
// Method.h
//
//
// Authors:
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#ifndef _MODULAR_DEVICE_METHOD_H_
#define _MODULAR_DEVICE_METHOD_H_
#include "Streaming.h"
#include "Array.h"
#include "JsonGenerator.h"
#include "ConstantVariable.h"
#include "Parameter.h"
#include "Constants.h"


namespace ModularDevice
{
class Server;

typedef void (*Callback)(void);
typedef void (Server::*ReservedCallback)(void);

class Method
{
public:
  Method();
  Method(const ConstantString &name);
  void setName(const ConstantString &name);
  void attachCallback(Callback callback);
  void addParameter(Parameter &parameter);
private:
  const ConstantString *name_ptr_;
  Callback callback_;
  bool callback_attached_;
  bool compareName(const char *name_to_compare);
  bool compareName(const ConstantString &name_to_compare);
  const ConstantString* getNamePointer();
  void callback();
  ReservedCallback reserved_callback_;
  bool reserved_;
  void attachReservedCallback(ReservedCallback callback);
  bool isReserved();
  void reservedCallback(Server *server);
  Array<Parameter*,constants::METHOD_PARAMETER_COUNT_MAX> parameter_ptr_array_;
  int findParameterIndex(const ConstantString &parameter_name);
  int parameter_count_;
  friend class Server;
};
}
#endif

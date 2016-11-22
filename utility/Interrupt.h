// ----------------------------------------------------------------------------
// Interrupt.h
//
//
// Authors:
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#ifndef _MODULAR_SERVER_INTERRUPT_H_
#define _MODULAR_SERVER_INTERRUPT_H_
#include "Streaming.h"
#include "Array.h"
#include "ConstantVariable.h"

#include "NamedElement.h"
#include "Callback.h"
#include "Constants.h"


namespace modular_server
{

namespace interrupt
{
enum{MODE_SUBSET_LENGTH=5};
extern ConstantString mode_detached;
extern ConstantString mode_low;
extern ConstantString mode_change;
extern ConstantString mode_rising;
extern ConstantString mode_falling;
extern constants::SubsetMemberType mode_ptr_subset[MODE_SUBSET_LENGTH];
}

class Callback;

class Interrupt : private NamedElement
{
public:
  Interrupt();
  void enablePullup();
  void disablePullup();
  size_t getNumber();
  size_t getPin();
  Callback * getCallbackPtr();
  const ConstantString & getMode();
  bool getPullup();

private:
  size_t number_;
  size_t pin_;
  Callback * callback_ptr_;
  const ConstantString * mode_ptr_;
  bool pullup_;
  Interrupt(const ConstantString & name, const size_t pin);
  void setup(const ConstantString & name);
  void setPin(const size_t pin);
  void attach(const Callback & callback, const ConstantString & mode);
  void detach();
  friend class Server;
  friend class Callback;
};
}
#endif

// ----------------------------------------------------------------------------
// Server.cpp
//
//
// Authors:
// Peter Polidoro polidorop@janelia.hhmi.org
// ----------------------------------------------------------------------------
#include "Server.h"


using namespace ArduinoJson;

namespace RemoteDevice
{

FLASH_STRING(default_device_name,"");
FLASH_STRING(get_device_info_method_name,"getDeviceInfo");
FLASH_STRING(get_method_ids_method_name,"getMethodIds");
FLASH_STRING(get_response_codes_method_name,"getResponseCodes");
FLASH_STRING(help_method_name,"?");

Server::Server(Stream &stream)
{
  setRequestStream(stream);
  setName(default_device_name);
  model_number_ = 0;
  serial_number_ = 0;
  firmware_number_ = 0;

  Method get_device_info_method(get_device_info_method_name);
  get_device_info_method.attachReservedCallback(&Server::getDeviceInfoCallback);
  addMethod(get_device_info_method);

  Method get_method_ids_method(get_method_ids_method_name);
  get_method_ids_method.attachReservedCallback(&Server::getMethodIdsCallback);
  addMethod(get_method_ids_method);

  Method get_response_codes_method(get_response_codes_method_name);
  get_response_codes_method.attachReservedCallback(&Server::getResponseCodesCallback);
  addMethod(get_response_codes_method);

  Method help_method(help_method_name);
  help_method.attachReservedCallback(&Server::help);
  addMethod(help_method);
}

void Server::setRequestStream(Stream &stream)
{
  stream_ptr_ = &stream;
}

void Server::handleRequest()
{
  while (stream_ptr_->available() > 0)
  {
    int request_length = stream_ptr_->readBytesUntil(EOL,request_,STRING_LENGTH_REQUEST);
    if (request_length == 0)
    {
      continue;
    }
    request_[request_length] = '\0';
    response = Generator::JsonObject<JSON_OBJECT_SIZE_RESPONSE>();
    if (request_[0] == START_CHAR_JSON_OBJECT)
    {
      request_type_ = OBJECT_REQUEST;
      Parser::JsonObject json_object = parser_.parse(request_);
      if (json_object.success())
      {
        processObjectRequest(json_object);
      }
      else
      {
        response["status"] = ERROR;
        response["error_message"] = "Parsing JSON object request failed! Could be invalid JSON or too many tokens.";
        response["received_request"] = request_;
      }
    }
    else
    {
      request_type_ = ARRAY_REQUEST;
      if (request_[0] != START_CHAR_JSON_ARRAY)
      {
        request_type_ = COMMAND_LINE_REQUEST;
        String request_string = String("[") + String(request_) + String("]");
        request_string.toCharArray(request_,STRING_LENGTH_REQUEST);
      }
      Parser::JsonArray json_array = parser_.parse(request_);
      if (json_array.success())
      {
        processArrayRequest(json_array);
      }
      else
      {
        response["status"] = ERROR;
        response["error_message"] = "Parsing JSON array request failed! Could be invalid JSON or too many tokens.";
        response["received_request"] = request_;
      }
      if (!response.containsKey("status"))
      {
        response["status"] = SUCCESS;
      }
      if (request_type_ == COMMAND_LINE_REQUEST)
      {
        int status = response["status"];
        switch (status)
        {
          case ERROR:
            response["status"] = "error";
            break;
          case SUCCESS:
            response["status"] = "success";
            break;
        }
        if (response.containsKey("method"))
        {
          // Make help response a little less cluttered
          if (strcmp(response["method"],"?") == 0)
          {
            response.remove("method");
            response.remove("status");
          }
        }
        response.prettyPrintTo(*stream_ptr_);
      }
      else
      {
        response.printTo(*stream_ptr_);
      }
      *stream_ptr_ << endl;
    }
  }
}

void Server::addMethod(Method method)
{
  char method_name[STRING_LENGTH_METHOD_NAME] = {0};
  _FLASH_STRING* method_name_ptr = method.getNamePointer();
  method_name_ptr->copy(method_name);
  if (String(method_name).length() > 0)
  {
    int method_index = getMethodIndexByName(method_name);
    if (method_index < 0)
    {
      method_vector_.push_back(method);
    }
    else
    {
      method_vector_[method_index] = Method(*method_name_ptr);
    }
  }
}

void Server::setName(_FLASH_STRING& name)
{
  name_ptr_ = &name;
}

void Server::setModelNumber(int model_number)
{
  model_number_ = model_number;
}

void Server::setFirmwareNumber(int firmware_number)
{
  firmware_number_ = firmware_number;
}

void Server::processObjectRequest(Parser::JsonObject &json_object)
{
}

void Server::processArrayRequest(Parser::JsonArray &json_array)
{
  char* method_string = json_array[0];
  int method_index = processMethodString(method_string);
  if (!(method_index < 0))
  {
    int array_elements_count = countJsonArrayElements(json_array);
    int parameter_count = array_elements_count - 1;
    if ((parameter_count == 1) && (String((char*)json_array[1]).equals("?")))
    {
      methodHelp(method_index);
    }
    else if ((parameter_count == 2) && (String((char*)json_array[2]).equals("?")))
    {
      int parameter_index = processParameterString(method_index, json_array[1]);
      parameterHelp(method_index, parameter_index);
    }
    else if (parameter_count != method_vector_[method_index].parameter_count_)
    {
      response["status"] = ERROR;
      String error_request = "Incorrect number of parameters. ";
      error_request += String(parameter_count) + String(" given. ");
      error_request += String(method_vector_[method_index].parameter_count_);
      error_request += String(" needed.");
      char error_str[STRING_LENGTH_ERROR];
      error_request.toCharArray(error_str,STRING_LENGTH_ERROR);
      response["error_message"] = error_str;
    }
    else
    {
      boolean parameters_ok = createParametersObject(method_index,json_array);
      if (parameters_ok && parameters.success())
      {
        executeMethod(method_index);
      }
      else if (parameters_ok)
      {
        response["status"] = ERROR;
        response["error_message"] = "Parsing JSON parameters string failed! Could be invalid JSON or too many tokens.";
      }
    }
  }
}

boolean Server::createParametersObject(int method_index, Parser::JsonArray &json_array)
{
  int parameter_index = 0;
  String parameters_string = "{";
  String parameter_template = "\"{key}\":{value}";
  String parameter_string;
  char parameter_name[PARAMETER_COUNT_MAX][STRING_LENGTH_PARAMETER_NAME] = {0};
  _FLASH_STRING* parameter_name_ptr;
  for (Parser::JsonArrayIterator it=json_array.begin(); it!=json_array.end(); ++it)
  {
    if (it!=json_array.begin())
    {
      if (checkParameter(method_index,parameter_index,*it))
      {
        if (parameter_index > 0)
        {
          parameter_string = String(",") + parameter_template;
        }
        else
        {
          parameter_string = parameter_template;
        }
        parameter_name_ptr = method_vector_[method_index].parameter_vector_[parameter_index].getNamePointer();
        parameter_name_ptr->copy(parameter_name[parameter_index]);
        parameter_string.replace("{key}",parameter_name[parameter_index]);
        char* value_char_array = (char*)*it;
        parameter_string.replace("{value}",value_char_array);
        parameters_string += parameter_string;
        parameter_index++;
      }
      else
      {
        return false;
      }
    }
  }
  parameters_string = parameters_string + "}";
  // Serial << "parameters_string: " << parameters_string << endl;
  char parameters_char_array[STRING_LENGTH_PARAMETERS];
  parameters_string.toCharArray(parameters_char_array,STRING_LENGTH_PARAMETERS);
  // Serial << "parameters_char_array: " << parameters_char_array << endl;
  parameters = parser_.parse(parameters_char_array);
  parameter_count_ = parameter_index;
  return true;
}

void Server::executeMethod(int method_index)
{
  if (method_vector_[method_index].isReserved())
  {
    method_vector_[method_index].reservedCallback(this);
  }
  else
  {
    method_vector_[method_index].callback();
  }
}

void Server::methodHelp(int method_index)
{
  method_help_array_ = Generator::JsonArray<PARAMETER_COUNT_MAX>();
  int parameter_index = 0;
  std::vector<Parameter>& parameter_vector = method_vector_[method_index].parameter_vector_;
  // char parameter_name[PARAMETER_COUNT_MAX][STRING_LENGTH_PARAMETER_NAME] = {0};
  _FLASH_STRING* parameter_name_ptr;
  for (std::vector<Parameter>::iterator it = parameter_vector.begin();
       it != parameter_vector.end();
       ++it)
  {
    if (parameter_index < PARAMETER_COUNT_MAX)
    {
      parameter_name_ptr = it->getNamePointer();
      parameter_name_ptr->copy(parameter_name_array_[parameter_index]);
      // Serial << "parameter_name_array_[parameter_index]: " << parameter_name_array_[parameter_index] << endl;
      // Serial << "parameter_name_ptr.length(): " << parameter_name_ptr->length() << endl;
      method_help_array_.add(parameter_name_array_[parameter_index]);
      parameter_index++;
    }
  }
  response["parameters"] = method_help_array_;
}

void Server::parameterHelp(int method_index, int parameter_index)
{
  char method_name[STRING_LENGTH_METHOD_NAME] = {0};
  _FLASH_STRING* method_name_ptr = method_vector_[method_index].getNamePointer();
  method_name_ptr->copy(method_name);

  parameter_help_object_ = Generator::JsonObject<JSON_OBJECT_SIZE_PARAMETER_HELP>();
  Parameter& parameter = method_vector_[method_index].parameter_vector_[parameter_index];
  char parameter_name[STRING_LENGTH_PARAMETER_NAME] = {0};
  _FLASH_STRING* parameter_name_ptr = parameter.getNamePointer();
  parameter_name_ptr->copy(parameter_name);
  char parameter_units[STRING_LENGTH_PARAMETER_UNITS] = {0};
  _FLASH_STRING* parameter_units_ptr = parameter.getUnitsPointer();
  parameter_units_ptr->copy(parameter_units);

  parameter_help_object_["name"] = parameter_name;
  parameter_help_object_["position"] = parameter_index;
  parameter_help_object_["method"] = method_name;
  if (strcmp(parameter_units,"") != 0)
  {
    parameter_help_object_["units"] = parameter_units;
  }
  ParameterType type = parameter.getType();
  switch (type)
  {
    case LONG_PARAMETER:
      parameter_help_object_["type"] = "long";
      if (parameter.rangeIsSet())
      {
        long min = parameter.getMin().l;
        long max = parameter.getMax().l;
        parameter_help_object_["min"] = min;
        parameter_help_object_["max"] = max;
      }
      break;
    case DOUBLE_PARAMETER:
      parameter_help_object_["type"] = "double";
      if (parameter.rangeIsSet())
      {
        double min = parameter.getMin().d;
        double max = parameter.getMax().d;
        parameter_help_object_["min"].set<DOUBLE_DIGITS>(min);
        parameter_help_object_["max"].set<DOUBLE_DIGITS>(max);
      }
      break;
    case STRING_PARAMETER:
      parameter_help_object_["type"] = "string";
      break;
  }
  response["parameter"] = parameter_help_object_;
}

boolean Server::checkParameter(int method_index, int parameter_index, Parser::JsonValue json_value)
{
  boolean parameter_ok = true;
  Parameter& parameter = method_vector_[method_index].parameter_vector_[parameter_index];
  ParameterType type = parameter.getType();
  String min_string = "";
  String max_string = "";
  switch (type)
  {
    case LONG_PARAMETER:
      if (parameter.rangeIsSet())
      {
        long value = (long)json_value;
        long min = parameter.getMin().l;
        long max = parameter.getMax().l;
        if ((value < min) || (value > max))
        {
          parameter_ok = false;
          min_string = String(min);
          max_string = String(max);
        }
      }
      break;
    case DOUBLE_PARAMETER:
      if (parameter.rangeIsSet())
      {
        double value = (double)json_value;
        double min = parameter.getMin().d;
        double max = parameter.getMax().d;
        if ((value < min) || (value > max))
        {
          parameter_ok = false;
          char temp_string[10];
          dtostrf(min,DOUBLE_DIGITS,DOUBLE_DIGITS,temp_string);
          min_string = String(temp_string);
          dtostrf(max,DOUBLE_DIGITS,DOUBLE_DIGITS,temp_string);
          max_string = String(temp_string);
        }
      }
      break;
    case STRING_PARAMETER:
      break;
  }
  if (!parameter_ok)
  {
    response["status"] = ERROR;
    String error = String("Parameter value out of range: ");
    error += min_string;
    error += String(" <= ");
    char parameter_name[STRING_LENGTH_PARAMETER_NAME] = {0};
    _FLASH_STRING* parameter_name_ptr = parameter.getNamePointer();
    parameter_name_ptr->copy(parameter_name);
    error += String(parameter_name);
    error += String(" <= ");
    error += max_string;
    char error_str[STRING_LENGTH_ERROR];
    error.toCharArray(error_str,STRING_LENGTH_ERROR);
    response["error_message"] = error_str;
  }
  return parameter_ok;
}

int Server::processMethodString(char *method_string)
{
  int method_index = -1;
  int method_id = String(method_string).toInt();
  if (String(method_string).compareTo("0") == 0)
  {
    method_index = 0;
    response["method_id"] = 0;
  }
  else if (method_id > 0)
  {
    method_index = method_id;
    response["method_id"] = method_id;
  }
  else
  {
    method_index = getMethodIndexByName(method_string);
    response["method"] = method_string;
  }
  if ((method_index < 0) || (method_index >= method_vector_.size()))
  {
    response["status"] = ERROR;
    response["error_message"] = "Unknown method.";
    method_index = -1;
  }
  return method_index;
}

int Server::getMethodIndexByName(char *method_name)
{
  int method_index = -1;
  for (std::vector<Method>::iterator it = method_vector_.begin();
       it != method_vector_.end();
       ++it)
  {
    if (it->compareName(method_name))
    {
      method_index = std::distance(method_vector_.begin(),it);
      break;
    }
  }
  return method_index;
}

int Server::processParameterString(int method_index, char *parameter_string)
{
  int parameter_index = -1;
  int parameter_id = String(parameter_string).toInt();
  if (String(parameter_string).compareTo("0") == 0)
  {
    parameter_index = 0;
  }
  else if (parameter_id > 0)
  {
    parameter_index = parameter_id;
  }
  else
  {
    parameter_index = getParameterIndexByName(method_index, parameter_string);
  }
  std::vector<Parameter>& parameter_vector = method_vector_[method_index].parameter_vector_;
  if ((parameter_index < 0) || (parameter_index >= parameter_vector.size()))
  {
    response["status"] = ERROR;
    response["error_message"] = "Unknown parameter.";
    parameter_index = -1;
  }
  return parameter_index;
}

int Server::getParameterIndexByName(int method_index, char *parameter_name)
{
  int parameter_index = -1;
  std::vector<Parameter>& parameter_vector = method_vector_[method_index].parameter_vector_;
  for (std::vector<Parameter>::iterator it = parameter_vector.begin();
       it != parameter_vector.end();
       ++it)
  {
    if (it->compareName(parameter_name))
    {
      parameter_index = std::distance(parameter_vector.begin(),it);
      break;
    }
  }
  return parameter_index;
}

int Server::countJsonArrayElements(Parser::JsonArray &json_array)
{
  int elements_count = 0;
  for (Parser::JsonArrayIterator i=json_array.begin(); i!=json_array.end(); ++i)
  {
    elements_count++;
  }
  return elements_count;
}

void Server::getDeviceInfoCallback()
{
  char device_name[STRING_LENGTH_DEVICE_NAME] = {0};
  name_ptr_->copy(device_name);
  response["name"] = device_name;
  response["model_number"] = model_number_;
  response["serial_number"] = serial_number_;
  response["firmware_number"] = firmware_number_;
}

void Server::getMethodIdsCallback()
{
  char method_name[METHOD_COUNT_MAX][STRING_LENGTH_METHOD_NAME] = {0};
  _FLASH_STRING* method_name_ptr;
  for (std::vector<Method>::iterator it = method_vector_.begin();
       it != method_vector_.end();
       ++it)
  {
    int method_index = std::distance(method_vector_.begin(),it);
    if (!method_vector_[method_index].isReserved())
    {
      method_name_ptr = it->getNamePointer();
      method_name_ptr->copy(method_name[method_index]);
      int method_id = method_index;
      response[method_name[method_index]] = method_id;
    }
  }
}

void Server::getResponseCodesCallback()
{
  response["response_success"] = SUCCESS;
  response["response_error"] = ERROR;
}

void Server::help()
{
  Generator::JsonObject<JSON_OBJECT_SIZE_DEVICE_INFO> device_info;
  char device_name[STRING_LENGTH_DEVICE_NAME] = {0};
  name_ptr_->copy(device_name);
  device_info["name"] = device_name;
  device_info["model_number"] = model_number_;
  device_info["serial_number"] = serial_number_;
  device_info["firmware_number"] = firmware_number_;
  response["device_info"] = device_info;
  Generator::JsonArray<METHOD_COUNT_MAX> methods;
  char method_name[METHOD_COUNT_MAX][STRING_LENGTH_METHOD_NAME] = {0};
  _FLASH_STRING* method_name_ptr;
  for (std::vector<Method>::iterator it = method_vector_.begin();
       it != method_vector_.end();
       ++it)
  {
    int method_index = std::distance(method_vector_.begin(),it);
    if (!method_vector_[method_index].isReserved())
    {
      method_name_ptr = it->getNamePointer();
      method_name_ptr->copy(method_name[method_index]);
      methods.add(method_name[method_index]);
    }
  }
  response["methods"] = methods;
}
}

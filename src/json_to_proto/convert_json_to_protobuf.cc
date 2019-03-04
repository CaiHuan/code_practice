#include "convert_json_to_protobuf.h"

#include <fcntl.h> 
#include <iostream>
#include <io.h>
#include <stdio.h>
#include <map>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "convert_switches.h"

#include "json_to_protobuf_serializer.h"

namespace self {
ConvertJsonToProtobuf::ConvertJsonToProtobuf() {
}

ConvertJsonToProtobuf::~ConvertJsonToProtobuf() {
}

bool ConvertJsonToProtobuf::Convert(
  const base::CommandLine* command_line,
  std::string& error_message) {
  base::FilePath input_file_path = 
    command_line->GetSwitchValuePath(convert_switches::kInputFilePath);
  base::FilePath output_file_path = 
    command_line->GetSwitchValuePath(convert_switches::kOutputFilePath);
  if (!base::PathExists(input_file_path)) {
    error_message = "input_filepath does not exist!";
    return false;
  }

  std::unique_ptr<base::DictionaryValue> root_dict = 
    ParseInputJson(input_file_path, error_message);
  if (!root_dict) {
    error_message += "\nparse input_file json fail!";
    return false;
  }
  JsonToProtobufSerializer json_to_protobuf_serializer(output_file_path);
  json_to_protobuf_serializer.Serialize(*root_dict.get());
  
  return true;
}

std::unique_ptr<base::DictionaryValue>
ConvertJsonToProtobuf::ParseInputJson(
  const base::FilePath& input_file_path,
  std::string& error_message) {
  base::File::Info file_info;
  if (!base::GetFileInfo(input_file_path, &file_info)) {
    return nullptr;
  }
  std::unique_ptr<char[]> read_buffer(new char[file_info.size]);
  if (!base::ReadFile(input_file_path, read_buffer.get(), file_info.size)) {
    return nullptr;
  }
  int error_code = 0;
  int error_line = 0;
  int error_column = 0;
  std::string temp(read_buffer.get(), file_info.size);
  std::unique_ptr<base::Value> root = base::JSONReader::ReadAndReturnError(
    temp, 0, &error_code, &error_message, &error_line, &error_column);
  if (!root) {
    error_message += ("\n line: " + base::IntToString(error_line));
    error_message += ("\n column: " + base::IntToString(error_column));
    return false;
  }
  return base::DictionaryValue::From(std::move(root));
}
} //namespace self
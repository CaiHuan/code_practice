#include <iostream>
#include <string>
#include "base/command_line.h"
#include "convert_json_to_protobuf.h"
#include "convert_switches.h"

static const char kHelpContent[] = "\
format like this \n\
input-filepath=xxx.json output-filepath=xxx\n";
void PrintHelp() {
  ::printf("%s", kHelpContent);
}
int main() {
  // Initialize the CommandLine singleton from the environment
  base::CommandLine::Init(0, nullptr);
  const base::CommandLine* command_line =
    base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(convert_switches::kInputFilePath) ||
      !command_line->HasSwitch(convert_switches::kOutputFilePath)) {
      PrintHelp();
      return -1;
  }
  std::unique_ptr<self::ConvertJsonToProtobuf> convert_json_to_protobuf = 
    self::ConvertJsonToProtobuf::New();
  std::string error_message;
  if (!convert_json_to_protobuf->Convert(command_line, error_message)) {
    ::printf("error reason is: %s\n", error_message.c_str());
    //::system("pause");
  }
  return 0;
}
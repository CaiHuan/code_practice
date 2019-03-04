#ifndef CONVERT_JSON_TO_PROTOBUF_H_
#define CONVERT_JSON_TO_PROTOBUF_H_

#include <memory>
#include <string>
#include <vector>

namespace base {
class Value;
class CommandLine;
class DictionaryValue;
class FilePath;
class ListValue;
}

namespace self {

class ConvertJsonToProtobuf {
public:
  static inline std::unique_ptr<ConvertJsonToProtobuf> New() {
    return std::unique_ptr<ConvertJsonToProtobuf>(new ConvertJsonToProtobuf());
  }

  ~ConvertJsonToProtobuf();

  bool Convert(
    const base::CommandLine* command_line,
    std::string& error_message);
  
private:
  ConvertJsonToProtobuf();

  std::unique_ptr<base::DictionaryValue> ParseInputJson(
    const base::FilePath& input_file_path,
    std::string& error_message);
};
} // namespace slef
#endif // CONVERT_JSON_TO_PROTOBUF_H_
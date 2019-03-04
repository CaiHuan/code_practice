#ifndef JSON_TO_PROTOBUF_SERIALIZER_H_
#define JSON_TO_PROTOBUF_SERIALIZER_H_

#include "base/values.h"
#include "base/files/file_path.h"
#include "base/macros.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}

namespace google {
namespace protobuf {
class DynamicMessageFactory;
class FileDescriptor;
class Message;
class Descriptor;
} //namespace protobuf
} // google

namespace self {
class JsonToProtobufSerializer : public base::ValueSerializer {
public:
  JsonToProtobufSerializer() = delete;

  explicit JsonToProtobufSerializer(
    const base::FilePath& output_file_path);

  ~JsonToProtobufSerializer();

  virtual bool Serialize(const base::Value& root) override;

private:
  std::unique_ptr<google::protobuf::Message> CreateMessage(
    const base::DictionaryValue* root_dict,
    const google::protobuf::FileDescriptor* file_desc,
    google::protobuf::DynamicMessageFactory* dynamic_message_factory);

  void AssignDataRecurveDictionaryValue(
    const base::DictionaryValue* dict_value,
    const std::string& key_name,
    const google::protobuf::Descriptor* parent_desc,
    google::protobuf::Message* parent_message,
    const google::protobuf::FileDescriptor* file_desc,
    google::protobuf::DynamicMessageFactory* dynamic_message_factory);

  void AssignDataRecurveListValue(
    const base::ListValue* list_value,
    const std::string& key_name,
    const google::protobuf::Descriptor* parent_desc,
    google::protobuf::Message* parent_message,
    const google::protobuf::FileDescriptor* file_desc,
    google::protobuf::DynamicMessageFactory* dynamic_message_factory);

  void AssignDataJudgeType(
    const base::Value* value,
    const std::string& key_name,
    const google::protobuf::Descriptor* parent_desc,
    google::protobuf::Message* parent_message,
    const google::protobuf::FileDescriptor* file_desc,
    google::protobuf::DynamicMessageFactory* dynamic_message_factory);

  void AssignDataListValueLogical(
    const base::Value* value,
    const std::string& key_name,
    const google::protobuf::Descriptor* parent_desc,
    google::protobuf::Message* parent_message,
    const google::protobuf::FileDescriptor* file_desc,
    google::protobuf::DynamicMessageFactory* dynamic_message_factory);

  void AssignData(
    const base::Value* value,
    const std::string& key_name,
    const google::protobuf::Descriptor* parent_desc,
    google::protobuf::Message* parent_message);

  void AssignRepeatedData(
    const base::ListValue* value,
    const std::string& key_name,
    const google::protobuf::Descriptor* parent_desc,
    google::protobuf::Message* parent_message);
private:
  const base::FilePath output_file_path_;

private:
  DISALLOW_COPY_AND_ASSIGN(JsonToProtobufSerializer);
};
} // namespace self
#endif // JSON_TO_PROTOBUF_SERIALIZER_H_

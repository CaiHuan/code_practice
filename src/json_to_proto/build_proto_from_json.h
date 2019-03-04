#ifndef BUILD_PROTO_FROM_JSON_H_
#define BUILD_PROTO_FROM_JSON_H_

#include "base/values.h"
#include "base/files/file_path.h"
#include "base/macros.h"

namespace base {
  class DictionaryValue;
  class ListValue;
}

namespace google {
namespace protobuf {
  class DescriptorProto;
  class FileDescriptorProto;
} // namespace protobuf
} // google

namespace self {
class BuildProtoFromJson {
public:
  BuildProtoFromJson();

  ~BuildProtoFromJson();

  std::unique_ptr<google::protobuf::FileDescriptorProto> 
    CreateProtoFile(const base::DictionaryValue* dict_value);

private:
  void BuildProtoRecurveDictionaryValue(
    const base::DictionaryValue* dict_value,
    const std::string& key_name,
    google::protobuf::DescriptorProto* parent_desc_proto,
    google::protobuf::FileDescriptorProto* file_desc_proto);

  void BuildProtoRecurveListValue(
    const base::ListValue* list_value,
    const std::string& key_name,
    google::protobuf::DescriptorProto* parent_desc_proto,
    google::protobuf::FileDescriptorProto* file_desc_proto);

  void BuildProtoJudgeType(
    const base::Value* value,
    const std::string& key_name,
    google::protobuf::DescriptorProto* parent_desc_proto,
    google::protobuf::FileDescriptorProto* file_desc_proto);

  void BuildProtoListValueLogical(
    const base::Value* value,
    const std::string& key_name,
    google::protobuf::DescriptorProto* desc_proto,
    google::protobuf::FileDescriptorProto* file_desc_proto);

private:
  const base::FilePath output_file_path_;

private:
  DISALLOW_COPY_AND_ASSIGN(BuildProtoFromJson);
  };
} // namespace self
#endif // BUILD_PROTO_FROM_JSON_H_

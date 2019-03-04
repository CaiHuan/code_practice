#include "json_to_protobuf_serializer.h"

#include "base/files/file_path.h"

#include <cctype>
#include <map>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/values.h"
#include "base/strings/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "build_proto_from_json.h"

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"

namespace self {

namespace {
static const std::map<base::Value::Type, google::protobuf::FieldDescriptorProto::Type> kTypeConvertMap = {
  {base::Value::Type::BOOLEAN,    google::protobuf::FieldDescriptorProto::TYPE_BOOL},
  {base::Value::Type::INTEGER,    google::protobuf::FieldDescriptorProto::TYPE_INT64},
  {base::Value::Type::DOUBLE,     google::protobuf::FieldDescriptorProto::TYPE_DOUBLE},
  {base::Value::Type::STRING,     google::protobuf::FieldDescriptorProto::TYPE_STRING},
  {base::Value::Type::BINARY,     google::protobuf::FieldDescriptorProto::TYPE_BYTES},
  {base::Value::Type::DICTIONARY, google::protobuf::FieldDescriptorProto::TYPE_MESSAGE}
};

static google::protobuf::FieldDescriptorProto::Type
ConvertJsonTypeToProtoType(base::Value::Type type) {
  auto find = kTypeConvertMap.find(type);
  return find->second;
};

/*
* 用于生成例如
{
  "a": [
    {
      "a": "1",
      "b": "2"
    },
    {
      "c": "3",
      "d": "4"
    }
  ]
}
  因为数组内的对象很可能是不同对象因此就不能只是用一个简单的map来解决，需要为每个对象生成不同的对象
  那么对象映射其实会是一个匿名的message那么就需要为他加上一个类型名称，最后就用一个随机数
*/
static int GenerateIndex() {
  static int start = 1;
  return start++;
}
}

JsonToProtobufSerializer::JsonToProtobufSerializer(
  const base::FilePath& output_file_path)
  : output_file_path_(output_file_path) {
}

JsonToProtobufSerializer::~JsonToProtobufSerializer() {
}

bool JsonToProtobufSerializer::Serialize(
  const base::Value& root) {
  const base::DictionaryValue* root_dictionary;
  if (!root.GetAsDictionary(&root_dictionary)) {
    return false;
  }
  BuildProtoFromJson build_proto_from_json;
  const base::DictionaryValue* root_dict_value;
  if (!root.GetAsDictionary(&root_dict_value)) {
    return false;
  }
  std::unique_ptr<google::protobuf::DescriptorPool> desc_pool(new google::protobuf::DescriptorPool());
  std::unique_ptr<google::protobuf::FileDescriptorProto> file_desc_proto 
    = build_proto_from_json.CreateProtoFile(root_dict_value);
  const google::protobuf::FileDescriptor*  file_desc = desc_pool->BuildFile(*file_desc_proto.get());
  std::unique_ptr<google::protobuf::DynamicMessageFactory> dynamic_message_factory(
    new google::protobuf::DynamicMessageFactory);
  auto root_message = CreateMessage(root_dict_value, file_desc, dynamic_message_factory.get());

  std::string temp_serialize = root_message->SerializeAsString();
  const google::protobuf::Reflection* reflection = root_message->GetReflection();
  //reflection->SetString(message, field, "sfds");
  std::vector<const google::protobuf::FieldDescriptor*> field_descs;
  reflection->ListFields(*root_message.get(), &field_descs);
  return true;
}

std::unique_ptr<google::protobuf::Message> JsonToProtobufSerializer::CreateMessage(
  const base::DictionaryValue* root_dict,
  const google::protobuf::FileDescriptor* file_desc,
  google::protobuf::DynamicMessageFactory* dynamic_message_factory) {
  
  const google::protobuf::Descriptor* root_desc = file_desc->FindMessageTypeByName("ROOT");
  std::unique_ptr<google::protobuf::Message> root_message(dynamic_message_factory->GetPrototype(root_desc)->New());
  
  for (base::DictionaryValue::Iterator dict_iterator(*root_dict);
    !dict_iterator.IsAtEnd(); dict_iterator.Advance()) {
    AssignDataJudgeType(&dict_iterator.value(), dict_iterator.key(), root_desc, 
      root_message.get(), file_desc, dynamic_message_factory);
  }

  return std::move(root_message);
}

void JsonToProtobufSerializer::AssignDataRecurveDictionaryValue(
  const base::DictionaryValue* dict_value,
  const std::string& key_name,
  const google::protobuf::Descriptor* parent_desc,
  google::protobuf::Message* parent_message,
  const google::protobuf::FileDescriptor* file_desc,
  google::protobuf::DynamicMessageFactory* dynamic_message_factory) {

  std::string unique_key_name = key_name + "_" + base::IntToString(GenerateIndex());
  const google::protobuf::Descriptor* current_desc = file_desc->FindMessageTypeByName(base::ToUpperASCII(unique_key_name));
  std::unique_ptr<google::protobuf::Message> current_message(dynamic_message_factory->GetPrototype(current_desc)->New());
  for (base::DictionaryValue::Iterator dict_iterator(*dict_value);
    !dict_iterator.IsAtEnd();
    dict_iterator.Advance()) {
    AssignDataJudgeType(&dict_iterator.value(), dict_iterator.key(), current_desc, current_message.get(), file_desc, dynamic_message_factory);
  }
  const google::protobuf::Reflection* reflection = parent_message->GetReflection();
  auto field_desc = parent_desc->FindFieldByLowercaseName(unique_key_name);
  reflection->SetAllocatedMessage(parent_message, current_message.release(), field_desc);
}

void JsonToProtobufSerializer::AssignDataRecurveListValue(
  const base::ListValue* list_value,
  const std::string& key_name,
  const google::protobuf::Descriptor* parent_desc,
  google::protobuf::Message* parent_message,
  const google::protobuf::FileDescriptor* file_desc,
  google::protobuf::DynamicMessageFactory* dynamic_message_factory) {
  DCHECK(list_value);

  //每一个数组里面的子项都会分配一个message,由于json这些子项是不会有name的，所以每个子项的名
  //字在父项的名字基础上进行数字递增
  std::string unique_key_name = key_name + "_" + base::IntToString(GenerateIndex());
  std::string upper_key_name = base::ToUpperASCII(unique_key_name);
  std::string lower_key_name = base::ToLowerASCII(unique_key_name);
  const google::protobuf::Descriptor* current_desc = file_desc->FindMessageTypeByName(upper_key_name);
  std::unique_ptr<google::protobuf::Message> current_message(dynamic_message_factory->GetPrototype(current_desc)->New());

  for (int index = 0; index < list_value->GetSize(); index++) {
    const base::Value* value;
    if (!list_value->Get(index, &value)) {
      continue;
    }

    AssignDataJudgeType(value, lower_key_name + "_" + base::IntToString(GenerateIndex()), current_desc, current_message.get(), file_desc, dynamic_message_factory);
  }

  const google::protobuf::Reflection* parent_message_reflection = parent_message->GetReflection();
  auto filed_desc = parent_desc->FindFieldByLowercaseName(unique_key_name);
  parent_message_reflection->SetAllocatedMessage(parent_message, current_message.release(), filed_desc);
}

void JsonToProtobufSerializer::AssignDataJudgeType(
  const base::Value* value,
  const std::string& key_name,
  const google::protobuf::Descriptor* parent_desc,
  google::protobuf::Message* parent_message,
  const google::protobuf::FileDescriptor* file_desc,
  google::protobuf::DynamicMessageFactory* dynamic_message_factory) {

  base::Value::Type type = value->GetType();

  if (base::Value::Type::LIST == type) {
    AssignDataListValueLogical(value, key_name, parent_desc, parent_message, file_desc, dynamic_message_factory);

  } else if (base::Value::Type::DICTIONARY == type) {
    const base::DictionaryValue* dict_value;
    if (value->GetAsDictionary(&dict_value)) {
      AssignDataRecurveDictionaryValue(dict_value, key_name, parent_desc, parent_message, file_desc, dynamic_message_factory);
    }
  } else {
    AssignData(value, key_name, parent_desc, parent_message);
  }
}

void JsonToProtobufSerializer::AssignDataListValueLogical(
  const base::Value* value,
  const std::string& key_name,
  const google::protobuf::Descriptor* parent_desc,
  google::protobuf::Message* parent_message,
  const google::protobuf::FileDescriptor* file_desc,
  google::protobuf::DynamicMessageFactory* dynamic_message_factory) {
  DCHECK(value && parent_desc && parent_message && file_desc && dynamic_message_factory);

  const base::ListValue* list_value;
  if (value->GetAsList(&list_value) && !list_value->empty()) {
    const base::Value* probe_value;
    list_value->Get(0, &probe_value);
    base::Value::Type probe_type = probe_value->GetType();

    if ((base::Value::Type::LIST == probe_type) ||
      (base::Value::Type::DICTIONARY == probe_type)) {
      //只要是一个key:[] 的格式，那么就为每个子项生成一个message，一位内子项可能不一样
      AssignDataRecurveListValue(list_value, key_name, parent_desc, parent_message, file_desc, dynamic_message_factory);

    } else {
      AssignRepeatedData(list_value, key_name, parent_desc, parent_message);
    }
  }
}

void JsonToProtobufSerializer::AssignData(
  const base::Value* value,
  const std::string& key_name,
  const google::protobuf::Descriptor* parent_desc,
  google::protobuf::Message* parent_message) {

  base::Value::Type type = value->GetType();
  auto field_desc = parent_desc->FindFieldByLowercaseName(key_name);
  auto reflection = parent_message->GetReflection();
  switch (type) {
  case base::Value::Type::BOOLEAN: {
    bool temp = false;
    value->GetAsBoolean(&temp);
    reflection->SetBool(parent_message, field_desc, temp);
  } break;
  case base::Value::Type::INTEGER: {
    int temp = 0;
    value->GetAsInteger(&temp);
    reflection->SetInt64(parent_message, field_desc, temp);
  } break;
  case base::Value::Type::DOUBLE: {
    double temp = 0.0f;
    value->GetAsDouble(&temp);
    reflection->SetDouble(parent_message, field_desc, temp);
  } break;
  case base::Value::Type::STRING: {
    std::string temp;
    value->GetAsString(&temp);
    reflection->SetString(parent_message, field_desc, temp);
  } break;
  case base::Value::Type::BINARY: {
  } break;
  default: {
  } break;
  }
}

void JsonToProtobufSerializer::AssignRepeatedData(
  const base::ListValue* value,
  const std::string& key_name,
  const google::protobuf::Descriptor* parent_desc,
  google::protobuf::Message* parent_message) {
  
  const google::protobuf::FieldDescriptor* field_desc = parent_desc->FindFieldByLowercaseName(key_name);
  const google::protobuf::Reflection* reflection = parent_message->GetReflection();
  for (const std::unique_ptr<base::Value>& value_item : *value) {
    switch (value_item->GetType()) {
    case base::Value::Type::BOOLEAN: {
      bool temp = false;
      value->GetAsBoolean(&temp);
      reflection->AddBool(parent_message, field_desc, temp);
    } break;
    case base::Value::Type::INTEGER: {
      int temp = 0;
      value->GetAsInteger(&temp);
      reflection->AddInt64(parent_message, field_desc, temp);
    } break;
    case base::Value::Type::DOUBLE: {
      double temp = 0.0f;
      value->GetAsDouble(&temp);
      reflection->AddDouble(parent_message, field_desc, temp);
    } break;
    case base::Value::Type::STRING: {
      std::string temp;
      value->GetAsString(&temp);
      reflection->AddString(parent_message, field_desc, temp);
    } break;
    case base::Value::Type::BINARY: {
    } break;
    default: {
    } break;
    }
  }
}

} //namespace self
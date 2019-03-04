#include "build_proto_from_json.h"

#include "base/files/file_path.h"

#include <cctype>
#include <map>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "convert_switches.h"
#include "google/protobuf/stubs/common.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.h"
#include "google//protobuf/util/json_util.h"
#include "google/protobuf/dynamic_message.h"

namespace self {

namespace {
static const int kStartIndexTag = 1;

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

static int GenerateNumber() {
  static int start = 1;
  return start++;
}
};

BuildProtoFromJson::BuildProtoFromJson() {
}

BuildProtoFromJson::~BuildProtoFromJson() {
}

std::unique_ptr<google::protobuf::FileDescriptorProto>
BuildProtoFromJson::CreateProtoFile(
  const base::DictionaryValue* dict_value) {
  DCHECK(dict_value);
  std::unique_ptr<google::protobuf::FileDescriptorProto> file_desc_proto(
    new google::protobuf::FileDescriptorProto());
  file_desc_proto->set_name("superman.proto");
  file_desc_proto->set_package("superman_permission");
  BuildProtoRecurveDictionaryValue(dict_value, "root", nullptr, file_desc_proto.get());

  return std::move(file_desc_proto);
}

//////////////////////////////////////////////////////////////////////////
void BuildProtoFromJson::BuildProtoRecurveDictionaryValue(
  const base::DictionaryValue* dict_value, 
  const std::string& key_name,
  google::protobuf::DescriptorProto* parent_desc_proto,
  google::protobuf::FileDescriptorProto* file_desc_proto) {
  DCHECK(dict_value);

  std::string unique_key_name;
  if (parent_desc_proto) {
    unique_key_name = key_name + "_" + base::IntToString(GenerateIndex());
  } else {
    unique_key_name = key_name;
  }
  google::protobuf::DescriptorProto* current_desc_proto = file_desc_proto->add_message_type();
  current_desc_proto->set_name(base::ToUpperASCII(unique_key_name));
  if (parent_desc_proto) {
    google::protobuf::FieldDescriptorProto* field_desc_proto = parent_desc_proto->add_field();
    field_desc_proto->set_label(google::protobuf::FieldDescriptorProto_Label_LABEL_OPTIONAL);
    field_desc_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE);
    field_desc_proto->set_type_name(base::ToUpperASCII(unique_key_name));
    field_desc_proto->set_name(base::ToLowerASCII(unique_key_name));
    field_desc_proto->set_number(GenerateNumber());
  }
  for (base::DictionaryValue::Iterator dict_iterator(*dict_value);
    !dict_iterator.IsAtEnd();
    dict_iterator.Advance()) {
    BuildProtoJudgeType(&dict_iterator.value(), dict_iterator.key(), current_desc_proto, file_desc_proto);
  }
}

void BuildProtoFromJson::BuildProtoRecurveListValue(
  const base::ListValue* list_value,
  const std::string& key_name,
  google::protobuf::DescriptorProto* parent_desc_proto,
  google::protobuf::FileDescriptorProto* file_desc_proto) {
  DCHECK(list_value);

  //每一个数组里面的子项都会分配一个message,由于json这些子项是不会有name的，所以每个子项的名
  //字在父项的名字基础上进行数字递增
  std::string unique_key_name = key_name + "_" + base::IntToString(GenerateIndex());
  std::string upper_key_name = base::ToUpperASCII(unique_key_name);
  std::string lower_key_name = base::ToLowerASCII(unique_key_name);
  google::protobuf::DescriptorProto* current_desc_proto = file_desc_proto->add_message_type();
  current_desc_proto->set_name(upper_key_name);

  google::protobuf::FieldDescriptorProto* field_desc_proto = parent_desc_proto->add_field();
  field_desc_proto->set_label(google::protobuf::FieldDescriptorProto_Label_LABEL_OPTIONAL);
  field_desc_proto->set_type(google::protobuf::FieldDescriptorProto_Type_TYPE_MESSAGE);
  field_desc_proto->set_type_name(upper_key_name);
  field_desc_proto->set_name(lower_key_name);
  field_desc_proto->set_number(GenerateNumber());

  for (int index = 0; index < list_value->GetSize(); index++) {
    const base::Value* value;
    if (!list_value->Get(index, &value)) {
      continue;
    }

    BuildProtoJudgeType(value, lower_key_name + "_" + base::IntToString(GenerateIndex()), current_desc_proto, file_desc_proto);
  }
}

void BuildProtoFromJson::BuildProtoJudgeType(
  const base::Value* value,
  const std::string& key_name,
  google::protobuf::DescriptorProto* parent_desc_proto,
  google::protobuf::FileDescriptorProto* file_desc_proto) {
  DCHECK(value && parent_desc_proto && file_desc_proto);

  base::Value::Type type = value->GetType();

  if (base::Value::Type::LIST == type) {

    BuildProtoListValueLogical(value, key_name, parent_desc_proto, file_desc_proto);
  } else if (base::Value::Type::DICTIONARY == type) {

    const base::DictionaryValue* dict_value;
    if (value->GetAsDictionary(&dict_value)) {
      BuildProtoRecurveDictionaryValue(dict_value, key_name, parent_desc_proto, file_desc_proto);
    }
  } else {

    google::protobuf::FieldDescriptorProto* field_desc_proto = parent_desc_proto->add_field();
    field_desc_proto->set_label(google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL);
    field_desc_proto->set_type(ConvertJsonTypeToProtoType(type));
    field_desc_proto->set_name(key_name);
    field_desc_proto->set_number(GenerateNumber());
  }
}

void BuildProtoFromJson::BuildProtoListValueLogical(
  const base::Value* value,
  const std::string& key_name,
  google::protobuf::DescriptorProto* parent_desc_proto,
  google::protobuf::FileDescriptorProto* file_desc_proto) {
  DCHECK(value && parent_desc_proto);

  const base::ListValue* list_value;
  if (value->GetAsList(&list_value) && !list_value->empty()) {
    const base::Value* probe_value;
    list_value->Get(0, &probe_value);
    base::Value::Type probe_type = probe_value->GetType();

    if ((base::Value::Type::LIST == probe_type) ||
      (base::Value::Type::DICTIONARY == probe_type)) {
      //只要是一个key:[] 的格式，那么就为每个子项生成一个message，一位内子项可能不一样
      BuildProtoRecurveListValue(list_value, key_name, parent_desc_proto, file_desc_proto);

    } else {
      google::protobuf::FieldDescriptorProto* field_desc_proto = parent_desc_proto->add_field();
      field_desc_proto->set_label(google::protobuf::FieldDescriptorProto_Label_LABEL_REPEATED);
      field_desc_proto->set_type(ConvertJsonTypeToProtoType(probe_type));
      field_desc_proto->set_name(key_name);
      field_desc_proto->set_number(GenerateNumber());
    }
  }
}

} //namespace self
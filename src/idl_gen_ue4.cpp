/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// independent from idl_parser, since this code is not needed for most clients

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace flatbuffers {

#if 0
namespace cpp {
// from idl_gen_cpp.cpp
static std::string WrapInNameSpace(const Parser &parser,
                                   const Definition &def);

static std::string GenTypeGet(const Parser &parser, const Type &type,
                              const char *afterbasic, const char *beforeptr,
                              const char *afterptr, bool real_enum);
} // namespace cpp
#endif

namespace ue4 {

static std::string CPPClassName(const Definition &def) {
    std::string qualified_name;
    auto ns = def.defined_namespace;
    for (auto it = ns->components.begin();
             it != ns->components.end(); ++it) {
      qualified_name += *it + "::";
    }
    return qualified_name + def.name;
}

static std::string UE4ClassName(const Definition &def) {
	return "UFB" + def.name;
}

// Return a C++ type from the table in idl.h
static std::string GenTypeBasic(const Parser &/*parser*/, const Type &type,
                                bool real_enum) {
  static const char *ctypename[] = {
    // Unreal types happen to match Python types.
    #define FLATBUFFERS_TD(ENUM, IDLTYPE, CTYPE, JTYPE, GTYPE, NTYPE, PTYPE, UTYPE) \
      #UTYPE,
      FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
    #undef FLATBUFFERS_TD
  };
  return real_enum && type.enum_def
      ? "E" + type.enum_def->name
      : ctypename[type.base_type];
}

static std::string GenTypeWire(const Parser &parser, const Type &type,
                               const char *postfix, bool real_enum);

// Return a C++ pointer type, specialized to the actual struct/table types,
// and vector element types.
static std::string GenTypePointer(const Parser &parser, const Type &type) {
  switch (type.base_type) {
    case BASE_TYPE_STRING:
      return "const FString &";
    case BASE_TYPE_VECTOR:
      return "const TArray<" +
             GenTypeWire(parser, type.VectorType(), "", false) + "> &";
    case BASE_TYPE_STRUCT: {
      return UE4ClassName(*type.struct_def) + " *";
    }
    case BASE_TYPE_UNION:
      // fall through
    default:
      return "void";
  }
}

// Return a C++ type for any type (scalar/pointer) specifically for
// building a flatbuffer.
static std::string GenTypeWire(const Parser &parser, const Type &type,
                               const char *postfix, bool real_enum) {
  return IsScalar(type.base_type)
    ? GenTypeBasic(parser, type, real_enum) + postfix
    : GenTypePointer(parser, type);
}

// Return a C++ type for any type (scalar/pointer) specifically for
// using a flatbuffer.
static std::string GenTypeGet(const Parser &parser, const Type &type, bool real_enum) {
  return (IsScalar(type.base_type)
    ? GenTypeBasic(parser, type, real_enum)
    : GenTypePointer(parser, type)) + " ";
}

// Generate an enum declaration and an enum string lookup table.
static void GenEnum(const Parser &/*parser*/, EnumDef &enum_def,
                    std::string *code_ptr, const GeneratorOptions &/*opts*/) {
  if (enum_def.generated) return;
  std::string &code = *code_ptr;
  GenComment(enum_def.doc_comment, code_ptr, nullptr);
  code += "UENUM(BlueprintType)\n";
  code += "enum class E" + enum_def.name + " {\n";
  for (auto it = enum_def.vals.vec.begin();
       it != enum_def.vals.vec.end();
       ++it) {
    auto &ev = **it;
    GenComment(ev.doc_comment, code_ptr, nullptr, "  ");
    code += "  " + ev.name + " = ";
    code += NumToString(ev.value);
    code += (it + 1) != enum_def.vals.vec.end() ? ",\n" : "\n";
  }
  code += "};\n\n";
}

// Generates a value with optionally a cast applied if the field has a
// different underlying type from its interface type (currently only the
// case for enums. "from" specify the direction, true meaning from the
// underlying type to the interface type.
std::string GenUnderlyingCast(const Parser &parser, const FieldDef &field,
                              bool from, const std::string &val) {
  return field.value.type.enum_def && IsScalar(field.value.type.base_type)
      ? "static_cast<" + GenTypeBasic(parser, field.value.type, from) + ">(" +
        val + ")"
      : val;
}
#if 0
static void GenConstructors(StructDef &struct_def, std::string *code_ptr) {
  std::string &code = *code_ptr;
  std::string ue4_class = UE4ClassName(struct_def);
  std::string cpp_class = CPPClassName(struct_def);

  // static Create method should be used instead of constructor since UE4 requires constructor
  // have no parameters
  code += "  static " + ue4_class + " *Create(const " + cpp_class + " *flatbuffer) {\n";
  code += "    auto o = NewObject<" + ue4_class + ">();\n";
  code += "    o->flatbuffer_ = flatbuffer;\n";
  for (auto it = struct_def.fields.vec.begin();
       it != struct_def.fields.vec.end();
       ++it) {
    auto &field = **it;
    if (!field.deprecated) {
        switch (field.value.type.base_type) {
          case BASE_TYPE_STRING:
            code += "    o->" + field.name + "_ = flatbuffer->" + field.name + "()->c_str();\n";
            break;
          case BASE_TYPE_VECTOR:
            code += "    for (auto elem : *flatbuffer->" + field.name + "()) {\n";
            code += "      o->" + field.name + "_.Add(elem);\n    }\n";
            break;
          case BASE_TYPE_STRUCT:
            code += "    o->" + field.name + "_ = " + UE4ClassName(*field.value.type.struct_def);
            code += "::Create(";
            if (struct_def.fixed) {
              code += "&";
            }
            code += "flatbuffer->" + field.name + "());\n";
            break;
          default:
            break;
        }
    }
  }
  code += "    return o;\n  }\n";

  //code += "  const " + cpp_class + " *RawFlatbuffer() const { return flatbuffer_; }\n\n";
}
#endif

static void GenDeserialize(StructDef &struct_def, std::string *code_ptr) {
  std::string &code = *code_ptr;
  std::string ue4_class = UE4ClassName(struct_def);
  std::string cpp_class = CPPClassName(struct_def);

  // static Create method should be used instead of constructor since UE4 requires constructor
  // have no parameters
  code += "  " + ue4_class + " (const " + cpp_class + " &flatbuffer) {\n";
  for (auto it = struct_def.fields.vec.begin();
       it != struct_def.fields.vec.end();
       ++it) {
    auto &field = **it;
    if (!field.deprecated) {
        if (IsScalar(field.value.type.base_type)) {
            code += "    " + field.name + " = flatbuffer." + field.name + "();\n";
        }
    }
  }
  code += "  }\n";

  //code += "  const " + cpp_class + " *RawFlatbuffer() const { return flatbuffer_; }\n\n";
}

#if 0
static void GenMembers(const Parser &parser, StructDef &struct_def, std::string *code_ptr) {
  std::string &code = *code_ptr;

  std::string cpp_class = CPPClassName(struct_def);
  code += "  const " + cpp_class + " *flatbuffer_;\n";
  for (auto it = struct_def.fields.vec.begin();
       it != struct_def.fields.vec.end();
       ++it) {
    auto &field = **it;
    if (!field.deprecated) {
        switch (field.value.type.base_type) {
          case BASE_TYPE_STRING:
            code += "  FString " + field.name + "_;\n";
            break;
          case BASE_TYPE_VECTOR:
            code += "  TArray<" + GenTypeWire(parser, field.value.type.VectorType(), "", false) + "> ";
            code += field.name + "_;\n";
            break;
          case BASE_TYPE_STRUCT:
            code += "  " + UE4ClassName(*field.value.type.struct_def) + " *" + field.name + "_;\n";
            break;
          default:
            break;
        }
    }
  }
}
#endif

// Generate an accessor struct, builder structs & function for a table.
static void GenTable(const Parser &parser, StructDef &struct_def,
                     const GeneratorOptions &/*opts*/, std::string *code_ptr) {
  if (struct_def.generated) return;
  std::string &code = *code_ptr;

  // Generate an accessor struct, with methods of the form:
  // type name() const { return GetField<type>(offset, defaultval); }
  GenComment(struct_def.doc_comment, code_ptr, nullptr);
  auto ue4_class = UE4ClassName(struct_def);
  auto cpp_class = CPPClassName(struct_def);
  code += "USTRUCT()\n";
  code += "struct " + ue4_class + " {\n";
  code += "  GENERATED_USTRUCT_BODY()\n";
  //code += " private:\n";
  //GenMembers(parser, struct_def, &code);

  //code += "\n public:\n";
  GenDeserialize(struct_def, &code);
  for (auto it = struct_def.fields.vec.begin();
       it != struct_def.fields.vec.end();
       ++it) {
    auto &field = **it;
    if (!field.deprecated) {  // Deprecated fields won't be accessible.
      //auto is_scalar = IsScalar(field.value.type.base_type);
      GenComment(field.doc_comment, code_ptr, nullptr, "  ");
      code += "  UPROPERTY()\n";
      code += "  const " + GenTypeGet(parser, field.value.type, true);
      code += field.name + ";\n";

      // Call a different accessor for pointers, that indirects.
//      auto accessor = is_scalar
  //      ? "GetField<"
    //    : (IsStruct(field.value.type) ? "GetStruct<" : "GetPointer<");
      //code += GenUnderlyingCast(parser, field, true, call);
      //code += "; }\n";
    }
  }
  code += "};\n\n";
}

// Generate an accessor class with a flatbuffers pointer member variable.
static void GenStruct(const Parser &parser, StructDef &struct_def,
                      const GeneratorOptions &/*opts*/,  std::string *code_ptr) {
  if (struct_def.generated) return;
  std::string &code = *code_ptr;

  // Generate an accessor struct, with private variables of the form:
  // type name_;
  // Generates manual padding and alignment.
  // Variables are private because they contain little endian data on all
  // platforms.
  GenComment(struct_def.doc_comment, code_ptr, nullptr);
  auto ue4_class = UE4ClassName(struct_def);
  auto cpp_class = CPPClassName(struct_def);
  code += "USTRUCT()\n";
  code += "struct " + ue4_class + " {\n";
  code += "  GENERATED_USTRUCT_BODY()\n\n";
  //code += " private:\n";
  //GenMembers(parser, struct_def, &code);

  //code += "\n public:\n";
  GenDeserialize(struct_def, &code);

  // Generate accessor methods of the form:
  // type name() const { return flatbuffers::EndianScalar(name_); }
  for (auto it = struct_def.fields.vec.begin();
       it != struct_def.fields.vec.end();
       ++it) {
    auto &field = **it;
    GenComment(field.doc_comment, code_ptr, nullptr, "  ");
    code += "  UPROPERTY()\n";
    code += "  const " + GenTypeGet(parser, field.value.type, true);
    code += field.name + ";\n";
  }
  code += "};\n\n";
}

}  // namespace ue4

// Iterate through all definitions we haven't generate code for (enums, structs,
// and tables) and output them to a single file.
// scs: skip enum generation
// kill the namespcing
// new ifndef
std::string GenerateUE4(const Parser &parser,
                        const std::string &file_name,
                        const GeneratorOptions &opts) {
  using namespace ue4;

  // Generate code for all the enum declarations.
  std::string enum_code;
  for (auto it = parser.enums_.vec.begin();
       it != parser.enums_.vec.end(); ++it) {
    GenEnum(parser, **it, &enum_code, opts);
  }

  // Generate forward declarations for all structs/tables, since they may
  // have circular references.
  // itoys: removed namespace related code since UE4 doesn't support UCLASS in namespaces.
  std::string forward_decl_code;
  for (auto it = parser.structs_.vec.begin();
       it != parser.structs_.vec.end(); ++it) {
    auto &struct_def = **it;
    auto ue4_class = UE4ClassName(struct_def);
    forward_decl_code += "class " + ue4_class + ";\n";
  }

  // Generate code for all structs, then all tables.
  std::string decl_code;
  for (auto it = parser.structs_.vec.begin();
       it != parser.structs_.vec.end(); ++it) {
    if ((**it).fixed) GenStruct(parser, **it, opts, &decl_code);
  }
  for (auto it = parser.structs_.vec.begin();
       it != parser.structs_.vec.end(); ++it) {
    if (!(**it).fixed) GenTable(parser, **it, opts, &decl_code);
  }

  // Only output file-level code if there were any declarations.
  if (enum_code.length() || decl_code.length()) {
    std::string code;
    code = "// automatically generated by the FlatBuffers compiler,"
           " do not modify\n\n";

    // Generate include guard.
    code += "#pragma once\n";

    // include flatbuffers cpp implementation
    code += "#include \"" + file_name + "_generated.h\"\n";

    if (opts.include_dependence_headers) {
      int num_includes = 0;
      for (auto it = parser.included_files_.begin();
           it != parser.included_files_.end(); ++it) {
        auto basename = flatbuffers::StripPath(
                          flatbuffers::StripExtension(it->first));
        if (basename != file_name) {
          // TODO: include other ue4 objects since they may reference eachother
          code += "#include \"" + basename + "_generated.h\"\n";
          num_includes++;
        }
      }
      if (num_includes) code += "\n";
    }
    // UE4 generated include goes last
    code += "#include \"" + file_name + "_ue4_generated.generated.h\"\n\n";

    code += forward_decl_code;
    code += "\n";

    // Output the main declaration code from above.
    code += enum_code;
    code += decl_code;

    return code;
  }

  return std::string();
}

static std::string GeneratedFileName(const std::string &path,
                                     const std::string &file_name) {
  return path + file_name + "_ue4_generated.h";
}

bool GenerateUE4(const Parser &parser,
                 const std::string &path,
                 const std::string &file_name,
                 const GeneratorOptions &opts) {
    auto code = GenerateUE4(parser, file_name, opts);
    return !code.length() ||
           SaveFile(GeneratedFileName(path, file_name).c_str(), code, false);
}

std::string UE4MakeRule(const Parser &parser,
                        const std::string &path,
                        const std::string &file_name,
                        const GeneratorOptions & /*opts*/) {
  std::string filebase = flatbuffers::StripPath(
      flatbuffers::StripExtension(file_name));
  std::string make_rule = GeneratedFileName(path, filebase) + ": ";
  auto included_files = parser.GetIncludedFilesRecursive(file_name);
  for (auto it = included_files.begin();
       it != included_files.end(); ++it) {
    make_rule += " " + *it;
  }
  return make_rule;
}

}  // namespace flatbuffers


#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <cctype>

#include "thrift/generate/t_oop_generator.h"
#include "thrift/platform.h"

#define WITH_DOC 1

using std::map;
using std::string;
using std::vector;

constexpr const char* endl = "\n";

string to_upper_case(string str);
string to_lower_case(string str);
string camel_case_to_underscores(string str);

class t_c_api_generator : public t_oop_generator {
public:
  /* constructor */
  t_c_api_generator(t_program* program,
                    const map<string, string>& parsed_options,
                    const string& option_string)
    : t_oop_generator(program), nspace_(""), nspace_uc_(""), nspace_lc_("") {
    out_dir_base_ = "gen-c_api";

    nspace_ = program->get_namespace("cpp");

    if (!nspace_.empty()) {
      for (auto&& c : nspace_) {
        if (c == '.') {
          c = '_';
        }
      }
    }

    nspace_lc_ = camel_case_to_underscores(nspace_) + "_";
    nspace_uc_ = to_upper_case(nspace_lc_);
  }

protected:
  virtual void init_generator() override;
  virtual void close_generator() override;

  virtual void generate_typedef(t_typedef* ttypedef) override;
  virtual void generate_enum(t_enum* tenum) override;
  virtual void generate_const(t_const* tconst) override;
  virtual void generate_struct(t_struct* tstruct) override;
  virtual void generate_service(t_service* tservice) override;
  virtual void generate_forward_declaration(t_struct*) override;

  virtual std::string indent_str() const override { return "\t"; }

private:
  /* file streams */
  ofstream_with_content_based_conditional_update f_header;
  ofstream_with_content_based_conditional_update f_source;
  ofstream_with_content_based_conditional_update f_csharp;

  string nspace_;
  string nspace_uc_;
  string nspace_lc_;

  string define_name_;

  // Formatting only variables;
  bool has_typedef_ = false;
  bool has_const_ = false;
  bool first_fw_decl_ = true;
  bool first_struct_ = true;

private:
  string get_type_string(t_type* type);
  std::ostream& make_doc(std::ostream& stream, t_doc* doc, bool add_lf = false);

  std::ostream& open_scope(std::ostream& stream);
  std::ostream& close_scope(std::ostream& stream);

  // Generators
  void generate_struct_header(t_struct* tstruct);
  void generate_struct_csharp(t_struct* tstruct);

  void generate_service_header(t_service* tservice);
  void generate_service_source(t_service* tservice);
  void generate_service_csharp(t_service* tservice);
};

void t_c_api_generator::init_generator() {
  MKDIR(get_out_dir().c_str());

  string f_c_header_name = get_out_dir() + program_name_ + "_api.h";
  f_header.open(f_c_header_name.c_str());

  string f_c_source_name = get_out_dir() + program_name_ + "_api.c";
  f_source.open(f_c_source_name.c_str());

  f_header << autogen_comment();
  f_source << autogen_comment();
  // string f_cs_source_name = get_out_dir() + program_name_ + "_api.cs";

  define_name_ = nspace_uc_ + "_" + to_upper_case(program_name_) + "_H";

  f_header << "#ifndef _" << define_name_ << endl;
  f_header << "#define _" << define_name_ << endl;
  f_header << endl;

  f_header << "#include <stdint.h>" << endl;
  f_header << endl;

  // Export macro
  f_header << "#ifdef _WINDOWS" << endl;
  f_header << "#  if defined(THRIFT_COMPILED)" << endl;
  f_header << "#    define THRIFT_DLLEXPORT __declspec(dllexport)" << endl;
  f_header << "#  elif defined(THRIFT_LINKED)" << endl;
  f_header << "#    define THRIFT_DLLEXPORT __declspec(dllimport)" << endl;
  f_header << "#  else" << endl;
  f_header << "#    define THRIFT_DLLEXPORT" << endl;
  f_header << "#  endif" << endl;
  f_header << "#else" << endl;
  f_header << "#  define THRIFT_DLLEXPORT" << endl;
  f_header << "#endif" << endl;
  f_header << endl;

  // C++ specific
  f_header << "#ifdef __cplusplus" << endl;
  f_header << "extern \"C\"" << endl;
  f_header << "#endif" << endl;
  open_scope(f_header);
  // TODO: Define list, set, map types
}

void t_c_api_generator::close_generator() {
  // Close tags
  close_scope(f_header) << endl;

  f_header << endl << "#endif // _" << define_name_ << endl;

  f_header.close();
  f_source.close();
}

void t_c_api_generator::generate_typedef(t_typedef* ttypedef) {
  has_typedef_ = true;
  make_doc(f_header, ttypedef);
  indent(f_header) << "typedef " << get_type_string(ttypedef->get_type()) << " "
                   << ttypedef->get_symbolic() << endl;
}

void t_c_api_generator::generate_enum(t_enum* tenum) {
  string name = nspace_ + tenum->get_name();

  make_doc(f_header, tenum);
  indent(f_header) << "typedef struct _" << name << endl;
  open_scope(f_header);

  indent(f_header) << "enum type" << endl;
  open_scope(f_header);

  for (size_t i = 0; i < tenum->get_constants().size(); ++i) {
    t_enum_value* value = tenum->get_constants()[i];
    make_doc(f_header, value, i != 0);
    indent(f_header) << value->get_name() << " = " << value->get_value() << "," << endl;
  }

  close_scope(f_header) << ";" << endl;
  close_scope(f_header) << " " << name << ";" << endl << endl;
}

void t_c_api_generator::generate_const(t_const* tconst) {
  has_const_ = true;
  indent(f_header) << "// " << get_type_string(tconst->get_type()) << " " << tconst->get_name()
                   << " = " << tconst->get_value() << endl;
}

void t_c_api_generator::generate_struct(t_struct* tstruct) {
  generate_struct_header(tstruct);
  generate_struct_csharp(tstruct);
}

void t_c_api_generator::generate_struct_header(t_struct* tstruct) {
  if (first_struct_) {
    f_header << endl;
    first_struct_ = false;
  }
  string name = nspace_ + tstruct->get_name();

  make_doc(f_header, tstruct);
  indent(f_header) << "typedef struct _" << name << endl;
  open_scope(f_header);

  for (size_t i = 0; i < tstruct->get_sorted_members().size(); ++i) {
    t_field* field = tstruct->get_sorted_members()[i];
    t_type* type = field->get_type();

    make_doc(f_header, field, i != 0);
    indent(f_header) << get_type_string(type) << " " << field->get_name() << ";" << endl;
  }

  close_scope(f_header) << " " << name << ";" << endl << endl;
}

void t_c_api_generator::generate_struct_csharp(t_struct* tstruct) {
  // TODO
}

void t_c_api_generator::generate_service(t_service* tservice) {
  generate_service_header(tservice);
  generate_service_source(tservice);
  generate_service_csharp(tservice);
}

void t_c_api_generator::generate_service_header(t_service* tservice) {
  if (has_const_) {
    f_header << endl;
  }

  // We do not care about extends keyword for now

  const string service_name = nspace_lc_ + camel_case_to_underscores(tservice->get_name()) + "_";

  auto functions = tservice->get_functions();
  for (size_t i = 0; i < functions.size(); ++i) {
    t_function* function = functions[i];
    const string function_name = service_name + camel_case_to_underscores(function->get_name());
    std::stringstream s_function;

    t_type* return_type = function->get_returntype();
    std::vector<t_field*> params = function->get_arglist()->get_members();

    make_doc(f_header, function, i != 0);
    s_function << "THRIFT_DLLEXPORT ";
    s_function << get_type_string(return_type) << " " << function_name << "(";

    size_t definition_size = s_function.str().size();

    for (size_t j = 0; j < params.size(); ++j) {
      t_field* param = params[j];

      const string param_type = get_type_string(param->get_type());
      const string param_name = param->get_name();

      if (j != 0) {
        s_function << "," << endl;
        indent(s_function);
        for (size_t k = 0; k < definition_size; ++k) {
          s_function << " ";
        }
      }
      s_function << param_type << " " << param_name;
    }

    s_function << ");" << endl;

    indent(f_header) << s_function.str();
  }
}

void t_c_api_generator::generate_service_source(t_service* tservice) {
  // TODO
}

void t_c_api_generator::generate_service_csharp(t_service* tservice) {
  // TODO
}

void t_c_api_generator::generate_forward_declaration(t_struct* tstruct) {
  if (first_fw_decl_ && has_typedef_) {
    f_header << endl;
    first_fw_decl_ = false;
  }
  indent(f_header) << "struct " << tstruct->get_name() << ";" << endl;
}

string t_c_api_generator::get_type_string(t_type* type) {
  if (type->is_base_type()) {
    t_base_type::t_base base_type = ((t_base_type*)type)->get_base();
    switch (base_type) {
    case t_base_type::TYPE_VOID:
      return "void";
    case t_base_type::TYPE_BOOL:
      return "int";
    case t_base_type::TYPE_I8:
      return "int8_t";
    case t_base_type::TYPE_I16:
      return "int16_t";
    case t_base_type::TYPE_I32:
      return "int32_t";
    case t_base_type::TYPE_I64:
      return "int64_t";
    case t_base_type::TYPE_DOUBLE:
      return "double";
    case t_base_type::TYPE_STRING:
      return "char*";
    }
  } else if (type->is_container()) {
    // TODO
    if (type->is_list()) {
      t_list* tlist = static_cast<t_list*>(type);
      return "list<" + get_type_string(tlist->get_elem_type()) + ">";
    }
    if (type->is_map()) {
      t_map* tmap = static_cast<t_map*>(type);
      string key_string = get_type_string(tmap->get_key_type());
      string val_string = get_type_string(tmap->get_val_type());
      return "map<" + key_string + ", " + val_string + ">";
    }
    if (type->is_set()) {
      t_set* tset = static_cast<t_set*>(type);
      return "set<" + get_type_string(tset->get_elem_type()) + ">";
    }
  } else {
    return type->get_name();
  }

  return "";
}

std::ostream& t_c_api_generator::make_doc(std::ostream& stream, t_doc* doc, bool add_lf) {
#if WITH_DOC
  if (doc->has_doc()) {
    if (add_lf) {
      stream << endl;
    }

    indent(stream) << "/**" << endl;

    string tmp;
    std::stringstream ss(doc->get_doc());

    while (std::getline(ss, tmp)) {
      indent(stream) << " * " << tmp << endl;
    }
    indent(stream) << " */" << endl;
  }
#endif
  return stream;
}

std::ostream& t_c_api_generator::open_scope(std::ostream& stream) {
  indent(stream) << "{" << endl;
  indent_up();
  return stream;
}

std::ostream& t_c_api_generator::close_scope(std::ostream& stream) {
  indent_down();
  indent(stream) << "}";
  return stream;
}

string to_upper_case(string str) {
  string s(str);
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}

string to_lower_case(string str) {
  string s(str);
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  return s;
}

// Transform ACamelCase string to a_camel_case one
string camel_case_to_underscores(string str) {
  string result;
  result += tolower(str[0]);
  for (size_t i = 1; i < str.size(); ++i) {
    char lower_case = tolower(str[i]);
    if (lower_case != str[i]) {
      result += '_';
    }
    result += lower_case;
  }
  return result;
}

THRIFT_REGISTER_GENERATOR(c_api, "C API for DXT purposes", "")

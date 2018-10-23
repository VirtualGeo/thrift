#include <fstream>
#include <ostream>
#include <set>
#include <string>
#include <vector>

#include <cassert>
#include <cctype>

#include "thrift/generate/t_oop_generator.h"
#include "thrift/platform.h"

#define WITH_DOC 1

using std::map;
using std::set;
using std::string;
using std::vector;

constexpr const char* endl = "\n";

string to_upper_case(string str);
string to_lower_case(string str);
string camel_case_to_underscores(string str);
string dots_to_underscore(string str);

bool is_base_type(t_type* type);
t_type* get_underlying_type(t_type* type);

class t_c_api_generator : public t_oop_generator {
public:
  /* constructor */
  t_c_api_generator(t_program* program,
                    const map<string, string>& parsed_options,
                    const string& option_string)
    : t_oop_generator(program), nspace_(""), nspace_uc_(""), nspace_lc_("") {
    out_dir_base_ = "gen-c_api";

    nspace_ = program->get_namespace("cpp");
    cs_nspace_ = program->get_namespace("csharp");

    if (!nspace_.empty()) {
      nspace_ = dots_to_underscore(nspace_);
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

  string cs_nspace_;

  string define_name_;

  // Formatting only variables;
  bool has_typedef_ = false;
  bool has_const_ = false;
  bool first_fw_decl_ = true;
  bool first_struct_ = true;

  int previous_indent_ = 0;
  int source_indent_ = 0;
  int csharp_indent_ = 0;

  set<std::string> cs_keywords_;

private:
  std::ostream& make_doc(std::ostream& stream, t_doc* doc, bool add_lf = false);

  std::ostream& open_scope(std::ostream& stream);
  std::ostream& close_scope(std::ostream& stream);

  // (De-)Init
  void init_header();
  void init_source();
  void init_csharp();

  void close_header();
  void close_source();
  void close_csharp();

  // Generators
  void generate_struct_header(t_struct* tstruct);
  void generate_struct_csharp(t_struct* tstruct);

  void generate_service_header(t_service* tservice);
  void generate_service_source(t_service* tservice);
  void generate_service_csharp(t_service* tservice);

  void convert_c_type_to_cpp(t_type* type, string c_name, string cpp_name);
  void convert_cpp_type_to_c(t_type* type, string cpp_name, string c_name);
  void convert_cs_type_to_c(t_type* type, string cs_name, string c_name);
  void convert_c_type_to_cs(t_type* type, string c_name, string cs_name);

  string get_c_type_name(t_type* type) const;
  string get_cpp_type_name(t_type* type) const;
  string get_cs_type_name(t_type* type) const;
  string get_cs_to_c_type_name(t_type* type, bool is_param = false) const;

  string get_c_struct_name(t_type* type) const;
  string get_cpp_struct_name(t_type* type) const;
  string get_cs_struct_name(t_type* type) const;

  string get_c_enum_value_name(t_enum* tenum, t_enum_value* tvalue) const;
  string get_cpp_enum_value_name(t_enum* tenum, t_enum_value* tvalue) const;
  string get_cs_enum_value_name(t_enum* tenum, t_enum_value* tvalue) const;

  string get_c_function_signature(t_function* tfunction, string prefix, bool dllexport);
  string get_init_service_name(t_service* tservice) const;

  string get_cs_dll_import(t_service* tservice) const;

  void push_indent(int indent);
  void pop_indent(int* indent);
  void set_indent(int indent);

  std::ostream& align_fn_params(std::ostream& stream, size_t length);

  void init_csharp_keywords();
};

void t_c_api_generator::init_generator() {
  MKDIR(get_out_dir().c_str());

  init_header();
  init_source();
  init_csharp();
}

void t_c_api_generator::init_header() {
  string name = get_out_dir() + program_name_ + "_api.h";
  f_header.open(name.c_str());

  f_header << autogen_comment();

  define_name_ = nspace_uc_ + "_" + to_upper_case(program_name_) + "_H";

  f_header << "#ifndef _" << define_name_ << endl;
  f_header << "#define _" << define_name_ << endl;
  f_header << endl;

  f_header << "#include <stdint.h>" << endl;
  f_header << endl;

  // Export macro
  f_header << "#ifdef _WINDOWS" << endl;
  f_header << "#  if defined(THRIFT_C_API_COMPILED)" << endl;
  f_header << "#    define THRIFT_C_API_DLLEXPORT __declspec(dllexport)" << endl;
  f_header << "#  elif defined(THRIFT_C_API_LINKED)" << endl;
  f_header << "#    define THRIFT_C_API_DLLEXPORT __declspec(dllimport)" << endl;
  f_header << "#  else" << endl;
  f_header << "#    define THRIFT_C_API_DLLEXPORT" << endl;
  f_header << "#  endif" << endl;
  f_header << "#else" << endl;
  f_header << "#  define THRIFT_C_API_DLLEXPORT" << endl;
  f_header << "#endif" << endl;
  f_header << endl;

  // C++ specific
  f_header << "#ifdef __cplusplus" << endl;
  f_header << "extern \"C\"" << endl;
  f_header << "#endif" << endl;
  open_scope(f_header);
  // TODO: Define list, set, map types
}

void t_c_api_generator::init_source() {
  string base_name = get_out_dir() + program_name_ + "_api";
  string name = get_out_dir() + program_name_ + "_api.cpp";
  string header_name = program_name_ + "_api.h";

  f_source.open(name.c_str());
  f_source << autogen_comment() << endl;

  f_source << "#include <assert.h>" << endl << endl;

  f_source << "#include \"" << header_name << "\"" << endl << endl;

  f_source << "// C++ services" << endl;
  auto services = program_->get_services();
  for (const auto& service : services) {
    const string service_name = service->get_name();
    f_source << "#include \"" << service_name << "Handler.h"
             << "\"" << endl;
  }
  f_source << endl;
}

void t_c_api_generator::init_csharp() {
  string name = get_out_dir() + program_name_ + "_api.cs";
  f_csharp.open(name.c_str());
  f_csharp << autogen_comment() << endl;

  f_csharp << "using System.Runtime.InteropServices;" << endl << endl;

  if (!cs_nspace_.empty()) {
    f_csharp << "namespace " << cs_nspace_ << endl;
    push_indent(csharp_indent_);
    open_scope(f_csharp);
    pop_indent(&csharp_indent_);
  }

  init_csharp_keywords();
}

void t_c_api_generator::close_generator() {
  close_header();
  close_source();
  close_csharp();
}

void t_c_api_generator::close_header() {
  // Close tags
  close_scope(f_header) << endl;
  f_header << endl << "#endif // _" << define_name_ << endl;
  f_header.close();
}

void t_c_api_generator::close_source() {
  f_source.close();
}

void t_c_api_generator::close_csharp() {
  if (!cs_nspace_.empty()) {
    push_indent(csharp_indent_);
    close_scope(f_csharp);
    pop_indent(&csharp_indent_);
  }
  f_csharp.close();
}

void t_c_api_generator::generate_typedef(t_typedef* ttypedef) {
  (void)ttypedef;
  // has_typedef_ = true;
  // const string name = nspace_ + "_" + ttypedef->get_symbolic();
  // make_doc(f_header, ttypedef);
  // indent(f_header) << "typedef " << get_c_type_name(ttypedef->get_type()) << " " << name << ";"
  //                  << endl;
}

void t_c_api_generator::generate_enum(t_enum* tenum) {
  const string name = nspace_ + "_" + tenum->get_name();

  indent(f_header) << "enum " << name << endl;
  open_scope(f_header);

  for (size_t i = 0; i < tenum->get_constants().size(); ++i) {
    t_enum_value* value = tenum->get_constants()[i];
    make_doc(f_header, value, i != 0);
    indent(f_header) << get_c_enum_value_name(tenum, value) << " = " << value->get_value() << ","
                     << endl;
  }

  close_scope(f_header) << ";" << endl << endl;
}

void t_c_api_generator::generate_const(t_const* tconst) {
  has_const_ = true;
  indent(f_header) << "// " << get_c_type_name(tconst->get_type()) << " " << tconst->get_name()
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
  string name = get_c_struct_name(tstruct);

  make_doc(f_header, tstruct);
  indent(f_header) << "typedef struct _" << name << endl;
  open_scope(f_header);

  for (size_t i = 0; i < tstruct->get_sorted_members().size(); ++i) {
    t_field* field = tstruct->get_sorted_members()[i];
    t_type* type = field->get_type();

    make_doc(f_header, field, i != 0);
    indent(f_header) << get_c_type_name(type) << " " << field->get_name() << ";" << endl;
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

  // Add a function which will initialize the service on the C++ side.
  indent(f_header) << "THRIFT_C_API_DLLEXPORT void init_service_" << get_init_service_name(tservice)
                   << ";" << endl;
  indent(f_header) << "THRIFT_C_API_DLLEXPORT void close_service_"
                   << get_init_service_name(tservice) << ";" << endl;

  auto functions = tservice->get_functions();
  for (size_t i = 0; i < functions.size(); ++i) {
    t_function* function = functions[i];
    make_doc(f_header, function, true);
    string function_sig = get_c_function_signature(function, service_name, true) + ";";

    indent(f_header) << function_sig << endl;
  }
}

void t_c_api_generator::generate_service_source(t_service* tservice) {
  push_indent(source_indent_);

  const string service_name = nspace_lc_ + camel_case_to_underscores(tservice->get_name()) + "_";
  const string cpp_handler = nspace_ + "::" + tservice->get_name() + "Handler";

  indent(f_source) << cpp_handler << "* g_handler = NULL;" << endl << endl;

  indent(f_source) << "void init_service_" << get_init_service_name(tservice) << endl;
  open_scope(f_source);
  indent(f_source) << "if (!g_handler) g_handler = new " << cpp_handler << "();" << endl;
  close_scope(f_source) << endl << endl;

  indent(f_source) << "void close_service_" << get_init_service_name(tservice) << endl;
  open_scope(f_source);
  indent(f_source) << "delete g_handler; g_handler = NULL;" << endl;
  close_scope(f_source) << endl << endl;

  auto functions = tservice->get_functions();
  for (size_t i = 0; i < functions.size(); ++i) {
    t_function* function = functions[i];
    make_doc(f_source, function, i != 0);
    string function_sig = get_c_function_signature(function, service_name, false);

    indent(f_source) << function_sig << endl;
    open_scope(f_source);
    indent(f_source) << "assert(g_handler && \"init_service_" << get_init_service_name(tservice)
                     << " has not been called.\");" << endl
                     << endl;

    auto args = function->get_arglist()->get_sorted_members();
    std::deque<string> arg_array;

    for (auto arg : args) {
      const string c_name = arg->get_name();
      const string cpp_name = "cpp_" + c_name;

      indent(f_source) << get_cpp_type_name(arg->get_type()) << " " << cpp_name << ";" << endl;

      convert_c_type_to_cpp(arg->get_type(), c_name, cpp_name);
      arg_array.push_back(cpp_name);
    }

    t_type* return_type = function->get_returntype();
    bool result_need_conversion = false;
    string result;
    if (return_type != nullptr) {
      if (is_base_type(return_type) && !return_type->is_string()) {
        string typestr = get_c_type_name(return_type);
        result = typestr + " result = (" + typestr + ")";
      } else {
        result_need_conversion = true;
        arg_array.push_front("cpp_result");

        indent(f_source) << get_cpp_type_name(return_type) << " cpp_result;" << endl;
      }
    }

    string fn_call = result + "g_handler->" + function->get_name() + "(";
    indent(f_source) << fn_call;

    for (size_t j = 0; j < arg_array.size(); ++j) {
      if (j != 0) {
        f_source << "," << endl;
        align_fn_params(f_source, fn_call.size());
      }
      f_source << arg_array[j];
    }
    f_source << ");" << endl;

    if (result_need_conversion) {
      f_source << endl;
      indent(f_source) << get_c_type_name(return_type) << " result;" << endl;
      convert_cpp_type_to_c(return_type, "cpp_result", "result");
    }

    if (return_type) {
      f_source << endl;
      indent(f_source) << "return result;" << endl;
    }

    close_scope(f_source) << endl;
  }

  pop_indent(&source_indent_);
}

void t_c_api_generator::generate_service_csharp(t_service* tservice) {
  const string c_service_name = nspace_lc_ + camel_case_to_underscores(tservice->get_name()) + "_";

  push_indent(csharp_indent_);
  indent(f_csharp) << "namespace " << tservice->get_name() << endl;
  open_scope(f_csharp);

  indent(f_csharp) << "public class DirectClient" << endl;
  open_scope(f_csharp);

  auto enums = tservice->get_program()->get_enums();
  for (size_t i = 0; i < enums.size(); ++i) {
    auto tenum = enums[i];
    indent(f_csharp) << "private enum " << get_c_type_name(tenum) << endl;
    open_scope(f_csharp);
    for (auto value : tenum->get_constants()) {
      indent(f_csharp) << get_c_enum_value_name(tenum, value) << " = " << value->get_value() << ","
                       << endl;
    }
    close_scope(f_csharp) << ";" << endl << endl;
  }

  auto structs = tservice->get_program()->get_structs();
  for (size_t i = 0; i < structs.size(); ++i) {
    auto tstruct = structs[i];
    indent(f_csharp) << "[StructLayout(LayoutKind.Sequential)]" << endl;
    indent(f_csharp) << "private struct " << get_c_struct_name(tstruct) << endl;
    open_scope(f_csharp);
    for (auto member : tstruct->get_sorted_members()) {
      indent(f_csharp) << "public " << get_cs_to_c_type_name(member->get_type()) << " "
                       << member->get_name() << ";" << endl;
    }
    close_scope(f_csharp) << ";" << endl << endl;
  }

  // Init functions
  indent(f_csharp) << get_cs_dll_import(tservice) << endl;
  indent(f_csharp) << "private static extern void init_service_" << get_init_service_name(tservice)
                   << ";" << endl
                   << endl;

  indent(f_csharp) << get_cs_dll_import(tservice) << endl;
  indent(f_csharp) << "private static extern void close_service_" << get_init_service_name(tservice)
                   << ";" << endl
                   << endl;

  auto functions = tservice->get_functions();
  for (auto function : functions) {
    indent(f_csharp) << get_cs_dll_import(tservice) << endl;
    indent(f_csharp) << "private static extern ";

    t_type* return_type = function->get_returntype();

    if (!return_type) {
      f_csharp << "void";
    } else {
      f_csharp << get_cs_to_c_type_name(return_type);
    }

    f_csharp << " " << c_service_name << camel_case_to_underscores(function->get_name()) << "(";
    auto args = function->get_arglist()->get_sorted_members();
    for (size_t j = 0; j < args.size(); ++j) {
      auto arg = args[j];
      if (j != 0) {
        f_csharp << ", ";
      }
      f_csharp << get_cs_to_c_type_name(arg->get_type(), true) << " " << arg->get_name();
    }
    f_csharp << ");" << endl << endl;
  }

  // Ctor / Dtor
  indent(f_csharp) << "public DirectClient()" << endl;
  open_scope(f_csharp);
  indent(f_csharp) << "init_service_" << get_init_service_name(tservice) << ";" << endl;
  close_scope(f_csharp) << endl << endl;

  indent(f_csharp) << "~DirectClient()" << endl;
  open_scope(f_csharp);
  indent(f_csharp) << "close_service_" << get_init_service_name(tservice) << ";" << endl;
  close_scope(f_csharp) << endl << endl;

  for (size_t i = 0; i < functions.size(); ++i) {
    auto function = functions[i];
    string function_name = function->get_name();
    if (cs_keywords_.find(function_name) != cs_keywords_.end()) {
      function_name = "@" + function_name;
    }

    indent(f_csharp) << "public " << get_cs_type_name(function->get_returntype()) << " "
                     << function_name << "(";

    auto args = function->get_arglist()->get_sorted_members();
    for (size_t j = 0; j < args.size(); ++j) {
      if (j != 0) {
        f_csharp << ", ";
      }

      auto arg = args[j];
      f_csharp << get_cs_type_name(arg->get_type()) << " " << arg->get_name();
    }

    f_csharp << ")" << endl;
    open_scope(f_csharp);

    for (auto arg : args) {
      const string cs_name = arg->get_name();
      const string c_name = "c_" + arg->get_name();

      indent(f_csharp) << get_cs_to_c_type_name(arg->get_type()) << " " << c_name << ";" << endl;

      convert_cs_type_to_c(arg->get_type(), cs_name, c_name);

      f_csharp << endl;
    }

    indent(f_csharp) << c_service_name << camel_case_to_underscores(function->get_name()) << "(";
    for (size_t j = 0; j < args.size(); ++j) {
      if (j != 0) {
        f_csharp << ", ";
      }

      f_csharp << "c_" << args[j]->get_name();
    }
    f_csharp << ");" << endl;

    close_scope(f_csharp) << endl;

    if (i != functions.size() - 1) {
      f_csharp << endl;
    }
  }

  close_scope(f_csharp) << endl;
  close_scope(f_csharp) << endl;
  pop_indent(&csharp_indent_);
}

void t_c_api_generator::generate_forward_declaration(t_struct* tstruct) {
  if (first_fw_decl_ && has_typedef_) {
    f_header << endl;
    first_fw_decl_ = false;
  }
  const string name = get_c_struct_name(tstruct);
  indent(f_header) << "typedef struct _" << name << " " << name << ";" << endl;
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

string t_c_api_generator::get_c_function_signature(t_function* tfunction,
                                                   string prefix,
                                                   bool dllexport) {
  const string function_name = prefix + camel_case_to_underscores(tfunction->get_name());
  std::stringstream s_function;

  t_type* return_type = tfunction->get_returntype();
  std::vector<t_field*> params = tfunction->get_arglist()->get_members();

  if (dllexport) {
    s_function << "THRIFT_C_API_DLLEXPORT ";
  }
  s_function << get_c_type_name(return_type) << " " << function_name << "(";

  size_t definition_size = s_function.str().size();

  for (size_t j = 0; j < params.size(); ++j) {
    t_field* param = params[j];

    const string param_type = get_c_type_name(param->get_type());
    const string param_name = param->get_name();

    if (j != 0) {
      s_function << "," << endl;
      align_fn_params(s_function, definition_size);
    }
    s_function << param_type << " " << param_name;
  }

  s_function << ")";

  return s_function.str();
}

string t_c_api_generator::get_init_service_name(t_service* tservice) const {
  return nspace_lc_ + camel_case_to_underscores(tservice->get_name()) + "()";
}

string t_c_api_generator::get_cs_dll_import(t_service* tservice) const {
  return "[DllImport(\"" + tservice->get_name()
         + ".dll\", CallingConvention = CallingConvention.Cdecl)]";
}

void t_c_api_generator::convert_c_type_to_cpp(t_type* type, string c_name, string cpp_name) {
  if (is_base_type(type)) {
    indent(f_source) << cpp_name << " = ";
    if (type->is_string()) {
      f_source << "std::string(" << c_name << ")";
    } else {
      f_source << "(" << get_cpp_type_name(type) << ")" << c_name;
    }
    f_source << ";" << endl;
  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*)type;
    auto consts = tenum->get_constants();

    indent(f_source) << "switch (" << c_name << ") {" << endl;
    indent_up();
    for (auto value : consts) {
      indent(f_source) << "case " << get_c_enum_value_name(tenum, value) << ": " << endl;
      indent_up();
      indent(f_source) << cpp_name << " = " << get_cpp_enum_value_name(tenum, value) << ";" << endl;
      indent(f_source) << "break;" << endl;
      indent_down();
    }
    indent_down();
    indent(f_source) << "}";
    f_source << endl;
  } else if (type->is_container()) {
    assert(false);
  } else if (type->is_struct()) {
    for (auto arg : ((t_struct*)type)->get_sorted_members()) {
      const string c_param_name = c_name + "." + arg->get_name();
      const string cpp_param_name = "cpp_" + dots_to_underscore(c_name) + "_" + arg->get_name();

      f_source << endl;
      indent(f_source) << get_cpp_type_name(arg->get_type()) << " " << cpp_param_name << ";"
                       << endl;
      convert_c_type_to_cpp(arg->get_type(), c_param_name, cpp_param_name);
      indent(f_source) << cpp_name << ".__set_" << arg->get_name() << "(" << cpp_param_name << ");"
                       << endl
                       << endl;
    }
  } else {
    std::cerr << get_c_type_name(type) << endl;
    assert(false);
  }

  f_source << endl;
}

void t_c_api_generator::convert_cpp_type_to_c(t_type* type, string cpp_name, string c_name) {
  if (type->is_string()) {
    // FIXME(CMA): This will leak
    string cname = dots_to_underscore(c_name);
    indent(f_source) << cname << " = (char*)malloc(" << cpp_name << ".size() + 1);" << endl;
    indent(f_source) << "memcpy(" << cname << ", " << cpp_name << ".c_str(), " << cpp_name
                     << ".size());" << endl;
    indent(f_source) << cname << "[" << cpp_name << ".size()] = '\\0';";
  } else if (is_base_type(type)) {
    indent(f_source) << dots_to_underscore(c_name) << " = (" << get_c_type_name(type) << ")"
                     << cpp_name << ";" << endl;
  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*)type;
    auto consts = tenum->get_constants();

    indent(f_source) << "switch (" << cpp_name << ") {" << endl;
    indent_up();
    for (auto value : consts) {
      indent(f_source) << "case " << get_cpp_enum_value_name(tenum, value) << ": " << endl;
      indent_up();
      indent(f_source) << c_name << " = " << get_c_enum_value_name(tenum, value) << ";" << endl;
      indent(f_source) << "break;" << endl;
      indent_down();
    }
    indent_down();
    indent(f_source) << "}";
    f_source << endl;
  } else if (type->is_container()) {
    assert(false);
  } else if (type->is_struct()) {
    for (auto arg : ((t_struct*)type)->get_sorted_members()) {
      const string c_param_name = c_name + "." + arg->get_name();
      const string cpp_param_name = "cpp_" + dots_to_underscore(c_name) + "_" + arg->get_name();

      f_source << endl;
      indent(f_source) << get_c_type_name(arg->get_type()) << " "
                       << dots_to_underscore(c_param_name) << ";" << endl;
      convert_cpp_type_to_c(arg->get_type(), cpp_param_name, c_param_name);
      indent(f_source) << c_param_name << " = " << dots_to_underscore(c_param_name) << ";" << endl;
    }
  } else {
    std::cerr << get_c_type_name(type) << endl;
    assert(false);
  }

  f_source << endl;
}

void t_c_api_generator::convert_cs_type_to_c(t_type* type, string cs_name, string c_name) {
  type = get_underlying_type(type);

  if (is_base_type(type)) {
    indent(f_csharp) << c_name << " = " << cs_name << ";" << endl;
  } else if (type->is_enum()) {
    t_enum* tenum = (t_enum*)type;
    auto consts = tenum->get_constants();

    // Prevent cs compiler from complaining
    indent(f_csharp) << c_name << " = " << get_c_struct_name(tenum) << "."
                     << get_c_enum_value_name(tenum, consts[0]) << ";" << endl;

    indent(f_csharp) << "switch (" << cs_name << ") {" << endl;
    indent_up();
    for (auto value : consts) {
      indent(f_csharp) << "case " << get_cs_enum_value_name(tenum, value) << ":" << endl;
      indent_up();
      indent(f_csharp) << c_name << " = " << get_c_struct_name(tenum) << "."
                       << get_c_enum_value_name(tenum, value) << ";" << endl;
      indent(f_csharp) << "break;" << endl;
      indent_down();
    }
    indent_down();
    indent(f_csharp) << "}";
    f_csharp << endl;
  } else if (type->is_container()) {
    assert(false);
  } else if (type->is_struct()) {
    for (auto arg : ((t_struct*)type)->get_sorted_members()) {
      string cs_arg_name = arg->get_name();
      cs_arg_name[0] = ::toupper(cs_arg_name[0]);

      const string cs_param_name = cs_name + "." + cs_arg_name;
      const string c_param_name = c_name + "." + arg->get_name();

      convert_cs_type_to_c(arg->get_type(), cs_param_name, c_param_name);
    }
    f_csharp << endl;
  } else {
    std::cerr << get_c_type_name(type) << endl;
    assert(false);
  }
}

void t_c_api_generator::convert_c_type_to_cs(t_type* type, string c_name, string cs_name) {
  indent(f_csharp) << endl;
}

void t_c_api_generator::push_indent(int indent) {
  previous_indent_ = indent_count();
  set_indent(indent);
}

void t_c_api_generator::pop_indent(int* indent) {
  *indent = indent_count();
  set_indent(previous_indent_);
}

void t_c_api_generator::set_indent(int indent) {
  if (indent_count() > indent) {
    while (indent_count() > indent) {
      indent_down();
    }
  } else {
    while (indent_count() < indent) {
      indent_up();
    }
  }
}

std::ostream& t_c_api_generator::align_fn_params(std::ostream& stream, size_t length) {
  indent(stream);
  for (size_t i = 0; i < length; ++i) {
    stream << " ";
  }
  return stream;
}

string t_c_api_generator::get_c_type_name(t_type* type) const {
  type = get_underlying_type(type);
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
      return "list<" + get_c_type_name(tlist->get_elem_type()) + ">";
    }
    if (type->is_map()) {
      t_map* tmap = static_cast<t_map*>(type);
      string key_string = get_c_type_name(tmap->get_key_type());
      string val_string = get_c_type_name(tmap->get_val_type());
      return "map<" + key_string + ", " + val_string + ">";
    }
    if (type->is_set()) {
      t_set* tset = static_cast<t_set*>(type);
      return "set<" + get_c_type_name(tset->get_elem_type()) + ">";
    }
  } else {
    return get_c_struct_name(type);
  }

  return "";
}

string t_c_api_generator::get_cpp_type_name(t_type* type) const {
  type = get_underlying_type(type);
  if (type->is_base_type()) {
    t_base_type::t_base base_type = ((t_base_type*)type)->get_base();
    switch (base_type) {
    case t_base_type::TYPE_VOID:
      return "void";
    case t_base_type::TYPE_BOOL:
      return "bool";
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
      return "std::string";
    }
  } else if (type->is_container()) {
    // TODO
    if (type->is_list()) {
      t_list* tlist = static_cast<t_list*>(type);
      return "std::vector<" + get_cpp_type_name(tlist->get_elem_type()) + ">";
    }
    if (type->is_map()) {
      t_map* tmap = static_cast<t_map*>(type);
      string key_string = get_cpp_type_name(tmap->get_key_type());
      string val_string = get_cpp_type_name(tmap->get_val_type());
      return "std::map<" + key_string + ", " + val_string + ">";
    }
    if (type->is_set()) {
      t_set* tset = static_cast<t_set*>(type);
      return "std::set<" + get_cpp_type_name(tset->get_elem_type()) + ">";
    }
  } else if (type->is_enum()) {
    return get_cpp_struct_name(type) + "::type";
  } else {
    return get_cpp_struct_name(type);
  }

  return "";
}

string t_c_api_generator::get_cs_type_name(t_type* type) const {
  type = get_underlying_type(type);
  if (is_base_type(type)) {
    return get_cs_to_c_type_name(type, false);
  }

  return type->get_name();
}

string t_c_api_generator::get_cs_to_c_type_name(t_type* type, bool is_param) const {
  type = get_underlying_type(type);
  if (is_base_type(type)) {
    t_base_type* base_type = (t_base_type*)type;
    switch (base_type->get_base()) {
    case t_base_type::TYPE_VOID:
      return "void";
    case t_base_type::TYPE_BOOL:
      return "bool";
    case t_base_type::TYPE_I8:
      return "sbyte";
    case t_base_type::TYPE_I16:
      return "short";
    case t_base_type::TYPE_I32:
      return "int";
    case t_base_type::TYPE_I64:
      return "long";
    case t_base_type::TYPE_DOUBLE:
      return "double";
    case t_base_type::TYPE_STRING:
      return is_param ? "[MarshalAs(UnmanagedType.LPStr)] string" : "string";
    }
  }

  return get_c_type_name(type);
}

string t_c_api_generator::get_c_struct_name(t_type* type) const {
  return nspace_ + "_" + type->get_name();
}

string t_c_api_generator::get_cpp_struct_name(t_type* type) const {
  return nspace_ + "::" + type->get_name();
}

string t_c_api_generator::get_cs_struct_name(t_type* type) const {
  std::cerr << "get_cs_struct_name() is not implemented" << std::endl;
  return string();
}

string t_c_api_generator::get_c_enum_value_name(t_enum* tenum, t_enum_value* tvalue) const {
  return tenum->get_name() + "_" + tvalue->get_name();
}

string t_c_api_generator::get_cpp_enum_value_name(t_enum* tenum, t_enum_value* tvalue) const {
  return nspace_ + "::" + tenum->get_name() + "::" + tvalue->get_name();
}

string t_c_api_generator::get_cs_enum_value_name(t_enum* tenum, t_enum_value* tvalue) const {
  return tenum->get_name() + "." + tvalue->get_name();
}

void t_c_api_generator::init_csharp_keywords() {
  cs_keywords_.clear();

  // C# keywords
  cs_keywords_.insert("abstract");
  cs_keywords_.insert("as");
  cs_keywords_.insert("base");
  cs_keywords_.insert("bool");
  cs_keywords_.insert("break");
  cs_keywords_.insert("byte");
  cs_keywords_.insert("case");
  cs_keywords_.insert("catch");
  cs_keywords_.insert("char");
  cs_keywords_.insert("checked");
  cs_keywords_.insert("class");
  cs_keywords_.insert("const");
  cs_keywords_.insert("continue");
  cs_keywords_.insert("decimal");
  cs_keywords_.insert("default");
  cs_keywords_.insert("delegate");
  cs_keywords_.insert("do");
  cs_keywords_.insert("double");
  cs_keywords_.insert("else");
  cs_keywords_.insert("enum");
  cs_keywords_.insert("event");
  cs_keywords_.insert("explicit");
  cs_keywords_.insert("extern");
  cs_keywords_.insert("false");
  cs_keywords_.insert("finally");
  cs_keywords_.insert("fixed");
  cs_keywords_.insert("float");
  cs_keywords_.insert("for");
  cs_keywords_.insert("foreach");
  cs_keywords_.insert("goto");
  cs_keywords_.insert("if");
  cs_keywords_.insert("implicit");
  cs_keywords_.insert("in");
  cs_keywords_.insert("int");
  cs_keywords_.insert("interface");
  cs_keywords_.insert("internal");
  cs_keywords_.insert("is");
  cs_keywords_.insert("lock");
  cs_keywords_.insert("long");
  cs_keywords_.insert("namespace");
  cs_keywords_.insert("new");
  cs_keywords_.insert("null");
  cs_keywords_.insert("object");
  cs_keywords_.insert("operator");
  cs_keywords_.insert("out");
  cs_keywords_.insert("override");
  cs_keywords_.insert("params");
  cs_keywords_.insert("private");
  cs_keywords_.insert("protected");
  cs_keywords_.insert("public");
  cs_keywords_.insert("readonly");
  cs_keywords_.insert("ref");
  cs_keywords_.insert("return");
  cs_keywords_.insert("sbyte");
  cs_keywords_.insert("sealed");
  cs_keywords_.insert("short");
  cs_keywords_.insert("sizeof");
  cs_keywords_.insert("stackalloc");
  cs_keywords_.insert("static");
  cs_keywords_.insert("string");
  cs_keywords_.insert("struct");
  cs_keywords_.insert("switch");
  cs_keywords_.insert("this");
  cs_keywords_.insert("throw");
  cs_keywords_.insert("true");
  cs_keywords_.insert("try");
  cs_keywords_.insert("typeof");
  cs_keywords_.insert("uint");
  cs_keywords_.insert("ulong");
  cs_keywords_.insert("unchecked");
  cs_keywords_.insert("unsafe");
  cs_keywords_.insert("ushort");
  cs_keywords_.insert("using");
  cs_keywords_.insert("virtual");
  cs_keywords_.insert("void");
  cs_keywords_.insert("volatile");
  cs_keywords_.insert("while");

  // C# contextual keywords
  cs_keywords_.insert("add");
  cs_keywords_.insert("alias");
  cs_keywords_.insert("ascending");
  cs_keywords_.insert("async");
  cs_keywords_.insert("await");
  cs_keywords_.insert("descending");
  cs_keywords_.insert("dynamic");
  cs_keywords_.insert("from");
  cs_keywords_.insert("get");
  cs_keywords_.insert("global");
  cs_keywords_.insert("group");
  cs_keywords_.insert("into");
  cs_keywords_.insert("join");
  cs_keywords_.insert("let");
  cs_keywords_.insert("orderby");
  cs_keywords_.insert("partial");
  cs_keywords_.insert("remove");
  cs_keywords_.insert("select");
  cs_keywords_.insert("set");
  cs_keywords_.insert("value");
  cs_keywords_.insert("var");
  cs_keywords_.insert("where");
  cs_keywords_.insert("yield");
}

// Helpers
bool is_base_type(t_type* type) {
  t_type* t = get_underlying_type(type);
  return t->is_base_type();
}

t_type* get_underlying_type(t_type* type) {
  t_type* result = type;
  while (result->is_typedef()) {
    result = ((t_typedef*)result)->get_type();
  }
  return result;
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

string dots_to_underscore(string str) {
  string result = str;
  for (auto&& c : result) {
    if (c == '.') {
      c = '_';
    }
  }
  return result;
}

THRIFT_REGISTER_GENERATOR(c_api, "C API for DXT purposes", "")

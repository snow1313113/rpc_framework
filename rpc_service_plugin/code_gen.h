/*
 * * file name: code_gen.h
 * * description: ...
 * * author: snow
 * * create time:2020  7 08
 * */

#ifndef _CODE_GEN_H_
#define _CODE_GEN_H_

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <set>
#include <sstream>
#include <string>

using std::set;
using std::string;
using std::stringstream;
using namespace google::protobuf::compiler;

class RpcServiceCodeGenerator : public CodeGenerator
{
public:
    virtual ~RpcServiceCodeGenerator(){};
    virtual bool Generate(const ::google::protobuf::FileDescriptor* file_, const string& params_,
                          GeneratorContext* context_, string* error_) const
    {
        if (file_->service_count() == 0)
            return true;

        string base_name = BaseName(file_->name());
        std::unique_ptr<::google::protobuf::io::ZeroCopyOutputStream> header_output(
            context_->OpenForInsert(base_name + ".pb.h", "namespace_scope"));
        std::unique_ptr<::google::protobuf::io::ZeroCopyOutputStream> header_include_out_put(
            context_->OpenForInsert(base_name + ".pb.h", "includes"));
        std::unique_ptr<::google::protobuf::io::ZeroCopyOutputStream> source_output(
            context_->OpenForInsert(base_name + ".pb.cc", "namespace_scope"));
        std::unique_ptr<::google::protobuf::io::ZeroCopyOutputStream> source_include_out_put(
            context_->OpenForInsert(base_name + ".pb.cc", "includes"));

        ::google::protobuf::io::CodedOutputStream header_out(header_output.get());
        ::google::protobuf::io::CodedOutputStream header_include_out(header_include_out_put.get());
        ::google::protobuf::io::CodedOutputStream source_out(source_output.get());
        ::google::protobuf::io::CodedOutputStream source_include_out(source_include_out_put.get());

        set<string> need_head_file_;
        CollectHeadFile(file_, need_head_file_);

        header_include_out.WriteString(GetHeaderIncludeStr(params_, need_head_file_));
        header_out.WriteString(GenRpcDecl(file_));

        source_include_out.WriteString(GetSourceIncludeStr(params_, need_head_file_));
        source_out.WriteString(GenRpcImpl(file_));

        return true;
    }

private:
    static string BaseName(const string& str_)
    {
        stringstream input(str_);
        string temp;
        std::getline(input, temp, '.');
        return temp;
    }

    static void CollectHeadFile(const ::google::protobuf::FileDescriptor* file_, set<string>& need_head_file_);

    string GetHeaderIncludeStr(const string& base_path_, const set<string>& need_head_file_) const;
    string GetSourceIncludeStr(const string& base_path_, const set<string>& need_head_file_) const;

    string GenRpcDecl(const ::google::protobuf::FileDescriptor* file_) const;
    string GenRpcImpl(const ::google::protobuf::FileDescriptor* file_) const;

    void GenServiceDecl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_,
                        const string& name_prefix_) const;
    void GenServiceImpl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_,
                        const string& name_prefix_) const;

    void GenServiceProtectedMethodDecl(stringstream& ss_, const ::google::protobuf::MethodDescriptor* method_,
                                       bool is_async_, const string& name_prefix_) const;
    void GenServicePrivateMethodDecl(stringstream& ss_, const ::google::protobuf::MethodDescriptor* method_) const;
    void GenServiceMethodImpl(stringstream& ss_, const string& class_name_,
                              const ::google::protobuf::MethodDescriptor* method_, bool is_async_,
                              const string& name_prefix_) const;

    void GenStubDecl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_) const;
    void GenStubImpl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_,
                     const string& name_prefix_) const;

    void GenStubMethodDecl(stringstream& ss_, const ::google::protobuf::MethodDescriptor* method_) const;
    void GenStubAsyncMethodDecl(stringstream& ss_, const ::google::protobuf::MethodDescriptor* method_) const;
    void GenStubMethodImpl(stringstream& ss_, const string& class_name_,
                           const ::google::protobuf::MethodDescriptor* method_, const string& name_prefix_) const;
    void GenStubAsyncMethodImpl(stringstream& ss_, const string& class_name_,
                                const ::google::protobuf::MethodDescriptor* method_, const string& name_prefix_) const;
};

#endif

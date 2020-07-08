/*
 * * file name: code_gen.cpp
 * * description: ...
 * * author: snow
 * * create time:2020  7 08
 * */

#include "code_gen.h"
#include "rpc_options.pb.h"

static const string g_indent = "    ";

void RpcServiceCodeGenerator::CollectHeadFile(const ::google::protobuf::FileDescriptor* file_,
                                              set<string>& need_head_file_)
{
    for (int i = 0; i < file_->service_count(); ++i)
    {
        const ::google::protobuf::ServiceDescriptor* service = file_->service(i);
        string name_prefix = service->options().GetExtension(all_service_prefix);
        if (!name_prefix.empty())
        {
            std::transform(name_prefix.begin(), name_prefix.end(), name_prefix.begin(),
                           [](auto&& c) { return std::tolower(c, std::locale()); });
            need_head_file_.insert(name_prefix);
        }

        for (int i = 0; i < service->method_count(); ++i)
        {
            const ::google::protobuf::MethodDescriptor* method = service->method(i);
            string class_name_prefix = method->options().GetExtension(service_prefix);
            if (!class_name_prefix.empty())
            {
                std::transform(class_name_prefix.begin(), class_name_prefix.end(), class_name_prefix.begin(),
                               [](auto&& c) { return std::tolower(c, std::locale()); });
                need_head_file_.insert(class_name_prefix);
            }
        }
    }
}

string RpcServiceCodeGenerator::GetHeaderIncludeStr(const string& base_path_, const set<string>& need_head_file_) const
{
    string prefix_str = "#include \"";
    prefix_str += base_path_;
    prefix_str += "/";

    string include_str;
    std::for_each(need_head_file_.begin(), need_head_file_.end(), [&](auto&& str) {
        include_str += prefix_str;
        include_str += str;
        include_str += "_context.h\"\n";
    });
    return include_str;
}

string RpcServiceCodeGenerator::GetSourceIncludeStr(const string& base_path_, const set<string>& need_head_file_) const
{
    string prefix_str = "#include \"";
    prefix_str += base_path_;
    prefix_str += "/";

    string include_str;
    std::for_each(need_head_file_.begin(), need_head_file_.end(), [&](auto&& str) {
        include_str += prefix_str;
        include_str += str;
        include_str += "_service.h\"\n";
    });
    return include_str;
}

string RpcServiceCodeGenerator::GenRpcDecl(const ::google::protobuf::FileDescriptor* file_) const
{
    stringstream ss;
    for (int i = 0; i < file_->service_count(); ++i)
    {
        string name_prefix = file_->service(i)->options().GetExtension(all_service_prefix);
        GenServiceDecl(ss, file_->service(i), name_prefix);
        ss << "\n";
        GenStubDecl(ss, file_->service(i));
        ss << "\n";
    }

    return ss.str();
}

string RpcServiceCodeGenerator::GenRpcImpl(const ::google::protobuf::FileDescriptor* file_) const
{
    stringstream ss;
    for (int i = 0; i < file_->service_count(); ++i)
    {
        string name_prefix = file_->service(i)->options().GetExtension(all_service_prefix);
        GenServiceImpl(ss, file_->service(i), name_prefix);
        ss << "\n";
        GenStubImpl(ss, file_->service(i), name_prefix);
        ss << "\n";
    }

    return ss.str();
}

void RpcServiceCodeGenerator::GenServiceDecl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_,
                                             const string& name_prefix_) const
{
    // 同步的base类
    ss_ << "class " << service_->name() << "Base : public " << service_->name() << "\n"
        << "{\n"
        << "protected:\n";
    for (int i = 0; i < service_->method_count(); ++i)
    {
        string class_name_prefix = service_->method(i)->options().GetExtension(service_prefix);
        if (class_name_prefix.empty())
            GenServiceProtectedMethodDecl(ss_, service_->method(i), false, name_prefix_);
        else
            GenServiceProtectedMethodDecl(ss_, service_->method(i), false, class_name_prefix);
    }
    ss_ << "private:\n";
    for (int i = 0; i < service_->method_count(); ++i)
    {
        GenServicePrivateMethodDecl(ss_, service_->method(i));
    }
    ss_ << "};\n";

    ss_ << "\n";

    // 异步的base类
    ss_ << "class " << service_->name() << "AsyncBase : public " << service_->name() << "\n"
        << "{\n"
        << "protected:\n";
    for (int i = 0; i < service_->method_count(); ++i)
    {
        string class_name_prefix = service_->method(i)->options().GetExtension(service_prefix);
        if (class_name_prefix.empty())
            GenServiceProtectedMethodDecl(ss_, service_->method(i), true, name_prefix_);
        else
            GenServiceProtectedMethodDecl(ss_, service_->method(i), true, class_name_prefix);
    }
    ss_ << "private:\n";
    for (int i = 0; i < service_->method_count(); ++i)
    {
        GenServicePrivateMethodDecl(ss_, service_->method(i));
    }
    ss_ << "};\n";
}

void RpcServiceCodeGenerator::GenServiceImpl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_,
                                             const string& name_prefix_) const
{
    // 同步接口的实现
    for (int i = 0; i < service_->method_count(); ++i)
    {
        string class_name_prefix = service_->method(i)->options().GetExtension(service_prefix);
        if (class_name_prefix.empty())
            GenServiceMethodImpl(ss_, service_->name() + "Base", service_->method(i), false, name_prefix_);
        else
            GenServiceMethodImpl(ss_, service_->name() + "Base", service_->method(i), false, class_name_prefix);
        ss_ << "\n";
    }

    // 异步接口的实现
    for (int i = 0; i < service_->method_count(); ++i)
    {
        string class_name_prefix = service_->method(i)->options().GetExtension(service_prefix);
        if (class_name_prefix.empty())
            GenServiceMethodImpl(ss_, service_->name() + "AsyncBase", service_->method(i), true, name_prefix_);
        else
            GenServiceMethodImpl(ss_, service_->name() + "AsyncBase", service_->method(i), true, class_name_prefix);
        ss_ << "\n";
    }
}

void RpcServiceCodeGenerator::GenServiceProtectedMethodDecl(stringstream& ss_,
                                                            const ::google::protobuf::MethodDescriptor* method_,
                                                            bool is_async_, const string& name_prefix_) const
{
    ss_ << g_indent << "virtual int32_t " << method_->name() << "(const " << method_->input_type()->name()
        << "& request, ";

    if (is_async_)
    {
        ss_ << method_->output_type()->name() << "& response, ua::" << name_prefix_ << "Context* context) = 0;\n";
    }
    else
    {
        ss_ << method_->output_type()->name() << "& response, const ua::" << name_prefix_ << "Context& context) = 0;\n";
    }
}

void RpcServiceCodeGenerator::GenServicePrivateMethodDecl(stringstream& ss_,
                                                          const ::google::protobuf::MethodDescriptor* method_) const
{
    ss_ << g_indent << "virtual void " << method_->name() << "(::google::protobuf::RpcController* controller,\n"
        << g_indent << g_indent << "const " << method_->input_type()->name() << "* request,\n"
        << g_indent << g_indent << method_->output_type()->name() << "* response,\n"
        << g_indent << g_indent << "::google::protobuf::Closure* done) final;\n";
}

void RpcServiceCodeGenerator::GenServiceMethodImpl(stringstream& ss_, const string& class_name_,
                                                   const ::google::protobuf::MethodDescriptor* method_, bool is_async_,
                                                   const string& name_prefix_) const
{
    ss_ << "void " << class_name_ << "::" << method_->name() << "(::google::protobuf::RpcController* controller,\n"
        << g_indent << "const " << method_->input_type()->name() << "* request,\n"
        << g_indent << method_->output_type()->name() << "* response,\n"
        << g_indent << "::google::protobuf::Closure* done)\n"
        << "{\n"
        << g_indent << "ua::" << name_prefix_ << "Context* context = static_cast<ua::" << name_prefix_
        << "Context*>(controller);\n";

    if (is_async_)
    {
        ss_ << g_indent << "context->ret_code = " << method_->name() << "(*request, *response, context);\n";
    }
    else
    {
        ss_ << g_indent << "context->ret_code = " << method_->name() << "(*request, *response, *context);\n";
    }
    ss_ << "}\n";
}

void RpcServiceCodeGenerator::GenStubDecl(stringstream& ss_,
                                          const ::google::protobuf::ServiceDescriptor* service_) const
{
    ss_ << "class " << service_->name() << "Stub\n"
        << "{\n"
        << "public:\n";
    for (int i = 0; i < service_->method_count(); ++i)
    {
        GenStubMethodDecl(ss_, service_->method(i));
    }
    ss_ << "\n";
    for (int i = 0; i < service_->method_count(); ++i)
    {
        GenStubAsyncMethodDecl(ss_, service_->method(i));
    }
    ss_ << "};\n";
}

void RpcServiceCodeGenerator::GenStubImpl(stringstream& ss_, const ::google::protobuf::ServiceDescriptor* service_,
                                          const string& name_prefix_) const
{
    for (int i = 0; i < service_->method_count(); ++i)
    {
        GenStubMethodImpl(ss_, service_->name() + "Stub", service_->method(i), name_prefix_);
        ss_ << "\n";
    }

    for (int i = 0; i < service_->method_count(); ++i)
    {
        GenStubAsyncMethodImpl(ss_, service_->name() + "Stub", service_->method(i), name_prefix_);
        ss_ << "\n";
    }
}

void RpcServiceCodeGenerator::GenStubMethodDecl(stringstream& ss_,
                                                const ::google::protobuf::MethodDescriptor* method_) const
{
    if (method_->options().GetExtension(broadcast))
    {
        ss_ << g_indent << "static int32_t " << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request);\n";
    }
    else
    {
        ss_ << g_indent << "static int32_t " << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request, " << method_->output_type()->name()
            << "* response = nullptr, uint64_t dest_id = 0);\n";
    }
}

void RpcServiceCodeGenerator::GenStubAsyncMethodDecl(stringstream& ss_,
                                                     const ::google::protobuf::MethodDescriptor* method_) const
{
    if (method_->options().GetExtension(broadcast))
    {
        ss_ << g_indent << "static int32_t Async" << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request);\n";
    }
    else
    {
        ss_ << g_indent << "using " << method_->name() << "CallBack = std::function<int32_t(int32_t, const "
            << method_->output_type()->name() << "&, ua::Context*)>;\n";
        ss_ << g_indent << "static int32_t Async" << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request, " << method_->name()
            << "CallBack callback = nullptr, ua::Context* context = nullptr, uint64_t dest_id = 0);\n";
    }
}

void RpcServiceCodeGenerator::GenStubMethodImpl(stringstream& ss_, const string& class_name_,
                                                const ::google::protobuf::MethodDescriptor* method_,
                                                const string& name_prefix_) const
{
    if (method_->options().GetExtension(broadcast))
    {
        ss_ << "int32_t " << class_name_ << "::" << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request)\n"
            << "{\n"
            << g_indent << "return ua::" << name_prefix_ << "Service::GetInst().Rpc(gid, 0x" << std::hex
            << method_->options().GetExtension(cmd) << ", request"
            << ", nullptr, 0, true";
    }
    else
    {
        ss_ << "int32_t " << class_name_ << "::" << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request, " << method_->output_type()->name()
            << "* response, uint64_t dest_id)\n"
            << "{\n"
            << g_indent << "return ua::" << name_prefix_ << "Service::GetInst().Rpc(gid, 0x" << std::hex
            << method_->options().GetExtension(cmd) << ", request"
            << ", response, dest_id";
    }
    if (method_->options().HasExtension(timeout))
    {
        ss_ << ", " << std::dec << method_->options().GetExtension(timeout);
    }
    ss_ << ");\n"
        << "}\n";
}

void RpcServiceCodeGenerator::GenStubAsyncMethodImpl(stringstream& ss_, const string& class_name_,
                                                     const ::google::protobuf::MethodDescriptor* method_,
                                                     const string& name_prefix_) const
{
    if (method_->options().GetExtension(broadcast))
    {
        ss_ << "int32_t " << class_name_ << "::Async" << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request)\n"
            << "{\n"
            << g_indent << "return ua::" << name_prefix_ << "Service::GetInst().AsyncRpc(gid, 0x" << std::hex
            << method_->options().GetExtension(cmd) << ", request"
            << ", nullptr, nullptr, nullptr, 0, true";
        if (method_->options().HasExtension(timeout))
        {
            ss_ << ", " << std::dec << method_->options().GetExtension(timeout);
        }
        ss_ << ");\n";
    }
    else
    {
        ss_ << "int32_t " << class_name_ << "::Async" << method_->name() << "(uint64_t gid, const "
            << method_->input_type()->name() << "& request, " << method_->name()
            << "CallBack callback, ua::Context* context, uint64_t dest_id)\n"
            << "{\n";

        ss_ << g_indent << "if (callback)\n"
            << g_indent << "{\n"
            << g_indent << g_indent << "auto tmp_rsp = new " << method_->output_type()->name() << ";\n"
            << g_indent << g_indent << "return ua::" << name_prefix_ << "Service::GetInst().AsyncRpc(gid, 0x"
            << std::hex << method_->options().GetExtension(cmd) << ", request, tmp_rsp,\n"
            << g_indent << g_indent << g_indent << "[=](int32_t ret_code) {\n"
            << g_indent << g_indent << g_indent << g_indent << "int32_t ret = callback(ret_code, *tmp_rsp, context);\n"
            << g_indent << g_indent << g_indent << g_indent << "delete tmp_rsp;\n"
            << g_indent << g_indent << g_indent << g_indent << "return ret;\n"
            << g_indent << g_indent << g_indent << g_indent << "}, context, dest_id";
        if (method_->options().HasExtension(timeout))
        {
            ss_ << ", " << std::dec << method_->options().GetExtension(timeout);
        }
        ss_ << ");\n" << g_indent << "}\n";

        ss_ << g_indent << "else\n"
            << g_indent << "{\n"
            << g_indent << g_indent << "return ua::" << name_prefix_ << "Service::GetInst().AsyncRpc(gid, 0x"
            << std::hex << method_->options().GetExtension(cmd) << ", request, nullptr, nullptr, nullptr, dest_id";

        if (method_->options().HasExtension(timeout))
        {
            ss_ << ", " << std::dec << method_->options().GetExtension(timeout);
        }
        ss_ << ");\n" << g_indent << "}\n";
    }
    ss_ << "}\n";
}

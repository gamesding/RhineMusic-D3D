#include "service.hpp"
namespace rhine {
Service::Service(const std::filesystem::path& root){
    SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};HANDLE readInput=nullptr,writeInput=nullptr,readOutput=nullptr,writeOutput=nullptr;
    if(!CreatePipe(&readInput,&writeInput,&attributes,0))throw std::runtime_error("Service input pipe");Handle childInput(readInput);input_.reset(writeInput);
    if(!CreatePipe(&readOutput,&writeOutput,&attributes,0))throw std::runtime_error("Service output pipe");output_.reset(readOutput);Handle childOutput(writeOutput);
    SetHandleInformation(input_.get(),HANDLE_FLAG_INHERIT,0);SetHandleInformation(output_.get(),HANDLE_FLAG_INHERIT,0);
    Handle silence(CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&attributes,OPEN_EXISTING,0,nullptr));
    STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;startup.StartupInfo.hStdInput=childInput.get();startup.StartupInfo.hStdOutput=childOutput.get();startup.StartupInfo.hStdError=silence.get();
    SIZE_T attributeSize=0;InitializeProcThreadAttributeList(nullptr,1,0,&attributeSize);std::vector<unsigned char> attributeMemory(attributeSize);startup.lpAttributeList=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeMemory.data());
    if(!InitializeProcThreadAttributeList(startup.lpAttributeList,1,0,&attributeSize))throw std::runtime_error("Service startup attributes");
    HANDLE inherited[]={childInput.get(),childOutput.get(),silence.get()};if(!UpdateProcThreadAttribute(startup.lpAttributeList,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr)){DeleteProcThreadAttributeList(startup.lpAttributeList);throw std::runtime_error("Service handle inheritance");}
    auto binary=root/L"runtime/node/node.exe",entry=root/L"backend/entry.mjs";std::wstring command=L"\""+binary.wstring()+L"\" \""+entry.wstring()+L"\"";
    PROCESS_INFORMATION process{};BOOL created=CreateProcessW(binary.c_str(),command.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT|CREATE_NO_WINDOW|CREATE_SUSPENDED,nullptr,root.c_str(),&startup.StartupInfo,&process);
    DeleteProcThreadAttributeList(startup.lpAttributeList);if(!created)throw std::runtime_error("Start audio service: "+systemError(GetLastError()));process_.reset(process.hProcess);Handle thread(process.hThread);
    job_.reset(CreateJobObjectW(nullptr,nullptr));JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!job_||!SetInformationJobObject(job_.get(),JobObjectExtendedLimitInformation,&limits,sizeof(limits))||!AssignProcessToJobObject(job_.get(),process_.get())){TerminateProcess(process_.get(),1);throw std::runtime_error("Service process ownership failed");}
    ResumeThread(thread.get());
    reader_=std::thread([this]{std::string pending;char buffer[16384];DWORD bytes=0;try{while(ReadFile(output_.get(),buffer,sizeof(buffer),&bytes,nullptr)&&bytes){pending.append(buffer,bytes);if(pending.size()>32*1024*1024)throw std::runtime_error("Service event exceeds limit");size_t end;while((end=pending.find('\n'))!=std::string::npos){auto value=json::parse(pending.substr(0,end));pending.erase(0,end+1);std::lock_guard lock(mutex_);events_.push_back(std::move(value));if(events_.size()>4096)throw std::runtime_error("Service event backlog");}}}catch(const std::exception& error){std::lock_guard lock(mutex_);events_.push_back({{"event","error"},{"error",error.what()}});}if(!stopping_){std::lock_guard lock(mutex_);events_.push_back({{"event","error"},{"error","本地音频服务已退出"}});}});
}
Service::~Service(){stopping_=true;try{send("shutdown");}catch(...){}input_.reset();if(process_&&WaitForSingleObject(process_.get(),5000)==WAIT_TIMEOUT)TerminateJobObject(job_.get(),1);job_.reset();if(reader_.joinable())reader_.join();}
unsigned Service::send(std::string operation,json args){json value={{"id",++counter_},{"op",std::move(operation)},{"args",std::move(args)}};const std::string line=value.dump()+'\n';if(line.size()>128*1024)throw std::runtime_error("Native request exceeds limit");size_t offset=0;while(offset<line.size()){DWORD written=0;if(!WriteFile(input_.get(),line.data()+offset,DWORD(line.size()-offset),&written,nullptr)||!written)throw std::runtime_error("Local service is unavailable");offset+=written;}return counter_;}
std::vector<json> Service::drain(){std::lock_guard lock(mutex_);std::vector<json> values;values.reserve(events_.size());while(!events_.empty()){values.push_back(std::move(events_.front()));events_.pop_front();}return values;}
}

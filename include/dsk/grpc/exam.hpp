std::unique_ptr<ClientAsyncResponseReader<     Res>>        AsyncUnary(ClientContext*, Req const&, CompletionQueue*);
std::unique_ptr<        ClientAsyncWriter<Req     >> AsyncClientStream(ClientContext*, Res*,       CompletionQueue*, void* tag);
std::unique_ptr<        ClientAsyncReader<     Res>> AsyncServerStream(ClientContext*, Req const&, CompletionQueue*, void* tag);
std::unique_ptr<  ClientAsyncReaderWriter<Req, Res>>   AsyncBidiStream(ClientContext*,             CompletionQueue*, void* tag);


void               RequestUnary(ServerContext*, Req*, ServerAsyncResponseWriter<Res     >*, CompletionQueue*, ServerCompletionQueue*, void *tag);
void        RequestClientStream(ServerContext*,       ServerAsyncReader        <Res, Req>*, CompletionQueue*, ServerCompletionQueue*, void *tag);
void        RequestServerStream(ServerContext*, Req*, ServerAsyncWriter        <Res     >*, CompletionQueue*, ServerCompletionQueue*, void *tag);
void RequestBidirectionalStream(ServerContext*,       ServerAsyncReaderWriter  <Res, Req>*, CompletionQueue*, ServerCompletionQueue*, void *tag);


              RequestUnary->ServerAsyncResponseWriter::Finish(const W& msg, const grpc::Status& status, void* tag)
       RequestClientStream->        ServerAsyncReader::Finish(const W& msg, const grpc::Status& status, void* tag)
       RequestServerStream->        ServerAsyncWriter::Finish(const grpc::Status& status, void* tag)
RequestBidirectionalStream->  ServerAsyncReaderWriter::Finish(const grpc::Status& status, void* tag)

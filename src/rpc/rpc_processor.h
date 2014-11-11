#ifndef RPC_PROCESSOR_H_
#define RPC_PROCESSOR_H_

#include "io/protocol.h"

namespace rpc {

class RpcProcessor : public io::Protocol::Processor {
  public:
    class Delegate {
      public:
        virtual ~Delegate() {
        }

        virtual void process(io::Connection* conn, io::InputBuf* input_buf,
                             const TimeStamp& time_stamp) = 0;
    };

    // response_handler used to dispatch response message.
    // request_handler used to dispatch request message.
    // request_handler and response_handler held by RpcProcessor.
    explicit RpcProcessor(Delegate* request_handler, Delegate* response_handler)
        : request_delegate_(request_handler), reply_delegate_(response_handler) {
      DCHECK_NOTNULL(request_handler);
      DCHECK_NOTNULL(response_handler);
    }
    virtual ~RpcProcessor() {
    }

  private:
    scoped_ptr<Delegate> request_delegate_;
    scoped_ptr<Delegate> reply_delegate_;

    virtual void dispatch(io::Connection* conn, io::InputBuf* input_buf,
                          const TimeStamp& time_stamp);

    DISALLOW_COPY_AND_ASSIGN(RpcProcessor);
};

}

#endif /* RPC_PROCESSOR_H_ */
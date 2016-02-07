#ifndef SRC_INSPECTOR_AGENT_H_
#define SRC_INSPECTOR_AGENT_H_

#if !HAVE_INSPECTOR
#   error("This header can only be used when inspector is enabled")
#endif

#include "inspector_socket.h"
#include "uv.h"
#include "v8.h"
#include "util.h"

#include <vector>

namespace blink {
class V8Inspector;
namespace protocol {
  class String16;
}
}

// Forward declaration to break recursive dependency chain with src/env.h.
namespace node {
class Environment;
}  // namespace node

namespace node {
namespace inspector {

class ChannelImpl;

class Agent {
 public:
  explicit Agent(node::Environment* env);
  ~Agent();

  // Start the inspector agent thread
  void Start(v8::Platform* platform, int port, bool wait);
  // Stop the inspector agent
  void Stop();

  bool IsStarted();
  bool connected() {  return connected_; }
  void WaitForDisconnect();

 protected:
  static void ThreadCbIO(Agent* agent);
  static void OnSocketConnectionIO(uv_stream_t* server, int status);
  static bool OnInspectorHandshakeIO(inspector_socket_t* socket,
                                     enum inspector_handshake_event state,
                                     const char* path);
  static void OnRemoteDataIO(uv_stream_t* stream, ssize_t read,
      const uv_buf_t* b);
  static void WriteCbIO(uv_async_t* async);

  void WorkerRunIO();
  void OnInspectorConnectionIO(inspector_socket_t* socket);

  void PostMessages();
  void SetConnected(bool connected);
  void Write(const blink::protocol::String16& message);

  uv_sem_t start_sem_;
  uv_cond_t pause_cond_;
  uv_mutex_t queue_lock_;
  uv_mutex_t pause_lock_;
  uv_thread_t thread_;
  uv_loop_t child_loop_;
  uv_tcp_t server_;

  int port_;
  bool wait_;
  bool connected_;
  bool shutting_down_;
  node::Environment* parent_env_;

  uv_async_t data_written_;
  inspector_socket_t* client_socket_;
  blink::V8Inspector* inspector_;
  v8::Platform* platform_;
  std::vector<blink::protocol::String16> message_queue_;
  bool dispatching_messages_;

  friend class AsyncWriteRequest;
  friend class ChannelImpl;
  friend class DispatchOnInspectorBackendTask;
  friend class SetConnectedTask;
  friend class V8NodeInspector;
  friend void InterruptCallback(v8::Isolate*, void* agent);
};

}  // namespace inspector
}  // namespace node

#endif  // SRC_INSPECTOR_AGENT_H_

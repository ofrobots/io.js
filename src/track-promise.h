#ifndef SRC_TRACK_PROMISE_H_
#define SRC_TRACK_PROMISE_H_

#include "v8.h"

namespace node {

class Environment;

class TrackPromise {
 public:
  static TrackPromise* New(v8::Isolate* isolate,
                           v8::Local<v8::Object> promise,
                           v8::Local<v8::Value> reason);

  static inline void WeakCallback(
      const v8::WeakCallbackInfo<TrackPromise>& data);

 private:
  TrackPromise(v8::Isolate* isolate, v8::Local<v8::Object> promise,
               v8::Local<v8::Value> reason);
  ~TrackPromise();

  v8::Persistent<v8::Object> promise_;
  v8::Persistent<v8::Value> reason_;
};

}  // namespace node

#endif  // SRC_TRACK_PROMISE_H_

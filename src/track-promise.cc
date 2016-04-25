#include "track-promise.h"
#include "env.h"
#include "env-inl.h"
#include "node_internals.h"

namespace node {

using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::Value;
using v8::WeakCallbackInfo;


TrackPromise* TrackPromise::New(Isolate* isolate,
                                Local<Object> promise, Local<Value> reason) {
  return new TrackPromise(isolate, promise, reason);
}


TrackPromise::TrackPromise(Isolate* isolate,
                           Local<Object> promise, Local<Value> reason)
    : promise_(isolate, promise), reason_(isolate, reason) {
  promise_.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
  promise_.MarkIndependent();
}


TrackPromise::~TrackPromise() {
  promise_.Reset();
  reason_.Reset();
}


void TrackPromise::WeakCallback(
    const WeakCallbackInfo<TrackPromise>& data) {
  TrackPromise* self = data.GetParameter();
  node::ReportPromiseRejection(data.GetIsolate(),
    PersistentToLocal(data.GetIsolate(), self->reason_));
  delete self;
}


}  // namespace node

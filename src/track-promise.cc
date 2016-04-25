#include "track-promise.h"
#include "env.h"
#include "env-inl.h"
#include "node_internals.h"

namespace node {

using v8::Function;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Persistent;
using v8::Value;
using v8::WeakCallbackData;

typedef void (*FreeCallback)(Local<Object> object, Local<Function> fn);


TrackPromise* TrackPromise::New(Isolate* isolate,
                                Local<Object> object) {
  return new TrackPromise(isolate, object);
}


Persistent<Object>* TrackPromise::persistent() {
  return &persistent_;
}


TrackPromise::TrackPromise(Isolate* isolate,
                           Local<Object> object)
    : persistent_(isolate, object) {
  persistent_.SetWeak(this, WeakCallback);
  persistent_.MarkIndependent();
}


TrackPromise::~TrackPromise() {
  persistent_.Reset();
}


void TrackPromise::WeakCallback(
    const WeakCallbackData<Object, TrackPromise>& data) {
  data.GetParameter()->WeakCallback(data.GetIsolate(), data.GetValue());
}


void TrackPromise::WeakCallback(Isolate* isolate, Local<Object> object) {
  node::ReportPromiseRejection(isolate, object.As<Value>());
  exit(1);
  delete this;
}

}  // namespace node

// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_DEBUG_DEBUG_INTERFACE_H_
#define V8_DEBUG_DEBUG_INTERFACE_H_

#include <functional>

#include "include/v8-debug.h"
#include "include/v8-util.h"
#include "include/v8.h"

#include "src/debug/interface-types.h"

namespace v8 {
namespace debug {

/**
 * Debugger is running in its own context which is entered while debugger
 * messages are being dispatched. This is an explicit getter for this
 * debugger context. Note that the content of the debugger context is subject
 * to change. The Context exists only when the debugger is active, i.e. at
 * least one DebugEventListener or MessageHandler is set.
 */
Local<Context> GetDebugContext(Isolate* isolate);

/**
 * Run a JavaScript function in the debugger.
 * \param fun the function to call
 * \param data passed as second argument to the function
 * With this call the debugger is entered and the function specified is called
 * with the execution state as the first argument. This makes it possible to
 * get access to information otherwise not available during normal JavaScript
 * execution e.g. details on stack frames. Receiver of the function call will
 * be the debugger context global object, however this is a subject to change.
 * The following example shows a JavaScript function which when passed to
 * v8::Debug::Call will return the current line of JavaScript execution.
 *
 * \code
 *   function frame_source_line(exec_state) {
 *     return exec_state.frame(0).sourceLine();
 *   }
 * \endcode
 */
// TODO(dcarney): data arg should be a MaybeLocal
MaybeLocal<Value> Call(Local<Context> context, v8::Local<v8::Function> fun,
                       Local<Value> data = Local<Value>());

/**
 * Enable/disable LiveEdit functionality for the given Isolate
 * (default Isolate if not provided). V8 will abort if LiveEdit is
 * unexpectedly used. LiveEdit is enabled by default.
 */
void SetLiveEditEnabled(Isolate* isolate, bool enable);

// Schedule a debugger break to happen when JavaScript code is run
// in the given isolate.
void DebugBreak(Isolate* isolate);

// Remove scheduled debugger break in given isolate if it has not
// happened yet.
void CancelDebugBreak(Isolate* isolate);

/**
 * Returns array of internal properties specific to the value type. Result has
 * the following format: [<name>, <value>,...,<name>, <value>]. Result array
 * will be allocated in the current context.
 */
MaybeLocal<Array> GetInternalProperties(Isolate* isolate, Local<Value> value);

enum ExceptionBreakState {
  NoBreakOnException = 0,
  BreakOnUncaughtException = 1,
  BreakOnAnyException = 2
};

/**
 * Defines if VM will pause on exceptions or not.
 * If BreakOnAnyExceptions is set then VM will pause on caught and uncaught
 * exception, if BreakOnUncaughtException is set then VM will pause only on
 * uncaught exception, otherwise VM won't stop on any exception.
 */
void ChangeBreakOnException(Isolate* isolate, ExceptionBreakState state);

enum StepAction {
  StepOut = 0,   // Step out of the current function.
  StepNext = 1,  // Step to the next statement in the current function.
  StepIn = 2,    // Step into new functions invoked or the next statement
                 // in the current function.
  StepFrame = 3  // Step into a new frame or return to previous frame.
};

void PrepareStep(Isolate* isolate, StepAction action);
void ClearStepping(Isolate* isolate);

/**
 * Out-of-memory callback function.
 * The function is invoked when the heap size is close to the hard limit.
 *
 * \param data the parameter provided during callback installation.
 */
typedef void (*OutOfMemoryCallback)(void* data);
void SetOutOfMemoryCallback(Isolate* isolate, OutOfMemoryCallback callback,
                            void* data);

/**
 * Native wrapper around v8::internal::Script object.
 */
class Script {
 public:
  v8::Isolate* GetIsolate() const;

  ScriptOriginOptions OriginOptions() const;
  bool WasCompiled() const;
  int Id() const;
  int LineOffset() const;
  int ColumnOffset() const;
  std::vector<int> LineEnds() const;
  MaybeLocal<String> Name() const;
  MaybeLocal<String> SourceURL() const;
  MaybeLocal<String> SourceMappingURL() const;
  MaybeLocal<Value> ContextData() const;
  MaybeLocal<String> Source() const;
  bool IsWasm() const;
  bool GetPossibleBreakpoints(const debug::Location& start,
                              const debug::Location& end,
                              std::vector<debug::Location>* locations) const;

 private:
  int GetSourcePosition(const debug::Location& location) const;
};

// Specialization for wasm Scripts.
class WasmScript : public Script {
 public:
  static WasmScript* Cast(Script* script);

  int NumFunctions() const;
  int NumImportedFunctions() const;

  debug::WasmDisassembly DisassembleFunction(int function_index) const;
};

void GetLoadedScripts(Isolate* isolate, PersistentValueVector<Script>& scripts);

MaybeLocal<UnboundScript> CompileInspectorScript(Isolate* isolate,
                                                 Local<String> source);

class DebugEventListener {
 public:
  virtual ~DebugEventListener() {}
  virtual void PromiseEventOccurred(debug::PromiseDebugActionType type,
                                    int id) {}
  virtual void ScriptCompiled(v8::Local<Script> script,
                              bool has_compile_error) {}
  virtual void BreakProgramRequested(v8::Local<v8::Context> paused_context,
                                     v8::Local<v8::Object> exec_state,
                                     v8::Local<v8::Value> break_points_hit) {}
  virtual void ExceptionThrown(v8::Local<v8::Context> paused_context,
                               v8::Local<v8::Object> exec_state,
                               v8::Local<v8::Value> exception,
                               bool is_promise_rejection, bool is_uncaught) {}
};

void SetDebugEventListener(Isolate* isolate, DebugEventListener* listener);

}  // namespace debug
}  // namespace v8

#endif  // V8_DEBUG_DEBUG_INTERFACE_H_

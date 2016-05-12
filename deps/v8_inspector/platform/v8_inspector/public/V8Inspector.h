// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8Inspector_h
#define V8Inspector_h

#include "platform/v8_inspector/public/V8DebuggerClient.h"
#include "platform/v8_inspector/public/V8InspectorSession.h"

#include "wtf/OwnPtr.h"
#include <v8.h>

namespace blink {

namespace protocol {
class Dispatcher;
class Frontend;
class FrontendChannel;
}

class V8Debugger;
class V8DebuggerAgent;
class V8HeapProfilerAgent;
class V8ProfilerAgent;

class V8Inspector : public V8DebuggerClient {
    WTF_MAKE_NONCOPYABLE(V8Inspector);
public:
    V8Inspector(v8::Isolate*, v8::Local<v8::Context>);

    void eventListeners(v8::Local<v8::Value>, V8EventListenerInfoList&) override;
    bool callingContextCanAccessContext(v8::Local<v8::Context> calling, v8::Local<v8::Context> target) override;
    String16 valueSubtype(v8::Local<v8::Value>) override;
    bool formatAccessorsAsProperties(v8::Local<v8::Value>) override;
    void muteWarningsAndDeprecations() override { }
    void unmuteWarningsAndDeprecations() override { }
    double currentTimeMS() override { return 0; };

    void muteConsole() override;
    void unmuteConsole() override;
    bool isExecutionAllowed() override;
    int ensureDefaultContextInGroup(int contextGroupId) override;

    ~V8Inspector();

    // Transport interface.
    void connectFrontend(protocol::FrontendChannel*);
    void disconnectFrontend();
    void dispatchMessageFromFrontend(const String16& message);

private:
    OwnPtr<V8Debugger> m_debugger;
    OwnPtr<V8InspectorSession> m_session;
    OwnPtr<protocol::Dispatcher> m_dispatcher;
    OwnPtr<protocol::Frontend> m_frontend;
};

}

#endif // V8Inspector_h

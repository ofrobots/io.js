// Copyright 2013 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/libplatform/default-platform.h"

#include <algorithm>
#include <queue>

#include "src/base/logging.h"
#include "src/base/platform/platform.h"
#include "src/base/platform/time.h"
#include "src/base/sys-info.h"
#include "src/libplatform/worker-thread.h"

namespace v8 {
namespace platform {

#define MAX_CATEGORY_GROUPS 100
static const char* g_category_groups[MAX_CATEGORY_GROUPS] = {0};
static uint8_t g_category_group_enabled[MAX_CATEGORY_GROUPS] = {0};
static base::Atomic32 g_category_index = 0;
static base::LazyMutex g_category_group_mutex = LAZY_MUTEX_INITIALIZER;


v8::Platform* CreateDefaultPlatform(int thread_pool_size) {
  DefaultPlatform* platform = new DefaultPlatform();
  platform->SetThreadPoolSize(thread_pool_size);
  platform->EnsureInitialized();
  return platform;
}


bool PumpMessageLoop(v8::Platform* platform, v8::Isolate* isolate) {
  return reinterpret_cast<DefaultPlatform*>(platform)->PumpMessageLoop(isolate);
}


const int DefaultPlatform::kMaxThreadPoolSize = 4;


class TraceBuffer {
public:
  static const size_t kTraceBufferSize = 1 << 12; // for now

  TraceBuffer();
  TraceEvent* AddTraceEvent(size_t* event_index) {
    DCHECK(!IsFull());
    *event_index = next_free_++;
    return &trace_events[*event_index];
  }

  bool IsFull() const { return next_free_ == kTraceBufferSize; }

private:
  size_t next_free_;
  std::array<TraceEvent, kTraceBufferSize> trace_events_;
};


class TraceConfig {
public:
  TraceConfig(const std::string& category_filter_string);

  TraceRecordMode GetTraceRecordMode() const { return record_mode_; }
  bool IsCategoryGroupEnabled(const char* category_group) const;

private:
  std::vector<std::string> included_categories_;
  std::vector<std::string> disabled_categories_;
  std::vector<std::string> excluded_categories_;

  DISALLOW_COPY_AND_ASSIGN(TraceConfig);
};

std::vector<std::string> SplitString(const std::string& s, char delim) {
  std::vector<std::string> elements;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elements.push_back(item);
  }
  return elements;
}

TraceConfig::TraceConfig(const std::string& category_filter_string,
                        TraceRecordMode record_mode)
  : record_mode_(record_mode) {
  if (!category_filter_string.empty()) {
    std::vector<std:string> split = SplitString(category_filter_string, ",");
    for (auto category : split) {
      // Ignore empty categories
      if (category.empty())
        continue;
      if (category.at(0) == "-") {
        // Excluded categories start with '-'.
        // Remove '-' from category string.
        category = category.substr(1);
        excluded_categories_.push_back(category);
      } else if (category.compare(0, strlen(TRACE_DISABLED_BY_DEFAULT("")),
                                  TRACE_DISABLED_BY_DEFAULT("")) == 0) {
        // Disabled by default categories.
        disabled_categories_.push_back(category);
      } else {
        included_categories_.push_back(category);
      }
    }
  }
}


bool TraceConfig::IsCategoryGroupEnabled(const std::string& category_group) const {
  return true; // for  now.
}

class TraceLog {
public:
  enum CategoryGroupEnabledFlags {
    ENABLED_FOR_RECORDING = 1 << 0,
    ENABLED_FOR_MONITORING = 1 << 1,
    ENABLED_FOR_EVENT_CALLBACK = 1 << 2
  };

  static TraceLog* GetInstance();

  static const unsigned char* GetCategoryGroupEnabled(const char* name);
  static const char* GetCategoryGroupName(
      const unsigned char* category_group_enabled);
  uint64_t AddTraceEvent(char phase, const uint8_t* category_enabled_flag,
                         const char* name, uint64_t id, uint64_t bind_id,
                         int32_t num_args, const char** arg_names,
                         const uint8_t* arg_types, const uint64_t* arg_values,
                         unsigned int flags);
  void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
                                const char* name, uint64_t handle);

private:
  static TraceLog* instance_;

  TraceConfig trace_config;

  DISALLOW_COPY_AND_ASSIGN(TraceLog);
};

// static
TraceLog* TraceLog::instance_ = nullptr;

// static
TraceLog* TraceLog::GetInstance() {
  return instance_ ? instance_ :
    (instance_ = new TraceLog()); // Leak.
}

TraceEventHandle TraceLog::AddTraceEvent(
    char phase,
    const unsigned char* category_group_enabled,
    const char* name,
    unsigned long long id,
    int num_args,
    const char** arg_names,
    const unsigned char* arg_types,
    const unsigned long long* arg_values,
    const scoped_refptr<ConvertableToTraceFormat>* convertable_values,
    unsigned int flags) {
  TraceEventHandle handle = {0, 0, 0};
  if (!*category_group_enabled)
    return handle;

  DCHECK(name);
  // timestamp


  if (*category_group_enabled & (ENABLED_FOR_RECORDING | ENABLED_FOR_MONITORING)) {
    TraceEvent* trace_event = nullptr;
    if (!trace_buffer_.IsFull()) {
      size_t event_index;
      trace_event = trace_buffer_.AddTraceEvent(event_index);
    }

    if (trace_event) {
      trace_event->Initialize(0, // threadid
                              0, // offset_event_timestamp
                              0, // thread_now
                              phase,
                              category_group_enabled,
                              name,
                              id,
                              0, // bind_id
                              num_args,
                              arg_names,
                              arg_types,
                              arg_values,
                              convertable_values,
                              flags);
    }

    // TODO: echo to console.
  }

}


DefaultPlatform::DefaultPlatform()
    : initialized_(false), thread_pool_size_(0) {}


DefaultPlatform::~DefaultPlatform() {
  base::LockGuard<base::Mutex> guard(&lock_);
  queue_.Terminate();
  if (initialized_) {
    for (auto i = thread_pool_.begin(); i != thread_pool_.end(); ++i) {
      delete *i;
    }
  }
  for (auto i = main_thread_queue_.begin(); i != main_thread_queue_.end();
       ++i) {
    while (!i->second.empty()) {
      delete i->second.front();
      i->second.pop();
    }
  }
  for (auto i = main_thread_delayed_queue_.begin();
       i != main_thread_delayed_queue_.end(); ++i) {
    while (!i->second.empty()) {
      delete i->second.top().second;
      i->second.pop();
    }
  }
}


void DefaultPlatform::SetThreadPoolSize(int thread_pool_size) {
  base::LockGuard<base::Mutex> guard(&lock_);
  DCHECK(thread_pool_size >= 0);
  if (thread_pool_size < 1) {
    thread_pool_size = base::SysInfo::NumberOfProcessors();
  }
  thread_pool_size_ =
      std::max(std::min(thread_pool_size, kMaxThreadPoolSize), 1);
}


void DefaultPlatform::EnsureInitialized() {
  base::LockGuard<base::Mutex> guard(&lock_);
  if (initialized_) return;
  initialized_ = true;

  for (int i = 0; i < thread_pool_size_; ++i)
    thread_pool_.push_back(new WorkerThread(&queue_));
}


Task* DefaultPlatform::PopTaskInMainThreadQueue(v8::Isolate* isolate) {
  auto it = main_thread_queue_.find(isolate);
  if (it == main_thread_queue_.end() || it->second.empty()) {
    return NULL;
  }
  Task* task = it->second.front();
  it->second.pop();
  return task;
}


Task* DefaultPlatform::PopTaskInMainThreadDelayedQueue(v8::Isolate* isolate) {
  auto it = main_thread_delayed_queue_.find(isolate);
  if (it == main_thread_delayed_queue_.end() || it->second.empty()) {
    return NULL;
  }
  double now = MonotonicallyIncreasingTime();
  std::pair<double, Task*> deadline_and_task = it->second.top();
  if (deadline_and_task.first > now) {
    return NULL;
  }
  it->second.pop();
  return deadline_and_task.second;
}


bool DefaultPlatform::PumpMessageLoop(v8::Isolate* isolate) {
  Task* task = NULL;
  {
    base::LockGuard<base::Mutex> guard(&lock_);

    // Move delayed tasks that hit their deadline to the main queue.
    task = PopTaskInMainThreadDelayedQueue(isolate);
    while (task != NULL) {
      main_thread_queue_[isolate].push(task);
      task = PopTaskInMainThreadDelayedQueue(isolate);
    }

    task = PopTaskInMainThreadQueue(isolate);

    if (task == NULL) {
      return false;
    }
  }
  task->Run();
  delete task;
  return true;
}


void DefaultPlatform::CallOnBackgroundThread(Task *task,
                                             ExpectedRuntime expected_runtime) {
  EnsureInitialized();
  queue_.Append(task);
}


void DefaultPlatform::CallOnForegroundThread(v8::Isolate* isolate, Task* task) {
  base::LockGuard<base::Mutex> guard(&lock_);
  main_thread_queue_[isolate].push(task);
}


void DefaultPlatform::CallDelayedOnForegroundThread(Isolate* isolate,
                                                    Task* task,
                                                    double delay_in_seconds) {
  base::LockGuard<base::Mutex> guard(&lock_);
  double deadline = MonotonicallyIncreasingTime() + delay_in_seconds;
  main_thread_delayed_queue_[isolate].push(std::make_pair(deadline, task));
}


void DefaultPlatform::CallIdleOnForegroundThread(Isolate* isolate,
                                                 IdleTask* task) {
  UNREACHABLE();
}


bool DefaultPlatform::IdleTasksEnabled(Isolate* isolate) { return false; }


double DefaultPlatform::MonotonicallyIncreasingTime() {
  return base::TimeTicks::HighResolutionNow().ToInternalValue() /
         static_cast<double>(base::Time::kMicrosecondsPerSecond);
}


uint64_t DefaultPlatform::AddTraceEvent(
    char phase, const uint8_t* category_enabled_flag, const char* name,
    uint64_t id, uint64_t bind_id, int num_args, const char** arg_names,
    const uint8_t* arg_types, const uint64_t* arg_values, unsigned int flags) {
  return 0;
}


void DefaultPlatform::UpdateTraceEventDuration(
    const uint8_t* category_enabled_flag, const char* name, uint64_t handle) {}


const uint8_t* DefaultPlatform::GetCategoryGroupEnabled(const char* name) {
  printf("--$$-- category: %s\n", name);
  DCHECK(!strchr(name, '"'));

  // Search through the existing category list. category_groups is append only,
  // so we can avoid acquiring the lock on the fast path.
  size_t current_category_index = base::Acquire_Load(&g_category_index);
  for (size_t i = 0; i < current_category_index; ++i) {
    if (strcmp(g_category_groups[i], name) == 0) {
      return &g_category_group_enabled[i];
    }
  }

  // This is the slow path: the lock is not held in the case above, so more
  // than one thread could have reached here trying to add the same category.
  // Only hold to lock when actually appending a new category, and
  // check the categories groups again.
  // Add the new category group.
  base::LockGuard<base::Mutex> lock_guard(g_category_group_mutex.Pointer());
  current_category_index = base::Acquire_Load(&g_category_index);
  for (size_t i = 0; i < current_category_index; ++i) {
    if (strcmp(g_category_groups[i], name) == 0) {
      return &g_category_group_enabled[i];
    }
  }

  // Create a new category group.
  DCHECK(current_category_index < MAX_CATEGORY_GROUPS);
  if (current_category_index < MAX_CATEGORY_GROUPS) {
    const char* new_group = strdup(name); // NOTE: this leaks.
    g_category_groups[current_category_index] = new_group;
    DCHECK(!g_category_group_enabled[current_category_index]);
    // TODO: UpdateCategoryGroupEnabledFlag
    uint8_t* category_group_enabled = &g_category_group_enabled[current_category_index];
    /* hack */ *g_category_group_enabled = 1;
    // Update the max index now.
    base::Release_Store(&g_category_index, current_category_index + 1);
    return category_group_enabled;
  } else {
    static uint8_t no = 0;
    return &no;
  }
}


const char* DefaultPlatform::GetCategoryGroupName(
    const uint8_t* category_enabled_flag) {
  static const char dummy[] = "dummy";
  return dummy;
}
}  // namespace platform
}  // namespace v8

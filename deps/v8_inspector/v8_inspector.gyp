# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    'v8_inspector.gypi',
  ],

  'targets': [
    {
      'target_name': 'v8_inspector',
      'type': '<(component)',

      'dependencies': [
        'platform/v8_inspector/v8_inspector.gyp:inspector_injected_script',
        'platform/v8_inspector/v8_inspector.gyp:inspector_debugger_script',
        'platform/v8_inspector/v8_inspector.gyp:protocol_sources',
      ],
      'defines': [
        'V8_INSPECTOR_USE_STL=1'
      ],
      'include_dirs': [
        '.',
        '../v8/include',
        '../v8',
        '<(SHARED_INTERMEDIATE_DIR)/blink',
      ],
      'sources': [
        '<@(v8_inspector_files)',
      ],
    },
  ] # end targets
}

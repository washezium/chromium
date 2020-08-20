// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_FUZZER_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_FUZZER_UTILS_H_

#include "third_party/blink/renderer/bindings/core/v8/script_function.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_encoded_video_config.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_video_decoder_init.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/modules/webcodecs/encoded_video_chunk.h"
#include "third_party/blink/renderer/modules/webcodecs/fuzzer_inputs.pb.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include <string>

namespace blink {

class FakeFunction : public ScriptFunction {
 public:
  static FakeFunction* Create(ScriptState* script_state, std::string name);

  explicit FakeFunction(ScriptState* script_state, std::string name);

  v8::Local<v8::Function> Bind();
  ScriptValue Call(ScriptValue) override;

 private:
  const std::string name_;
};

EncodedVideoConfig* MakeDecoderConfig(
    const wc_fuzzer::ConfigureVideoDecoder& proto);

EncodedVideoChunk* MakeEncodedVideoChunk(
    const wc_fuzzer::EncodedVideoChunk& proto);

String ToChunkType(wc_fuzzer::EncodedVideoChunk_EncodedVideoChunkType type);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBCODECS_FUZZER_UTILS_H_

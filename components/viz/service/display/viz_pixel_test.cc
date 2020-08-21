// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/viz_pixel_test.h"

#include "build/build_config.h"
#include "ui/base/ui_base_features.h"

namespace viz {
namespace {

std::vector<RendererType> GetRendererTypes(bool include_software,
                                           bool include_dawn) {
  std::vector<RendererType> types;
  if (include_software)
    types.push_back(RendererType::kSoftware);
  types.push_back(RendererType::kGL);
  types.push_back(RendererType::kSkiaGL);
#if defined(ENABLE_VIZ_VULKAN_TESTS)
  types.push_back(RendererType::kSkiaVulkan);
#endif
#if defined(ENABLE_VIZ_DAWN_TESTS)
  if (include_dawn)
    types.push_back(RendererType::kSkiaDawn);
#endif
  return types;
}

}  // namespace

std::vector<RendererType> GetRendererTypes() {
  return GetRendererTypes(true, true);
}

std::vector<RendererType> GetGpuRendererTypes(bool include_dawn) {
  return GetRendererTypes(false, include_dawn);
}

// static
cc::PixelTest::GraphicsBackend VizPixelTest::RenderTypeToBackend(
    RendererType renderer_type) {
  if (renderer_type == RendererType::kSkiaVulkan) {
#if defined(USE_OZONE) && defined(OS_LINUX) && !defined(OS_CHROMEOS)
    // TODO(https://crbug.com/1113577): Enable SkiaVulkan backend for
    // PixelTests. For example, RendererPixelTest* hadn't been using
    // SkiaVulkanRenderer until USE_X11 was defined for the OS_LINUX
    // configuration that uses USE_OZONE. Thus, given the lack of test
    // coverage, we must fix this test variant so that we do not loose
    // important test coverage when USE_X11 goes away.
    if (!features::IsUsingOzonePlatform())
#endif
    {
      return GraphicsBackend::kSkiaVulkan;
    }
  } else if (renderer_type == RendererType::kSkiaDawn) {
    return GraphicsBackend::kSkiaDawn;
  }

  return GraphicsBackend::kDefault;
}

VizPixelTest::VizPixelTest(RendererType type)
    : PixelTest(RenderTypeToBackend(type)), renderer_type_(type) {}

void VizPixelTest::SetUp() {
  switch (renderer_type_) {
    case RendererType::kSoftware:
      SetUpSoftwareRenderer();
      break;
    case RendererType::kGL:
      SetUpGLRenderer(GetSurfaceOrigin());
      break;
    case RendererType::kSkiaGL:
    case RendererType::kSkiaVulkan:
    case RendererType::kSkiaDawn:
      SetUpSkiaRenderer(GetSurfaceOrigin());
      break;
  }
}

gfx::SurfaceOrigin VizPixelTest::GetSurfaceOrigin() const {
  return gfx::SurfaceOrigin::kBottomLeft;
}

VizPixelTestWithParam::VizPixelTestWithParam() : VizPixelTest(GetParam()) {}

}  // namespace viz

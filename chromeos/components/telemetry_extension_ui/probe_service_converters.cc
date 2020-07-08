// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/telemetry_extension_ui/probe_service_converters.h"

#include <utility>

#include "base/notreached.h"
#include "chromeos/components/telemetry_extension_ui/mojom/probe_service.mojom.h"
#include "chromeos/services/cros_healthd/public/mojom/cros_healthd_probe.mojom.h"

namespace chromeos {
namespace probe_service_converters {

namespace {

cros_healthd::mojom::ProbeCategoryEnum Convert(
    health::mojom::ProbeCategoryEnum input) {
  switch (input) {
    case health::mojom::ProbeCategoryEnum::kBattery:
      return cros_healthd::mojom::ProbeCategoryEnum::kBattery;
    case health::mojom::ProbeCategoryEnum::kNonRemovableBlockDevices:
      return cros_healthd::mojom::ProbeCategoryEnum::kNonRemovableBlockDevices;
    case health::mojom::ProbeCategoryEnum::kCachedVpdData:
      return cros_healthd::mojom::ProbeCategoryEnum::kCachedVpdData;
  }
  NOTREACHED();
}

}  // namespace

health::mojom::ErrorType Convert(cros_healthd::mojom::ErrorType input) {
  switch (input) {
    case cros_healthd::mojom::ErrorType::kFileReadError:
      return health::mojom::ErrorType::kFileReadError;
    case cros_healthd::mojom::ErrorType::kParseError:
      return health::mojom::ErrorType::kParseError;
    case cros_healthd::mojom::ErrorType::kSystemUtilityError:
      return health::mojom::ErrorType::kSystemUtilityError;
  }
  NOTREACHED();
}

health::mojom::ProbeErrorPtr Convert(cros_healthd::mojom::ProbeErrorPtr input) {
  if (!input) {
    return nullptr;
  }
  return health::mojom::ProbeError::New(Convert(input->type),
                                        std::move(input->msg));
}

health::mojom::DoubleValuePtr Convert(double input) {
  return health::mojom::DoubleValue::New(input);
}

health::mojom::Int64ValuePtr Convert(int64_t input) {
  return health::mojom::Int64Value::New(input);
}

health::mojom::UInt32ValuePtr Convert(uint32_t input) {
  return health::mojom::UInt32Value::New(input);
}

health::mojom::UInt64ValuePtr Convert(uint64_t input) {
  return health::mojom::UInt64Value::New(input);
}

health::mojom::UInt64ValuePtr Convert(
    cros_healthd::mojom::UInt64ValuePtr input) {
  if (!input) {
    return nullptr;
  }
  return health::mojom::UInt64Value::New(input->value);
}

health::mojom::BatteryInfoPtr Convert(
    cros_healthd::mojom::BatteryInfoPtr input) {
  if (!input) {
    return nullptr;
  }

  return health::mojom::BatteryInfo::New(
      Convert(input->cycle_count), Convert(input->voltage_now),
      std::move(input->vendor), std::move(input->serial_number),
      Convert(input->charge_full_design), Convert(input->charge_full),
      Convert(input->voltage_min_design), std::move(input->model_name),
      Convert(input->charge_now), Convert(input->current_now),
      std::move(input->technology), std::move(input->status),
      std::move(input->manufacture_date),
      Convert(std::move(input->temperature)));
}

health::mojom::BatteryResultPtr Convert(
    cros_healthd::mojom::BatteryResultPtr input) {
  if (!input) {
    return nullptr;
  }

  switch (input->which()) {
    case cros_healthd::mojom::BatteryResult::Tag::BATTERY_INFO:
      return health::mojom::BatteryResult::NewBatteryInfo(
          Convert(std::move(input->get_battery_info())));
    case cros_healthd::mojom::BatteryResult::Tag::ERROR:
      return health::mojom::BatteryResult::NewError(
          Convert(std::move(input->get_error())));
  }
  NOTREACHED();
}

health::mojom::NonRemovableBlockDeviceInfoPtr Convert(
    cros_healthd::mojom::NonRemovableBlockDeviceInfoPtr input) {
  return health::mojom::NonRemovableBlockDeviceInfo::New(
      std::move(input->path), Convert(input->size), std::move(input->type),
      Convert(static_cast<uint32_t>(input->manufacturer_id)),
      std::move(input->name), Convert(static_cast<uint32_t>(input->serial)),
      Convert(input->bytes_read_since_last_boot),
      Convert(input->bytes_written_since_last_boot),
      Convert(input->read_time_seconds_since_last_boot),
      Convert(input->write_time_seconds_since_last_boot),
      Convert(input->io_time_seconds_since_last_boot),
      Convert(std::move(input->discard_time_seconds_since_last_boot)));
}

health::mojom::NonRemovableBlockDeviceResultPtr Convert(
    cros_healthd::mojom::NonRemovableBlockDeviceResultPtr input) {
  if (!input) {
    return nullptr;
  }

  switch (input->which()) {
    case cros_healthd::mojom::NonRemovableBlockDeviceResult::Tag::
        BLOCK_DEVICE_INFO:
      return health::mojom::NonRemovableBlockDeviceResult::NewBlockDeviceInfo(
          ConvertPtrVector<health::mojom::NonRemovableBlockDeviceInfoPtr>(
              std::move(input->get_block_device_info())));
    case cros_healthd::mojom::NonRemovableBlockDeviceResult::Tag::ERROR:
      return health::mojom::NonRemovableBlockDeviceResult::NewError(
          Convert(std::move(input->get_error())));
  }
  NOTREACHED();
}

health::mojom::CachedVpdInfoPtr Convert(
    cros_healthd::mojom::CachedVpdInfoPtr input) {
  if (!input) {
    return nullptr;
  }

  return health::mojom::CachedVpdInfo::New(std::move(input->sku_number));
}

health::mojom::CachedVpdResultPtr Convert(
    cros_healthd::mojom::CachedVpdResultPtr input) {
  if (!input) {
    return nullptr;
  }

  switch (input->which()) {
    case cros_healthd::mojom::CachedVpdResult::Tag::VPD_INFO:
      return health::mojom::CachedVpdResult::NewVpdInfo(
          Convert(std::move(input->get_vpd_info())));
    case cros_healthd::mojom::CachedVpdResult::Tag::ERROR:
      return health::mojom::CachedVpdResult::NewError(
          Convert(std::move(input->get_error())));
  }
  NOTREACHED();
}

health::mojom::TelemetryInfoPtr Convert(
    cros_healthd::mojom::TelemetryInfoPtr input) {
  if (!input) {
    return nullptr;
  }

  return health::mojom::TelemetryInfo::New(
      Convert(std::move(input->battery_result)),
      Convert(std::move(input->block_device_result)),
      Convert(std::move(input->vpd_result)));
}

std::vector<cros_healthd::mojom::ProbeCategoryEnum> ConvertCategoryVector(
    const std::vector<health::mojom::ProbeCategoryEnum>& input) {
  std::vector<cros_healthd::mojom::ProbeCategoryEnum> output;
  for (const auto element : input) {
    output.push_back(Convert(element));
  }
  return output;
}

}  // namespace probe_service_converters
}  // namespace chromeos

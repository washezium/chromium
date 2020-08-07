// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/nearby_sharing/nearby_sharing_service_impl.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/browser_features.h"
#include "chrome/browser/nearby_sharing/common/nearby_share_prefs.h"
#include "chrome/browser/nearby_sharing/fake_nearby_connection.h"
#include "chrome/browser/nearby_sharing/fake_nearby_connections_manager.h"
#include "chrome/browser/nearby_sharing/fast_initiation_manager.h"
#include "chrome/browser/nearby_sharing/mock_nearby_process_manager.h"
#include "chrome/browser/nearby_sharing/mock_nearby_sharing_decoder.h"
#include "chrome/browser/nearby_sharing/nearby_connections_manager.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/services/sharing/public/proto/wire_format.pb.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/test/mock_bluetooth_adapter.h"
#include "net/base/mock_network_change_notifier.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/idle/scoped_set_idle_state.h"

using ::testing::_;
using testing::NiceMock;
using testing::Return;

using NetConnectionType = net::NetworkChangeNotifier::ConnectionType;

class FakeFastInitiationManager : public FastInitiationManager {
 public:
  explicit FakeFastInitiationManager(
      scoped_refptr<device::BluetoothAdapter> adapter,
      bool should_succeed_on_start,
      base::OnceCallback<void()> on_stop_advertising_callback,
      base::OnceCallback<void()> on_destroy_callback)
      : FastInitiationManager(adapter),
        should_succeed_on_start_(should_succeed_on_start),
        on_stop_advertising_callback_(std::move(on_stop_advertising_callback)),
        on_destroy_callback_(std::move(on_destroy_callback)) {}

  ~FakeFastInitiationManager() override {
    std::move(on_destroy_callback_).Run();
  }

  void StartAdvertising(FastInitType type,
                        base::OnceCallback<void()> callback,
                        base::OnceCallback<void()> error_callback) override {
    ++start_advertising_call_count_;
    if (should_succeed_on_start_)
      std::move(callback).Run();
    else
      std::move(error_callback).Run();
  }

  void StopAdvertising(base::OnceCallback<void()> callback) override {
    std::move(on_stop_advertising_callback_).Run();
    std::move(callback).Run();
  }

  size_t start_advertising_call_count() {
    return start_advertising_call_count_;
  }

 private:
  bool should_succeed_on_start_;
  size_t start_advertising_call_count_ = 0u;
  base::OnceCallback<void()> on_stop_advertising_callback_;
  base::OnceCallback<void()> on_destroy_callback_;
};

class FakeFastInitiationManagerFactory : public FastInitiationManager::Factory {
 public:
  explicit FakeFastInitiationManagerFactory(bool should_succeed_on_start)
      : should_succeed_on_start_(should_succeed_on_start) {}

  std::unique_ptr<FastInitiationManager> CreateInstance(
      scoped_refptr<device::BluetoothAdapter> adapter) override {
    auto fake_fast_initiation_manager = std::make_unique<
        FakeFastInitiationManager>(
        adapter, should_succeed_on_start_,
        base::BindOnce(&FakeFastInitiationManagerFactory::OnStopAdvertising,
                       weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(
            &FakeFastInitiationManagerFactory::OnFastInitiationManagerDestroyed,
            weak_ptr_factory_.GetWeakPtr()));
    last_fake_fast_initiation_manager_ = fake_fast_initiation_manager.get();
    return std::move(fake_fast_initiation_manager);
  }

  void OnStopAdvertising() { stop_advertising_called_ = true; }

  void OnFastInitiationManagerDestroyed() {
    fast_initiation_manager_destroyed_ = true;
    last_fake_fast_initiation_manager_ = nullptr;
  }

  size_t StartAdvertisingCount() {
    return last_fake_fast_initiation_manager_->start_advertising_call_count();
  }

  bool StopAdvertisingCalledAndManagerDestroyed() {
    return stop_advertising_called_ && fast_initiation_manager_destroyed_;
  }

 private:
  FakeFastInitiationManager* last_fake_fast_initiation_manager_ = nullptr;
  bool should_succeed_on_start_ = false;
  bool stop_advertising_called_ = false;
  bool fast_initiation_manager_destroyed_ = false;
  base::WeakPtrFactory<FakeFastInitiationManagerFactory> weak_ptr_factory_{
      this};
};

class MockTransferUpdateCallback : public TransferUpdateCallback {
 public:
  ~MockTransferUpdateCallback() override = default;

  MOCK_METHOD(void,
              OnTransferUpdate,
              (const ShareTarget& shareTarget,
               const TransferMetadata& transferMetadata),
              (override));
};

namespace {

const char kEndpointId[] = "endpoint_id";

sharing::mojom::FramePtr GetValidIntroductionFrame() {
  std::vector<sharing::mojom::TextMetadataPtr> mojo_text_metadatas;
  for (int i = 1; i <= 3; i++) {
    mojo_text_metadatas.push_back(sharing::mojom::TextMetadata::New(
        "title " + base::NumberToString(i),
        static_cast<sharing::mojom::TextMetadata::Type>(i), i, i, i));
  }

  sharing::mojom::V1FramePtr mojo_v1frame = sharing::mojom::V1Frame::New();
  mojo_v1frame->set_introduction(sharing::mojom::IntroductionFrame::New(
      std::vector<sharing::mojom::FileMetadataPtr>(),
      std::move(mojo_text_metadatas), base::nullopt,
      std::vector<sharing::mojom::WifiCredentialsMetadataPtr>()));

  sharing::mojom::FramePtr mojo_frame = sharing::mojom::Frame::New();
  mojo_frame->set_v1(std::move(mojo_v1frame));
  return mojo_frame;
}

class NearbySharingServiceImplTest : public testing::Test {
 public:
  NearbySharingServiceImplTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kNearbySharing);
    RegisterNearbySharingPrefs(prefs_.registry());
  }

  ~NearbySharingServiceImplTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());

    mock_bluetooth_adapter_ =
        base::MakeRefCounted<NiceMock<device::MockBluetoothAdapter>>();
    ON_CALL(*mock_bluetooth_adapter_, IsPresent())
        .WillByDefault(
            Invoke(this, &NearbySharingServiceImplTest::IsBluetoothPresent));
    ON_CALL(*mock_bluetooth_adapter_, IsPowered())
        .WillByDefault(
            Invoke(this, &NearbySharingServiceImplTest::IsBluetoothPowered));
    ON_CALL(*mock_bluetooth_adapter_, AddObserver(_))
        .WillByDefault(
            Invoke(this, &NearbySharingServiceImplTest::AddAdapterObserver));
    device::BluetoothAdapterFactory::SetAdapterForTesting(
        mock_bluetooth_adapter_);

    service_ = CreateService("name");
    SetFakeFastInitiationManagerFactory(/*should_succeed_on_start=*/true);
  }

  void TearDown() override { profile_manager_.DeleteAllTestingProfiles(); }

  std::unique_ptr<NearbySharingServiceImpl> CreateService(
      const std::string& profile_name) {
    Profile* profile = profile_manager_.CreateTestingProfile(profile_name);
    fake_nearby_connections_manager_ = new FakeNearbyConnectionsManager();
    notification_tester_ =
        std::make_unique<NotificationDisplayServiceTester>(profile);
    NotificationDisplayService* notification_display_service =
        NotificationDisplayServiceFactory::GetForProfile(profile);
    auto service = std::make_unique<NearbySharingServiceImpl>(
        &prefs_, notification_display_service, profile,
        base::WrapUnique(fake_nearby_connections_manager_),
        &mock_nearby_process_manager_);

    // Allow the posted task to fetch the BluetoothAdapter to finish.
    base::RunLoop().RunUntilIdle();

    return service;
  }

  void SetFakeFastInitiationManagerFactory(bool should_succeed_on_start) {
    fast_initiation_manager_factory_ =
        std::make_unique<FakeFastInitiationManagerFactory>(
            should_succeed_on_start);
    FastInitiationManager::Factory::SetFactoryForTesting(
        fast_initiation_manager_factory_.get());
  }

  bool IsBluetoothPresent() { return is_bluetooth_present_; }
  bool IsBluetoothPowered() { return is_bluetooth_powered_; }

  void AddAdapterObserver(device::BluetoothAdapter::Observer* observer) {
    DCHECK(!adapter_observer_);
    adapter_observer_ = observer;
  }

  void SetConnectionType(net::NetworkChangeNotifier::ConnectionType type) {
    network_notifier_->SetConnectionType(type);
    network_notifier_->NotifyObserversOfNetworkChangeForTests(
        network_notifier_->GetConnectionType());
  }

  NiceMock<MockNearbyProcessManager>& mock_nearby_process_manager() {
    return mock_nearby_process_manager_;
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  ui::ScopedSetIdleState idle_state_{ui::IDLE_STATE_IDLE};
  TestingProfileManager profile_manager_{TestingBrowserProcess::GetGlobal()};
  sync_preferences::TestingPrefServiceSyncable prefs_;
  FakeNearbyConnectionsManager* fake_nearby_connections_manager_ = nullptr;
  std::unique_ptr<NotificationDisplayServiceTester> notification_tester_;
  std::unique_ptr<NearbySharingServiceImpl> service_;
  std::unique_ptr<FakeFastInitiationManagerFactory>
      fast_initiation_manager_factory_;
  bool is_bluetooth_present_ = true;
  bool is_bluetooth_powered_ = true;
  device::BluetoothAdapter::Observer* adapter_observer_ = nullptr;
  scoped_refptr<NiceMock<device::MockBluetoothAdapter>> mock_bluetooth_adapter_;
  NiceMock<MockNearbyProcessManager> mock_nearby_process_manager_;
  std::unique_ptr<net::test::MockNetworkChangeNotifier> network_notifier_ =
      net::test::MockNetworkChangeNotifier::Create();
};

}  // namespace

TEST_F(NearbySharingServiceImplTest, AddsNearbyProcessObserver) {
  EXPECT_TRUE(
      mock_nearby_process_manager().observers_.HasObserver(service_.get()));
}

TEST_F(NearbySharingServiceImplTest, RemovesNearbyProcessObserver) {
  service_.reset();
  EXPECT_FALSE(mock_nearby_process_manager().observers_.might_have_observers());
}

TEST_F(NearbySharingServiceImplTest, DisableNearbyShutdownConnections) {
  prefs_.SetBoolean(prefs::kNearbySharingEnabledPrefName, false);
  service_->FlushMojoForTesting();
  EXPECT_TRUE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest, StartFastInitiationAdvertising) {
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
  EXPECT_EQ(1u, fast_initiation_manager_factory_->StartAdvertisingCount());

  // Call RegisterSendSurface() a second time and make sure StartAdvertising is
  // not called again.
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
  EXPECT_EQ(1u, fast_initiation_manager_factory_->StartAdvertisingCount());
}

TEST_F(NearbySharingServiceImplTest, StartFastInitiationAdvertisingError) {
  SetFakeFastInitiationManagerFactory(/*should_succeed_on_start=*/false);
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
}

TEST_F(NearbySharingServiceImplTest,
       StartFastInitiationAdvertising_BluetoothNotPresent) {
  is_bluetooth_present_ = false;
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
}

TEST_F(NearbySharingServiceImplTest,
       StartFastInitiationAdvertising_BluetoothNotPowered) {
  is_bluetooth_powered_ = false;
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
}

TEST_F(NearbySharingServiceImplTest, StopFastInitiationAdvertising) {
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
  EXPECT_EQ(1u, fast_initiation_manager_factory_->StartAdvertisingCount());
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->UnregisterSendSurface(/*transfer_callback=*/nullptr,
                                            /*discovery_callback=*/nullptr));
  EXPECT_TRUE(fast_initiation_manager_factory_
                  ->StopAdvertisingCalledAndManagerDestroyed());
}

TEST_F(NearbySharingServiceImplTest,
       StopFastInitiationAdvertising_BluetoothBecomesNotPresent) {
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
  adapter_observer_->AdapterPresentChanged(mock_bluetooth_adapter_.get(),
                                           false);
  EXPECT_TRUE(fast_initiation_manager_factory_
                  ->StopAdvertisingCalledAndManagerDestroyed());
}

TEST_F(NearbySharingServiceImplTest,
       StopFastInitiationAdvertising_BluetoothBecomesNotPowered) {
  EXPECT_EQ(NearbySharingService::StatusCodes::kOk,
            service_->RegisterSendSurface(
                /*transfer_callback=*/nullptr,
                /*discovery_callback=*/nullptr,
                NearbySharingService::SendSurfaceState::kForeground));
  adapter_observer_->AdapterPoweredChanged(mock_bluetooth_adapter_.get(),
                                           false);
  EXPECT_TRUE(fast_initiation_manager_factory_
                  ->StopAdvertisingCalledAndManagerDestroyed());
}

TEST_F(NearbySharingServiceImplTest,
       ForegroundRegisterReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_EQ(PowerLevel::kHighPower,
            fake_nearby_connections_manager_->GetAdvertisingPowerLevel());
}

TEST_F(NearbySharingServiceImplTest,
       BackgroundRegisterReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kSelectedContacts));
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kBackground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_EQ(PowerLevel::kLowPower,
            fake_nearby_connections_manager_->GetAdvertisingPowerLevel());
}

TEST_F(NearbySharingServiceImplTest,
       RegisterReceiveSurfaceTwiceSameCallbackKeepAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  NearbySharingService::StatusCodes result2 = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result2, NearbySharingService::StatusCodes::kError);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       RegisterReceiveSurfaceTwiceKeepAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  MockTransferUpdateCallback callback2;
  NearbySharingService::StatusCodes result2 = service_->RegisterReceiveSurface(
      &callback2, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result2, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       ScreenLockedRegisterReceiveSurfaceNotAdvertising) {
  ui::ScopedSetIdleState locked(ui::IDLE_STATE_LOCKED);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       DataUsageChangedRegisterReceiveSurfaceRestartsAdvertising) {
  ui::ScopedSetIdleState locked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);

  prefs_.SetInteger(prefs::kNearbySharingDataUsageName,
                    static_cast<int>(DataUsage::kOffline));
  service_->FlushMojoForTesting();
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_EQ(DataUsage::kOffline,
            fake_nearby_connections_manager_->GetAdvertisingDataUsage());

  prefs_.SetInteger(prefs::kNearbySharingDataUsageName,
                    static_cast<int>(DataUsage::kOnline));
  service_->FlushMojoForTesting();
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_EQ(DataUsage::kOnline,
            fake_nearby_connections_manager_->GetAdvertisingDataUsage());
}

TEST_F(NearbySharingServiceImplTest,
       NoNetworkRegisterReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  // Succeeds since bluetooth is present.
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       NoBluetoothNoNetworkRegisterReceiveSurfaceNotAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  is_bluetooth_present_ = false;
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest, WifiRegisterReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       EthernetRegisterReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       ThreeGRegisterReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_3G);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  // Since bluetooth is on, connection still succeeds.
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       NoBluetoothWifiReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  is_bluetooth_present_ = false;
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       NoBluetoothEthernetReceiveSurfaceIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  is_bluetooth_present_ = false;
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       NoBluetoothThreeGReceiveSurfaceNotAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  is_bluetooth_present_ = false;
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_3G);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       DisableFeatureReceiveSurfaceNotAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  prefs_.SetBoolean(prefs::kNearbySharingEnabledPrefName, false);
  service_->FlushMojoForTesting();
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_TRUE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       DisableFeatureReceiveSurfaceStopsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  prefs_.SetBoolean(prefs::kNearbySharingEnabledPrefName, false);
  service_->FlushMojoForTesting();
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_TRUE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       ForegroundReceiveSurfaceNoOneVisibilityIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kNoOne));
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       BackgroundReceiveSurfaceNoOneVisibilityNotAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kNoOne));
  service_->FlushMojoForTesting();
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kBackground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       BackgroundReceiveSurfaceVisibilityToNoOneStopsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kSelectedContacts));
  service_->FlushMojoForTesting();
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kBackground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kNoOne));
  service_->FlushMojoForTesting();
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       BackgroundReceiveSurfaceVisibilityToSelectedStartsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kNoOne));
  service_->FlushMojoForTesting();
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kBackground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());

  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kSelectedContacts));
  service_->FlushMojoForTesting();
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       ForegroundReceiveSurfaceSelectedContactsVisibilityIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kSelectedContacts));
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       BackgroundReceiveSurfaceSelectedContactsVisibilityIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kSelectedContacts));
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kBackground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       ForegroundReceiveSurfaceAllContactsVisibilityIsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kAllContacts));
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       BackgroundReceiveSurfaceAllContactsVisibilityNotAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  prefs_.SetInteger(prefs::kNearbySharingBackgroundVisibilityName,
                    static_cast<int>(Visibility::kAllContacts));
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kBackground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest, UnregisterReceiveSurfaceStopsAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  NearbySharingService::StatusCodes result2 =
      service_->UnregisterReceiveSurface(&callback);
  EXPECT_EQ(result2, NearbySharingService::StatusCodes::kOk);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
  EXPECT_FALSE(fake_nearby_connections_manager_->IsShutdown());
}

TEST_F(NearbySharingServiceImplTest,
       UnregisterReceiveSurfaceDifferentCallbackKeepAdvertising) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  MockTransferUpdateCallback callback2;
  NearbySharingService::StatusCodes result2 =
      service_->UnregisterReceiveSurface(&callback2);
  EXPECT_EQ(result2, NearbySharingService::StatusCodes::kError);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest, UnregisterReceiveSurfaceNeverRegistered) {
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  MockTransferUpdateCallback callback;
  NearbySharingService::StatusCodes result =
      service_->UnregisterReceiveSurface(&callback);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kError);
  EXPECT_FALSE(fake_nearby_connections_manager_->IsAdvertising());
}

TEST_F(NearbySharingServiceImplTest,
       IncomingConnection_EmptyIntroductionFrame) {
  std::string intro = "introduction_frame";
  std::vector<uint8_t> bytes(intro.begin(), intro.end());
  NiceMock<MockNearbySharingDecoder> mock_decoder;
  EXPECT_CALL(mock_decoder, DecodeFrame(testing::Eq(bytes), testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<uint8_t>& data,
              MockNearbySharingDecoder::DecodeFrameCallback callback) {
            sharing::mojom::V1FramePtr mojo_v1frame =
                sharing::mojom::V1Frame::New();
            mojo_v1frame->set_introduction(
                sharing::mojom::IntroductionFrame::New());

            sharing::mojom::FramePtr mojo_frame = sharing::mojom::Frame::New();
            mojo_frame->set_v1(std::move(mojo_v1frame));
            std::move(callback).Run(std::move(mojo_frame));
          }));

  EXPECT_CALL(mock_nearby_process_manager(),
              GetOrStartNearbySharingDecoder(testing::_))
      .WillRepeatedly(testing::Return(&mock_decoder));

  FakeNearbyConnection connection;
  connection.AppendReadableData(bytes);
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  NiceMock<MockTransferUpdateCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, OnTransferUpdate(testing::_, testing::_))
      .WillOnce(testing::Invoke([&run_loop](const ShareTarget& share_target,
                                            TransferMetadata metadata) {
        EXPECT_EQ(TransferMetadata::Status::kUnsupportedAttachmentType,
                  metadata.status());
        run_loop.Quit();
      }));

  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  service_->OnIncomingConnection(kEndpointId, {}, &connection);
  run_loop.Run();

  // To avoid UAF in OnIncomingTransferUpdate().
  service_->UnregisterReceiveSurface(&callback);
}

TEST_F(NearbySharingServiceImplTest,
       IncomingConnection_ValidIntroductionFrame) {
  std::string intro = "introduction_frame";
  std::vector<uint8_t> bytes(intro.begin(), intro.end());
  NiceMock<MockNearbySharingDecoder> mock_decoder;
  EXPECT_CALL(mock_decoder, DecodeFrame(testing::Eq(bytes), testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<uint8_t>& data,
              MockNearbySharingDecoder::DecodeFrameCallback callback) {
            std::move(callback).Run(GetValidIntroductionFrame());
          }));

  EXPECT_CALL(mock_nearby_process_manager(),
              GetOrStartNearbySharingDecoder(testing::_))
      .WillRepeatedly(testing::Return(&mock_decoder));

  FakeNearbyConnection connection;
  connection.AppendReadableData(bytes);
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  NiceMock<MockTransferUpdateCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, OnTransferUpdate(testing::_, testing::_))
      .WillOnce(testing::Invoke([&run_loop](const ShareTarget& share_target,
                                            TransferMetadata metadata) {
        EXPECT_EQ(TransferMetadata::Status::kAwaitingLocalConfirmation,
                  metadata.status());
        run_loop.Quit();
      }));

  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  service_->OnIncomingConnection(kEndpointId, {}, &connection);
  run_loop.Run();

  // To avoid UAF in OnIncomingTransferUpdate().
  service_->UnregisterReceiveSurface(&callback);
}

TEST_F(NearbySharingServiceImplTest, AcceptInvalidShareTarget) {
  ShareTarget share_target;
  base::RunLoop run_loop;
  service_->Accept(
      share_target,
      base::BindLambdaForTesting(
          [&](NearbySharingServiceImpl::StatusCodes status_code) {
            EXPECT_EQ(NearbySharingServiceImpl::StatusCodes::kOutOfOrderApiCall,
                      status_code);
            run_loop.Quit();
          }));

  run_loop.Run();
}

TEST_F(NearbySharingServiceImplTest, AcceptValidShareTarget) {
  // TODO(himanshujaju) - Refactor common set up.
  std::string intro = "introduction_frame";
  std::vector<uint8_t> bytes(intro.begin(), intro.end());
  NiceMock<MockNearbySharingDecoder> mock_decoder;
  EXPECT_CALL(mock_decoder, DecodeFrame(testing::Eq(bytes), testing::_))
      .WillOnce(testing::Invoke(
          [&](const std::vector<uint8_t>& data,
              MockNearbySharingDecoder::DecodeFrameCallback callback) {
            std::move(callback).Run(GetValidIntroductionFrame());
          }));

  EXPECT_CALL(mock_nearby_process_manager(),
              GetOrStartNearbySharingDecoder(testing::_))
      .WillRepeatedly(testing::Return(&mock_decoder));

  ShareTarget share_target;
  FakeNearbyConnection connection;
  connection.AppendReadableData(bytes);
  ui::ScopedSetIdleState unlocked(ui::IDLE_STATE_IDLE);
  SetConnectionType(net::NetworkChangeNotifier::CONNECTION_WIFI);
  NiceMock<MockTransferUpdateCallback> callback;
  base::RunLoop run_loop;
  EXPECT_CALL(callback, OnTransferUpdate(testing::_, testing::_))
      .WillOnce(testing::Invoke([&](const ShareTarget& incoming_share_target,
                                    TransferMetadata metadata) {
        EXPECT_EQ(TransferMetadata::Status::kAwaitingLocalConfirmation,
                  metadata.status());
        share_target = incoming_share_target;
        run_loop.Quit();
      }));

  NearbySharingService::StatusCodes result = service_->RegisterReceiveSurface(
      &callback, NearbySharingService::ReceiveSurfaceState::kForeground);
  EXPECT_EQ(result, NearbySharingService::StatusCodes::kOk);
  EXPECT_TRUE(fake_nearby_connections_manager_->IsAdvertising());

  service_->OnIncomingConnection(kEndpointId, {}, &connection);
  run_loop.Run();

  base::RunLoop run_loop_accept;
  EXPECT_CALL(callback, OnTransferUpdate(testing::_, testing::_))
      .WillOnce(testing::Invoke(
          [](const ShareTarget& share_target, TransferMetadata metadata) {
            EXPECT_EQ(TransferMetadata::Status::kAwaitingRemoteAcceptance,
                      metadata.status());
          }));

  service_->Accept(share_target,
                   base::BindLambdaForTesting(
                       [&](NearbySharingServiceImpl::StatusCodes status_code) {
                         EXPECT_EQ(NearbySharingServiceImpl::StatusCodes::kOk,
                                   status_code);
                         run_loop_accept.Quit();
                       }));

  run_loop_accept.Run();

  EXPECT_TRUE(
      fake_nearby_connections_manager_->DidUpgradeBandwidth(kEndpointId));

  // To avoid UAF in OnIncomingTransferUpdate().
  service_->UnregisterReceiveSurface(&callback);
}

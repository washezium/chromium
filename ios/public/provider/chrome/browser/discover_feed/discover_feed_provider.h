// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_DISCOVER_FEED_DISCOVER_FEED_PROVIDER_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_DISCOVER_FEED_DISCOVER_FEED_PROVIDER_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"

@protocol ApplicationCommands;
class Browser;

// DiscoverFeedProvider allows embedders to provide functionality for a Discover
// Feed.
class DiscoverFeedProvider {
 public:
  // Observer class for discover feed events.
  class Observer {
   public:
    Observer() {}
    virtual ~Observer() {}
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    // Called whenever the FeedProvider Model has changed. At this point all
    // existing Feed ViewControllers are stale and need to be refreshed.
    virtual void OnDiscoverFeedModelRecreated() = 0;
  };

  DiscoverFeedProvider() = default;
  virtual ~DiscoverFeedProvider() = default;

  DiscoverFeedProvider(const DiscoverFeedProvider&) = delete;
  DiscoverFeedProvider& operator=(const DiscoverFeedProvider&) = delete;

  // Returns true if the Discover Feed is enabled.
  virtual bool IsDiscoverFeedEnabled();
  // Returns the Discover Feed ViewController.
  virtual UIViewController* NewFeedViewController(Browser* browser)
      NS_RETURNS_RETAINED;
  // Updates the feed's theme to match the user's theme (light/dark).
  virtual void UpdateTheme();
  // Refreshes the Discover Feed with completion.
  virtual void RefreshFeedWithCompletion(ProceduralBlock completion);
  // Methods to register or remove observers.
  virtual void AddObserver(Observer* observer);
  virtual void RemoveObserver(Observer* observer);
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_DISCOVER_FEED_DISCOVER_FEED_PROVIDER_H_

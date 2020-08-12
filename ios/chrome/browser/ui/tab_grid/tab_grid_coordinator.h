// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_COORDINATOR_H_

#import <Foundation/Foundation.h>

#import "base/ios/block_types.h"
#import "ios/chrome/browser/chrome_root_coordinator.h"

@protocol ApplicationCommands;
class Browser;
@protocol BrowsingDataCommands;
@protocol TabGridCoordinatorDelegate;
struct UrlLoadParams;

@interface TabGridCoordinator : ChromeRootCoordinator

- (instancetype)initWithWindow:(UIWindow*)window
     applicationCommandEndpoint:
         (id<ApplicationCommands>)applicationCommandEndpoint
    browsingDataCommandEndpoint:
        (id<BrowsingDataCommands>)browsingDataCommandEndpoint
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithWindow:(UIWindow*)window NS_UNAVAILABLE;

@property(nonatomic, weak) id<TabGridCoordinatorDelegate> delegate;

@property(nonatomic, assign) Browser* regularBrowser;
@property(nonatomic, assign) Browser* incognitoBrowser;

// The view controller, if any, that is active.
@property(nonatomic, readonly, strong) UIViewController* activeViewController;

// If this property is YES, calls to |showTabSwitcher:completion:| and
// |showTabViewController:completion:| will present the given view controllers
// without animation.  This should only be used by unittests.
@property(nonatomic, readwrite, assign) BOOL animationsDisabledForTesting;

// Stops all child coordinators then calls |completion|. |completion| is called
// whether or not child coordinators exist.
- (void)stopChildCoordinatorsWithCompletion:(ProceduralBlock)completion;

// Displays the TabGrid.
- (void)showTabGrid;

// Displays the given view controller, replacing any TabSwitchers or other view
// controllers that may currently be visible.  Runs the given |completion| block
// after the view controller is visible.
- (void)showTabViewController:(UIViewController*)viewController
                   completion:(ProceduralBlock)completion;

// Perform any initial setup required for the appearance of the TabGrid.
- (void)prepareToShowTabGrid;

// Restores the internal state of the tab switcher with the given browser,
// which must not be nil. |activeBrowser| is the browser which starts active,
// and must be one of the other two browsers. Should only be called when the
// object is not being shown.
- (void)restoreInternalStateWithMainBrowser:(Browser*)mainBrowser
                                 otrBrowser:(Browser*)otrBrowser
                              activeBrowser:(Browser*)activeBrowser;

// Create a new tab in |browser|. Implementors are expected to also perform an
// animation from the selected tab in the tab switcher to the newly created tab
// in the content area. Objects adopting this protocol should call the following
// delegate methods:
//   |-tabSwitcher:shouldFinishWithBrowser:|
// to inform the delegate when this animation begins and ends.
- (void)dismissWithNewTabAnimationToBrowser:(Browser*)browser
                          withUrlLoadParams:(const UrlLoadParams&)urlLoadParams
                                    atIndex:(int)position;

// Updates the OTR (Off The Record) browser. Should only be called when both
// the current OTR browser and the new OTR browser are either nil or contain no
// tabs. This must be called after the otr browser has been deleted because the
// incognito browser state is deleted.
- (void)setOtrBrowser:(Browser*)otrBrowser;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_COORDINATOR_H_

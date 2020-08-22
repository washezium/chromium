// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/in_product_help/feature_promo_registry.h"

#include "base/no_destructor.h"
#include "base/optional.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/in_product_help/feature_promo_bubble_params.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/grit/generated_resources.h"
#include "components/feature_engagement/public/feature_constants.h"

namespace {

// Functions to get an anchor view for an IPH should go here.

// kIPHDesktopTabGroupsNewGroupFeature:
views::View* GetTabGroupsAnchorView(BrowserView* browser_view) {
  constexpr int kPreferredAnchorTab = 2;
  return browser_view->tabstrip()->GetTabViewForPromoAnchor(
      kPreferredAnchorTab);
}

}  // namespace

FeaturePromoRegistry::FeaturePromoRegistry() {
  RegisterKnownFeatures();
}

FeaturePromoRegistry::~FeaturePromoRegistry() = default;

// static
FeaturePromoRegistry* FeaturePromoRegistry::GetInstance() {
  static base::NoDestructor<FeaturePromoRegistry> instance;
  return instance.get();
}

base::Optional<FeaturePromoBubbleParams>
FeaturePromoRegistry::GetParamsForFeature(const base::Feature& iph_feature,
                                          BrowserView* browser_view) {
  auto data_it = feature_promo_data_.find(&iph_feature);
  DCHECK(data_it != feature_promo_data_.end());

  views::View* const anchor_view =
      data_it->second.get_anchor_view_callback.Run(browser_view);
  if (!anchor_view)
    return base::nullopt;

  FeaturePromoBubbleParams params = data_it->second.params;
  params.anchor_view = anchor_view;
  return params;
}

void FeaturePromoRegistry::RegisterFeature(
    const base::Feature& iph_feature,
    const FeaturePromoBubbleParams& params,
    GetAnchorViewCallback get_anchor_view_callback) {
  FeaturePromoData data;
  data.params = params;
  data.get_anchor_view_callback = std::move(get_anchor_view_callback);
  feature_promo_data_.emplace(&iph_feature, std::move(data));
}

void FeaturePromoRegistry::ClearFeaturesForTesting() {
  feature_promo_data_.clear();
}

void FeaturePromoRegistry::ReinitializeForTesting() {
  ClearFeaturesForTesting();
  RegisterKnownFeatures();
}

void FeaturePromoRegistry::RegisterKnownFeatures() {
  {
    // kIPHDesktopTabGroupsNewGroupFeature:
    FeaturePromoBubbleParams params;
    params.body_string_specifier = IDS_TAB_GROUPS_NEW_GROUP_PROMO;
    params.arrow = views::BubbleBorder::TOP_LEFT;
    RegisterFeature(feature_engagement::kIPHDesktopTabGroupsNewGroupFeature,
                    params, base::BindRepeating(GetTabGroupsAnchorView));
  }
}

FeaturePromoRegistry::FeaturePromoData::FeaturePromoData() = default;
FeaturePromoRegistry::FeaturePromoData::FeaturePromoData(FeaturePromoData&&) =
    default;
FeaturePromoRegistry::FeaturePromoData::~FeaturePromoData() = default;

// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Tests for shared Polymer components. */

// Polymer BrowserTest fixture.
GEN_INCLUDE(['//chrome/test/data/webui/polymer_browser_test_base.js']);

GEN('#include "content/public/test/browser_test.h"');

// Polymer 2 test list format:
//
// ['ModuleNameTest', 'module.js',
//   [<module.js dependency source list>]
// ]
// clang-format off
[
  ['CrPolicyNetworkBehaviorMojo', 'network/cr_policy_network_behavior_mojo_tests.js',
    ['../../cr_elements/cr_policy_strings.js']
  ],
  ['CrPolicyNetworkIndicatorMojo', 'network/cr_policy_network_indicator_mojo_tests.js',
    ['../../cr_elements/cr_policy_strings.js']
  ],
  ['NetworkConfig', 'network/network_config_test.js',
    [
      '//ui/webui/resources/js/assert.js',
      '//ui/webui/resources/js/promise_resolver.js',
      '../../fake_chrome_event.js',
      '../../chromeos/networking_private_constants.js',
      '../../chromeos/fake_network_config_mojom.js',
    ]
  ],
  ['NetworkConfigElementBehavior', 'network/network_config_element_behavior_test.js', []],
  ['NetworkListItemTest', 'network/network_list_item_test.js', []],
  ['NetworkPasswordInput', 'network/network_password_input_test.js', []],
].forEach(test => registerTest('NetworkComponents', 'os-settings', ...test));

[
  ['BasePage', 'cellular_setup/base_page_test.js', []],
  ['FinalPage', 'cellular_setup/final_page_test.js', []],
  ['SimDetectPage', 'cellular_setup/sim_detect_page_test.js', []],
].forEach(test => registerTest('CellularSetup', 'cellular-setup', ...test));
// clang-format on

function registerTest(componentName, webuiHost, testName, module, deps) {
  const className = `${componentName}${testName}Test`;
  this[className] = class extends PolymerTest {
    /** @override */
    get browsePreload() {
      return `chrome://${webuiHost}/`;
    }

    /** @override */
    get extraLibraries() {
      return super.extraLibraries.concat(module).concat(deps);
    }
  };

  TEST_F(className, 'All', () => mocha.run());
}

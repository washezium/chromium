// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Tests for shared Polymer components. */

// Polymer BrowserTest fixture.
GEN_INCLUDE(['//chrome/test/data/webui/polymer_browser_test_base.js']);

GEN('#include "content/public/test/browser_test.h"');

// Tests are flaky on ChromeOS, debug (crbug.com/1114675).
GEN('#if defined(OS_CHROMEOS) && !defined(NDEBUG)');
GEN('#define MAYBE_All DISABLED_All');
GEN('#else');
GEN('#define MAYBE_All All');
GEN('#endif');

// Polymer 2 test list format:
//
// ['ModuleNameTest', 'module.js',
//   [<module.js dependency source list>]
// ]
// clang-format off
[
  ['CrPolicyNetworkBehaviorMojo', 'cr_policy_network_behavior_mojo_tests.js',
    ['../../cr_elements/cr_policy_strings.js']
  ],
  ['CrPolicyNetworkIndicatorMojo', 'cr_policy_network_indicator_mojo_tests.js',
    [ '../../cr_elements/cr_policy_strings.js' ]
  ],
  ['NetworkConfig', 'network_config_test.js',
    [
      '//ui/webui/resources/js/assert.js',
      '//ui/webui/resources/js/promise_resolver.js',
      '../../fake_chrome_event.js',
      '../../chromeos/networking_private_constants.js',
      '../../chromeos/fake_network_config_mojom.js',
    ]
  ],
  ['NetworkConfigElementBehavior', 'network_config_element_behavior_test.js',
    []
  ],
  ['NetworkPasswordInput', 'network_password_input_test.js',
    []
  ],
].forEach(test => registerTest('Network', 'internet-config-dialog', ...test));

[
  ['BasePage', 'base_page_test.js',
    []
  ],
].forEach(test => registerTest('CellularSetup', 'cellular-setup', ...test));

// clang-format on

function registerTest(componentName, webuiHost, testName, module, deps) {
  const className = `${componentName}${testName}Test`;
  this[className] = class extends PolymerTest {
    /** @override */
    get browsePreload() {
      return `chrome://${webuiHost}/test_loader.html?module=cr_components/chromeos/${module}`;
    }

    /** @override */
    get extraLibraries() {
      return PolymerTest.prototype.extraLibraries.concat(module).concat(deps);
    }
  };

  TEST_F(className, 'All', () => mocha.run());
}

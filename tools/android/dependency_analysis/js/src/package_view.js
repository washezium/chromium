// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {PackageGraphPage} from './vue_components/package_graph_page.js';

// For ease of development, we serve our testing data on localhost:8888. This
// should be changed as we find other ways to serve the assets (eg. user upload
// or hosted externally).
const LOCALHOST = 'http://localhost:8888';

import * as d3 from 'd3';
import './common.css';
import './package.css';

document.addEventListener('DOMContentLoaded', () => {
  d3.json(`${LOCALHOST}/json_graph.txt`).then(data => {
    new PackageGraphPage({
      el: '#package-graph-page',
      propsData: {
        graphJson: data.package_graph,
      },
    });
  });
});

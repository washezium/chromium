#!/usr/bin/env python
# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Paths to description XML files in this directory."""

import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'common'))
import path_util


def _FindHistogramsXmlFiles():
  """Gets a list relative path to all histograms xmls under histograms_xml/."""
  file_list = []
  for dirName, _, fileList in os.walk(PATH_TO_HISTOGRAMS_XML_DIR):
    for filename in fileList:
      if filename == 'histograms.xml' or filename == 'histogram_suffixes.xml':
        # Compute the relative path of the histograms xml file.
        filepath = os.path.relpath(
          os.path.join(dirName, filename), PATH_TO_HISTOGRAMS_XML_DIR)
        file_list.append(os.path.join('tools/metrics/histograms/', filepath))
  return file_list


ENUMS_XML_RELATIVE = 'tools/metrics/histograms/enums.xml'
# The absolute path to the histograms_xml folder.
PATH_TO_HISTOGRAMS_XML_DIR = path_util.GetInputFile(
    'tools/metrics/histograms/histograms_xml')
# In the middle state, histogram paths include both the large histograms.xml
# file as well as the split up files.
# TODO: Improve on the current design to avoid calling `os.walk()` at the time
# of module import.
HISTOGRAMS_XMLS_RELATIVE = (['tools/metrics/histograms/histograms.xml'] +
                            _FindHistogramsXmlFiles())
OBSOLETE_XML_RELATIVE = ('tools/metrics/histograms/histograms_xml/'
                         'obsolete_histograms.xml')
ALL_XMLS_RELATIVE = [ENUMS_XML_RELATIVE, OBSOLETE_XML_RELATIVE
                     ] + HISTOGRAMS_XMLS_RELATIVE

ENUMS_XML = path_util.GetInputFile(ENUMS_XML_RELATIVE)
UKM_XML = path_util.GetInputFile('tools/metrics/ukm/ukm.xml')
HISTOGRAMS_XMLS = [path_util.GetInputFile(f) for f in HISTOGRAMS_XMLS_RELATIVE]
OBSOLETE_XML = path_util.GetInputFile(OBSOLETE_XML_RELATIVE)
ALL_XMLS = [path_util.GetInputFile(f) for f in ALL_XMLS_RELATIVE]

ALL_TEST_XMLS_RELATIVE = [
    'tools/metrics/histograms/test_data/enums.xml',
    'tools/metrics/histograms/test_data/histograms.xml',
    'tools/metrics/histograms/test_data/ukm.xml',
]
ALL_TEST_XMLS = [path_util.GetInputFile(f) for f in ALL_TEST_XMLS_RELATIVE]
TEST_ENUMS_XML, TEST_HISTOGRAMS_XML, TEST_UKM_XML = ALL_TEST_XMLS

# The path to the `histogram_index` file.
HISTOGRAMS_INDEX = path_util.GetInputFile(
    'tools/metrics/histograms/histograms_index.txt')


def main():
  with open(HISTOGRAMS_INDEX, 'w+') as f:
    f.write("\n".join(HISTOGRAMS_XMLS_RELATIVE))


if __name__ == '__main__':
  # Update the `histograms_index` file whenever histograms paths are updated.
  # This file records all currently existing histograms.xml paths.
  main()

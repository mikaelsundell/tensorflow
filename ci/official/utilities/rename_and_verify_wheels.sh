#!/usr/bin/env bash
#
# Copyright 2022 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
#
# Usage: rename_and_verify_wheels.sh
# This script is aware of TFCI_ variables, so it doesn't need any arguments.
# Puts new wheel through auditwheel to rename and verify it, deletes the old
# one, checks the filesize, and then ensures the new wheel is installable.
set -euxo pipefail

cd "$TFCI_OUTPUT_DIR"

if [[ "$(ls *.whl | wc -l)" != "1" ]]; then
  echo "Error: $TFCI_OUTPUT_DIR should contain exactly one .whl file."
  exit 1
fi

# Repair wheels with auditwheel and delete the old one.
if [[ "$TFCI_WHL_AUDIT_ENABLE" == "1" ]]; then
  python3 -m auditwheel repair --plat "$TFCI_WHL_AUDIT_PLAT" --wheel-dir . *.whl
  # if the wheel is already named correctly, auditwheel won't rename it. so we
  # list all .whl files by their modification time (ls -t) and delete anything
  # other than the most recently-modified one (the new one).
  ls -t *.whl | tail -n +2 | xargs rm
fi

# Check if size is too big. TFCI_WHL_SIZE_LIMIT is in find's format, which can be
# 'k' for kilobytes, 'M' for megabytes, or 'G' for gigabytes, and the + to indicate
# "anything greater than" is added by the script.
if [[ "$TFCI_WHL_SIZE_LIMIT_ENABLE" == "1" ]] && [[ -n "$(find . -iname "*.whl" -size "+$TFCI_WHL_SIZE_LIMIT")" ]]; then
  echo "Error: Generated wheel is too big! Limit is $TFCI_WHL_SIZE_LIMIT"
  echo '(search for TFCI_WHL_SIZE_LIMIT to change it)'
  ls -sh *.whl
  exit 2
fi

# Quick install checks
venv=$(mktemp -d)
"python${TFCI_PYTHON_VERSION}" -m venv "$venv"
python="$venv/bin/python3"
"$python" -m pip install *.whl
"$python" -c 'import tensorflow as tf; t1=tf.constant([1,2,3,4]); t2=tf.constant([5,6,7,8]); print(tf.add(t1,t2).shape)'
"$python" -c 'import sys; import tensorflow as tf; sys.exit(0 if "keras" in tf.keras.__name__ else 1)'

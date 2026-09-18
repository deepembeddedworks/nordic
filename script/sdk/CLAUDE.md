nRF Connect SDK v2.9.1 Workspace Setup

Objective

Set up an nRF Connect SDK v2.9.1 workspace in the current directory.

The required command sequence is:

git clone --branch v2.9.1 https://github.com/nrfconnect/sdk-nrf nrf
west init -l nrf
west update

Execution rules

Execute the commands yourself; do not merely explain them.

Run the commands one at a time and verify the exit status after each command.

Do not proceed to the next step when a command fails. Diagnose the error and report it.

Do not use sudo unless the user explicitly approves it.

Do not delete, overwrite, reset, or modify an existing NCS workspace without explicit approval.

Do not change the requested NCS version from v2.9.1.

Preserve local user changes and unrelated files.

Preflight checks

Before starting, print the current directory and check the required commands:

pwd
git --version
west --version

If git or west is unavailable, stop and explain what is missing. Ask before installing system packages or modifying the Python environment.

Check whether these paths already exist:

test -e nrf && echo "nrf already exists"
test -e .west && echo ".west already exists"

If either path exists, inspect the existing workspace and stop for user confirmation instead of overwriting or reinitializing it.

Step 1: Clone sdk-nrf

Execute:

git clone --branch v2.9.1 https://github.com/nrfconnect/sdk-nrf nrf

After the clone succeeds, verify:

git -C nrf status --short --branch
git -C nrf describe --tags --exact-match

The checked-out tag must be v2.9.1.

Step 2: Initialize the west workspace

From the directory containing nrf, execute:

west init -l nrf

After it succeeds, confirm that .west/config exists:

test -f .west/config

Step 3: Download the west projects

Execute from the workspace root:

west update

This operation may take considerable time and download several repositories. Do not interrupt it merely because it produces no output for a while. If it fails, preserve all downloaded data so that a later west update can resume.

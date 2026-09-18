Download and Prepare the nRF Connect SDK Toolchain

Objective

Download the Linux toolchain compatible with the current host architecture and nRF Connect SDK v2.9.1, extract it, and create an env.sh file for the user.

The user will source env.sh manually. Do not source it automatically and do not modify .bashrc, .profile, or any system file.

Fixed requirements

NCS version: v2.9.1

Expected NCS v2.9.x toolchain bundle ID: b77d8c1312

Bundle browser: https://files.nordicsemi.com/ui/native/NCS/external/bundles/v3/

Downloader: wget -c

Extraction command: tar -xvf

Environment file: env.sh

The correct variable is LD_LIBRARY_PATH, not LED_LIBRARY_PATH.

Do not change the version, bundle ID, server, installation directory, or selected archive without explaining the reason and obtaining user approval.

Safety and execution rules

Execute one stage at a time and check every exit status.

Never use sudo.

Never delete or overwrite an existing archive, extracted toolchain, or env.sh without explicit approval.

Do not guess an artifact URL or architecture name.

Do not continue after an error or an ambiguous artifact search.

Use only an official files.nordicsemi.com HTTPS URL for the tarball.

Do not install packages, modify shell startup files, or build an application.

Step 1: Inspect the host

Run:

pwd
uname -s
uname -m
getconf LONG_BIT
wget --version | head -n 1
tar --version | head -n 1

Accept only Linux. Normalize the uname -m result as follows:

uname -m

Artifact architecture

x86_64

x86_64

aarch64 or arm64

aarch64

For any other value, stop and report that no supported mapping has been established.

Step 2: Resolve the exact official tarball

Inspect the official Nordic bundle directory below without downloading a tarball yet:

https://files.nordicsemi.com/ui/native/NCS/external/bundles/v3/

Find a Linux archive that satisfies all of these conditions:

belongs to the NCS v2.9.1 toolchain;

matches bundle ID b77d8c1312, when the ID is present in the path or metadata;

matches the normalized host architecture;

is a tar archive whose filename begins with or clearly identifies ncs-linux or the corresponding Nordic NCS Linux toolchain bundle;

is listed under https://files.nordicsemi.com/ui/native/NCS/external/bundles/v3/.

Use this bundle listing, Nordic manifest, or checksum metadata to establish the match. The /ui/native/ address is the browser view; when it exposes a separate direct-download URL, use that exact official files.nordicsemi.com URL with wget -c. Do not construct a different repository path by guessing, and do not choose a file merely because its name contains linux.

Before downloading, print:

Detected architecture: ...
Selected NCS version: v2.9.1
Selected bundle ID: b77d8c1312
Selected filename: ...
Selected URL: ...
Download directory: ...
Extraction directory: ...

If there is no unique verified match, stop and ask the user to provide or approve the exact URL. Do not try multiple large downloads.

Step 3: Download with resume support

Create local directories without sudo:

mkdir -p downloads toolchains

Download the selected file using its exact filename:

wget -c -P downloads '<VERIFIED_OFFICIAL_TARBALL_URL>'

Wait until wget exits successfully. A partial .part file or interrupted transfer is not a completed download.

If Nordic supplies a SHA-256 checksum, download or obtain it from official metadata and verify it:

sha256sum downloads/<EXACT_ARCHIVE_FILENAME>

Compare the full hash exactly. Stop on a mismatch. Never extract an archive with a failed checksum.

Also verify that the archive is readable before extraction:

tar -tf downloads/<EXACT_ARCHIVE_FILENAME> >/dev/null

Step 4: Extract the toolchain

First inspect the first archive entries to determine whether the archive already contains a top-level directory:

tar -tf downloads/<EXACT_ARCHIVE_FILENAME> | head -n 20

The final toolchain root should be:

<workspace>/toolchains/b77d8c1312

If that destination already contains files, stop and ask for confirmation. Do not merge or overwrite it.

Create the destination and extract using tar -xvf. Select the correct form based on the archive layout so that an accidental duplicated directory such as b77d8c1312/b77d8c1312 is not created. A typical extraction command is:

mkdir -p toolchains/b77d8c1312
tar -xvf downloads/<EXACT_ARCHIVE_FILENAME> -C toolchains/b77d8c1312

Do not use --strip-components unless the archive listing proves that exactly one common wrapper directory must be removed. Explain the choice before executing it.

Step 5: Create env.sh

Inspect the extracted directory before generating the file:

find toolchains/b77d8c1312 -maxdepth 4 -type f \
  \( -name west -o -name cmake -o -name ninja -o -name python3 -o -name 'arm-zephyr-eabi-gcc' \) \
  -print
find toolchains/b77d8c1312 -maxdepth 4 -type d \
  \( -name bin -o -name lib -o -name lib64 -o -name '*linux-gnu*' \) \
  -print

Generate <workspace>/env.sh. It must:

be sourceable from any current directory;

locate itself using BASH_SOURCE[0];

export an absolute NCS_TOOLCHAIN_ROOT;

prepend only directories that actually exist to PATH;

prepend only directories that actually exist to LD_LIBRARY_PATH;

preserve the user's existing PATH and LD_LIBRARY_PATH;

avoid adding duplicate entries when sourced repeatedly;

contain no hard-coded username or home directory;

print the resulting toolchain root when sourced;

not execute a new shell.

Use this structure, adjusting candidate subdirectories only after inspecting the extracted archive:

#!/usr/bin/env bash

_ncs_env_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export NCS_TOOLCHAIN_ROOT="${_ncs_env_dir}/toolchains/b77d8c1312"

_ncs_prepend_unique() {
    local variable_name="$1"
    local directory="$2"
    local current_value

    [ -d "$directory" ] || return 0
    current_value="${!variable_name-}"

    case ":${current_value}:" in
        *":${directory}:"*) return 0 ;;
    esac

    if [ -n "$current_value" ]; then
        printf -v "$variable_name" '%s:%s' "$directory" "$current_value"
    else
        printf -v "$variable_name" '%s' "$directory"
    fi
    export "$variable_name"
}

_ncs_prepend_unique PATH "${NCS_TOOLCHAIN_ROOT}/usr/local/bin"
_ncs_prepend_unique PATH "${NCS_TOOLCHAIN_ROOT}/opt/zephyr-sdk/arm-zephyr-eabi/bin"

_ncs_prepend_unique LD_LIBRARY_PATH "${NCS_TOOLCHAIN_ROOT}/usr/local/lib"
_ncs_prepend_unique LD_LIBRARY_PATH "${NCS_TOOLCHAIN_ROOT}/usr/local/lib64"

# Add an architecture-specific existing library directory here only if the
# extracted bundle actually contains it.

unset -f _ncs_prepend_unique
unset _ncs_env_dir

echo "NCS toolchain environment: ${NCS_TOOLCHAIN_ROOT}"

Do not blindly copy nonexistent paths from the template. If the bundle contains an official environment/setup script, inspect it and ensure that env.sh includes or sources the necessary settings while still explicitly setting PATH and LD_LIBRARY_PATH as requested.

Make the generated script executable, but do not source it:

chmod u+x env.sh
bash -n env.sh

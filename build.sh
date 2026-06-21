#!/usr/bin/env bash
#
# Build the EnhancedInput plugin against a Unigine SDK -- no empty_cpp project.
# The plugin is configured standalone from source/plugins/Ryutp/EnhancedInput;
# the engine is referenced from the SDK and the .so files land in
# <repo>/bin/plugins/Ryutp/EnhancedInput.
#
# Builds every combination: Debug/Release x float/double.
# Generator Ninja, all cores, always-clean, fail fast.
#
# SDK resolution (first hit wins):
#   1. $UNIGINE_SDK_PATH (a valid SDK dir)
#   2. --sdk <id|path>
#   3. SDK Browser registry: the only installed SDK, else interactive picker.
#
# Editor plugin needs Qt 6.5.3 -- $UNIGINE_QTROOT must point at it.

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_SRC="$REPO_DIR/source/plugins/Ryutp/EnhancedInput"
BUILD_ROOT="$REPO_DIR/build"
BROWSER_JSON="${XDG_CONFIG_HOME:-$HOME/.config}/unigine/browser.json"
JOBS="$(nproc)"
SDK_ARG=""

while [ $# -gt 0 ]; do
	case "$1" in
		--sdk) SDK_ARG="$2"; shift 2 ;;
		-*)    echo "Unknown option: $1" >&2; exit 1 ;;
		*)     echo "Unexpected arg: $1" >&2; exit 1 ;;
	esac
done

is_sdk() { [ -f "$1/include/UnigineEngine.h" ]; }

# id<TAB>path for every installed SDK, default first.
sdk_list() {
	python3 - "$BROWSER_JSON" <<'PY'
import json,sys
d=json.load(open(sys.argv[1]))
sdk=d.get("sdk",{}); inst=sdk.get("installed",{}); default=sdk.get("default")
for k in sorted(inst, key=lambda k:(k!=default, k)):
    print(f"{k}\t{inst[k]}")
PY
}

resolve_sdk_path() {
	# 1. env
	if [ -n "${UNIGINE_SDK_PATH:-}" ] && is_sdk "$UNIGINE_SDK_PATH"; then
		printf '%s\n' "${UNIGINE_SDK_PATH%/}"; return 0
	fi
	# 2. --sdk as a path
	if [ -n "$SDK_ARG" ] && is_sdk "$SDK_ARG"; then
		printf '%s\n' "${SDK_ARG%/}"; return 0
	fi
	# 3. registry
	[ -f "$BROWSER_JSON" ] || { echo "ERROR: no SDK: set \$UNIGINE_SDK_PATH, pass --sdk <dir>, or install one in the SDK Browser." >&2; return 1; }
	command -v python3 >/dev/null || { echo "ERROR: python3 required to read $BROWSER_JSON (or pass --sdk <dir>)." >&2; return 1; }
	local lines; lines="$(sdk_list)"
	[ -n "$lines" ] || { echo "ERROR: no installed SDKs in $BROWSER_JSON." >&2; return 1; }
	if [ -n "$SDK_ARG" ]; then            # --sdk as an id
		local p; p="$(awk -F'\t' -v id="$SDK_ARG" '$1==id{print $2}' <<<"$lines")"
		[ -n "$p" ] || { echo "ERROR: --sdk '$SDK_ARG' not found. Installed:" >&2; awk -F'\t' '{print "  "$1}' <<<"$lines" >&2; return 1; }
		printf '%s\n' "${p%/}"; return 0
	fi
	if [ "$(wc -l <<<"$lines")" -eq 1 ]; then awk -F'\t' '{print $2}' <<<"$lines"; return 0; fi
	[ -r /dev/tty ] || { echo "ERROR: several SDKs installed; pass --sdk <id> (no TTY for picker)." >&2; awk -F'\t' '{print "  "$1}' <<<"$lines" >&2; return 1; }
	local ids=() paths=() id path i=1
	echo "Select SDK:" >&2
	while IFS=$'\t' read -r id path; do echo "  $i) $id  ($(basename "$path"))" >&2; ids+=("$id"); paths+=("$path"); i=$((i+1)); done <<<"$lines"
	local sel; read -r -p "SDK [1]: " sel </dev/tty; sel="${sel:-1}"
	local idx=$((sel-1))
	[ "$idx" -ge 0 ] && [ "$idx" -lt "${#paths[@]}" ] 2>/dev/null || { echo "ERROR: invalid selection." >&2; return 1; }
	printf '%s\n' "${paths[$idx]%/}"
}

SDK_PATH="$(resolve_sdk_path)" || exit 1
is_sdk "$SDK_PATH" || { echo "ERROR: '$SDK_PATH' is not a valid SDK." >&2; exit 1; }
echo "SDK: $SDK_PATH"

# build_type  precision  unigine_double
CONFIGS=(
	"Debug   double 1"
	"Release double 1"
	"Debug   float  0"
	"Release float  0"
)

for cfg in "${CONFIGS[@]}"; do
	read -r build_type precision dbl <<<"$cfg"
	build_dir="$BUILD_ROOT/${build_type}-${precision}"

	echo "=============================================================="
	echo " ${build_type} / ${precision}"
	echo "=============================================================="

	rm -rf "$build_dir"
	cmake -S "$PLUGIN_SRC" -B "$build_dir" -G Ninja \
		-DCMAKE_BUILD_TYPE="$build_type" \
		-DUNIGINE_DOUBLE="$dbl" \
		-DUNIGINE_SDK_PATH="$SDK_PATH/" \
		-DUNIGINE_BIN_DIR="$REPO_DIR/bin" \
		-DUNIGINE_LIB_DIR="$SDK_PATH/bin" \
		-DUNIGINE_INCLUDE_DIR="$SDK_PATH/include;$REPO_DIR/include"
	cmake --build "$build_dir" -j "$JOBS"
done

echo "All builds succeeded. Plugins -> $REPO_DIR/bin/plugins/Ryutp/EnhancedInput"

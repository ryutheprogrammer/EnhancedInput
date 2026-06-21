#!/usr/bin/env bash
#
# Build-machine project: a minimal Unigine empty_cpp source tree that REFERENCES
# a chosen installed SDK and carries the EnhancedInput plugin. Compiles only --
# no engine data, no .umount, no launch scripts (the artifact is the plugin .so;
# the project is never run here).
#
#   make_project.sh <target_dir> [--sdk <id>] [--from <seed_project>]
#                                [--no-pull] [--no-build]
#
#   - SDK is taken from the SDK Browser registry (browser.json -> sdk.installed):
#       --sdk <id>  | the only installed SDK | interactive picker when several.
#   - only source/ is cloned (app skeleton + cmake modules); engine INCLUDE+LIBS
#     are referenced via UNIGINE_SDK_PATH in CMakeLists. Nothing heavy copied.
#   - plugin sources are refreshed (git pull) then overlaid; build via build.sh.
#   - on an EXISTING target: the source skeleton is kept; only the SDK reference
#     and the plugin are refreshed.

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_REL="plugins/Ryutp/EnhancedInput"
BROWSER_JSON="${XDG_CONFIG_HOME:-$HOME/.config}/unigine/browser.json"

SDK_ID=""
SEED=""
DO_BUILD=1
DO_PULL=1
TARGET=""

usage() { echo "Usage: $0 <target_dir> [--sdk <id>] [--from <seed_project>] [--no-pull] [--no-build]" >&2; exit 1; }

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

# echoes "id<TAB>path" for the chosen SDK
resolve_sdk() {
	local lines; lines="$(sdk_list)"
	[ -n "$lines" ] || { echo "ERROR: no installed SDKs in $BROWSER_JSON" >&2; return 1; }
	if [ -n "$SDK_ID" ]; then
		local p; p="$(awk -F'\t' -v id="$SDK_ID" '$1==id{print $2}' <<<"$lines")"
		[ -n "$p" ] || { echo "ERROR: --sdk '$SDK_ID' not installed. Available:" >&2
			awk -F'\t' '{print "  "$1}' <<<"$lines" >&2; return 1; }
		printf '%s\t%s\n' "$SDK_ID" "$p"; return 0
	fi
	if [ "$(wc -l <<<"$lines")" -eq 1 ]; then printf '%s\n' "$lines"; return 0; fi
	[ -r /dev/tty ] || { echo "ERROR: several SDKs installed; pass --sdk <id> (no TTY for picker)." >&2
		awk -F'\t' '{print "  "$1}' <<<"$lines" >&2; return 1; }
	local ids=() paths=() id path i=1
	echo "Select SDK:" >&2
	while IFS=$'\t' read -r id path; do
		echo "  $i) $id" >&2; ids+=("$id"); paths+=("$path"); i=$((i+1))
	done <<<"$lines"
	local sel; read -r -p "SDK [1]: " sel </dev/tty; sel="${sel:-1}"
	local idx=$((sel-1))
	[ "$idx" -ge 0 ] && [ "$idx" -lt "${#ids[@]}" ] 2>/dev/null || { echo "ERROR: invalid selection" >&2; return 1; }
	printf '%s\t%s\n' "${ids[$idx]}" "${paths[$idx]}"
}

# a clean, self-contained empty_cpp project to clone the skeleton from
discover_seed() {
	local pf dir
	[ -f "$BROWSER_JSON" ] || return 1
	while IFS= read -r pf; do
		dir="$(dirname "$pf")"
		if [ "$(basename "$dir")" = "empty_cpp" ] && [ -f "$dir/source/CMakeLists.txt" ]; then
			printf '%s\n' "$dir"; return 0
		fi
	done < <(grep -oE '"filepath"[[:space:]]*:[[:space:]]*"[^"]*"' "$BROWSER_JSON" | sed -E 's/.*"([^"]*)".*/\1/')
	return 1
}

while [ $# -gt 0 ]; do
	case "$1" in
		--sdk)      SDK_ID="$2"; shift 2 ;;
		--from)     SEED="$2"; shift 2 ;;
		--no-pull)  DO_PULL=0; shift ;;
		--no-build) DO_BUILD=0; shift ;;
		-*)         echo "Unknown option: $1" >&2; usage ;;
		*)          [ -z "$TARGET" ] || { echo "Unexpected arg: $1" >&2; usage; }
		            TARGET="$1"; shift ;;
	esac
done

[ -n "$TARGET" ] || usage
[ -f "$BROWSER_JSON" ] || { echo "ERROR: SDK Browser registry not found at $BROWSER_JSON." >&2; exit 1; }
command -v python3 >/dev/null || { echo "ERROR: python3 is required to read $BROWSER_JSON." >&2; exit 1; }
for d in "source/$PLUGIN_REL" "include/$PLUGIN_REL" "bin/$PLUGIN_REL"; do
	[ -d "$REPO_DIR/$d" ] || { echo "ERROR: deliverable missing: $d" >&2; exit 1; }
done

IFS=$'\t' read -r SDK_ID SDK_PATH < <(resolve_sdk)
[ -n "${SDK_PATH:-}" ] || { echo "ERROR: could not resolve an SDK." >&2; exit 1; }
[ -f "$SDK_PATH/include/UnigineEngine.h" ] || { echo "ERROR: '$SDK_PATH' is not a valid SDK (no include/UnigineEngine.h)." >&2; exit 1; }
echo "== SDK:     $SDK_ID"
echo "            $SDK_PATH"

# --- skeleton: clone only source/ (fresh target; engine is referenced) ---
if [ ! -e "$TARGET" ]; then
	if [ -z "$SEED" ]; then
		SEED="$(discover_seed)" || { echo "ERROR: no 'empty_cpp' seed project in the registry; pass --from <project_dir>." >&2; exit 1; }
		echo "== seed:    $SEED (source/ only)"
	fi
	[ -d "$SEED/source" ] || { echo "ERROR: '$SEED' is not an empty_cpp project (no source/)." >&2; exit 1; }
	mkdir -p "$TARGET/source" "$TARGET/bin"
	rsync -a --exclude='build/' "$SEED/source"/ "$TARGET/source"/
else
	echo "== existing target: keeping source skeleton, refreshing SDK ref + plugin"
	rm -rf "$TARGET/source/$PLUGIN_REL" "$TARGET/include/$PLUGIN_REL" "$TARGET/bin/$PLUGIN_REL"
fi

# --- patch source/CMakeLists.txt ---
echo "== patch:   source/CMakeLists.txt"
CML="$TARGET/source/CMakeLists.txt"
sed -i "s#^set(UNIGINE_SDK_PATH .*#set(UNIGINE_SDK_PATH $SDK_PATH/)#" "$CML"
sed -i 's/^set(UNIGINE_DOUBLE .*/if(NOT DEFINED UNIGINE_DOUBLE)\n\tset(UNIGINE_DOUBLE 1)\nendif()/' "$CML"
grep -q ':_double>)' "$CML" || \
	sed -i '/string(APPEND binary_name "_x64")/i string(APPEND binary_name $<$<BOOL:${UNIGINE_DOUBLE}>:_double>)' "$CML"
grep -q "add_subdirectory($PLUGIN_REL)" "$CML" || \
	printf '\n    add_subdirectory(%s)\n' "$PLUGIN_REL" >> "$CML"

# --- plugin: pull then overlay ---
if [ "$DO_PULL" -eq 1 ]; then
	echo "== pull:    $REPO_DIR"
	git -C "$REPO_DIR" pull --ff-only || echo "WARN: git pull failed; overlaying current working tree." >&2
fi
echo "== overlay: EnhancedInput plugin"
for d in "source/$PLUGIN_REL" "include/$PLUGIN_REL" "bin/$PLUGIN_REL"; do
	mkdir -p "$TARGET/$d"
	rsync -a --delete "$REPO_DIR/$d"/ "$TARGET/$d"/
done
cp "$REPO_DIR/build.sh" "$REPO_DIR/build.ps1" "$TARGET"/

# --- build ---
if [ "$DO_BUILD" -eq 1 ]; then
	echo "== build:   $TARGET"
	( cd "$TARGET" && ./build.sh )
fi

echo "Project ready: $TARGET  (SDK: $SDK_ID)"

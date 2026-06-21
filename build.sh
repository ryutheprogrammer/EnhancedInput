set -euo pipefail

SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/source" && pwd)"
BUILD_ROOT="${SRC_DIR}/build"
JOBS="$(nproc)"

CONFIGS=(
	"Debug   double 1"
	"Release double 1"
	"Debug   float  0"
	"Release float  0"
)

for cfg in "${CONFIGS[@]}"; do
	read -r build_type precision dbl <<<"$cfg"
	build_dir="${BUILD_ROOT}/${build_type}-${precision}"

	echo "=============================================================="
	echo " ${build_type} / ${precision}  ->  ${build_dir}"
	echo "=============================================================="

	rm -rf "$build_dir"

	cmake -S "$SRC_DIR" -B "$build_dir" \
		-G Ninja \
		-DCMAKE_BUILD_TYPE="$build_type" \
		-DUNIGINE_DOUBLE="$dbl"

	cmake --build "$build_dir" -j "$JOBS"
done

echo "All builds succeeded."

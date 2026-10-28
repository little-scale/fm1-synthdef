# Source from the Bash scripts in this directory.
FM1_WORKSPACE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$FM1_WORKSPACE/.venv/bin:$PATH"
# Upstream expands PYTHON inside sh -c without quotes, so use a PATH-resolved name.
export PYTHON=python3
export CC=/usr/bin/clang
export AC79_SDK="$FM1_WORKSPACE/dependencies/ac79-sdk"
export JIELI_TOOLCHAIN="$FM1_WORKSPACE/dependencies/jieli/jieli-linux-toolchains-20260730.1"
export JIELI_DOCKER=1
if [[ -f "$FM1_WORKSPACE/setup/docker-image.txt" ]]; then
    export JIELI_DOCKER_IMAGE="$(cat "$FM1_WORKSPACE/setup/docker-image.txt")"
fi
mkdir -p "$FM1_WORKSPACE/logs"

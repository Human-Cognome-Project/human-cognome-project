#!/usr/bin/env bash
# HCP Source Workstation — Build Configuration
#
# make-style CLI frontend wrapping CMake options.
# Run this before building to configure your workstation.
#
# Usage:
#   ./configure.sh              # Interactive menu
#   ./configure.sh --defaults   # Use all defaults (postgres, GPU auto-detect, core vocab)
#   ./configure.sh --sqlite --cpu --vocab=english
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build/linux"

# ---- Defaults ----
DB_BACKEND="postgres"
GPU_MODE="auto"
VOCAB_PACK="core"
MODULES="source"       # source is always included
BUILD_TYPE="profile"

# ---- Color helpers ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

header() { echo -e "\n${BOLD}${CYAN}=== $1 ===${NC}\n"; }
info()   { echo -e "  ${GREEN}*${NC} $1"; }
warn()   { echo -e "  ${YELLOW}!${NC} $1"; }
err()    { echo -e "  ${RED}X${NC} $1"; }

# ---- GPU Detection ----
detect_gpu() {
    local has_nvidia=false
    local has_cuda=false

    if command -v nvidia-smi &>/dev/null; then
        if nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null | head -1 | grep -qi "nvidia"; then
            has_nvidia=true
        fi
    fi

    if [ "$has_nvidia" = true ]; then
        if command -v nvcc &>/dev/null; then
            has_cuda=true
        elif [ -d "/usr/local/cuda" ]; then
            has_cuda=true
        fi
    fi

    if [ "$has_nvidia" = true ] && [ "$has_cuda" = true ]; then
        echo "gpu"
    elif [ "$has_nvidia" = true ]; then
        echo "gpu_no_cuda"
    else
        echo "cpu"
    fi
}

# ---- PostgreSQL Detection ----
detect_postgres() {
    if command -v pg_isready &>/dev/null && pg_isready -q 2>/dev/null; then
        echo "available"
    elif [ -f /usr/lib/x86_64-linux-gnu/libpq.so ]; then
        echo "lib_only"
    else
        echo "none"
    fi
}

# ---- Parse CLI arguments ----
INTERACTIVE=true
for arg in "$@"; do
    case "$arg" in
        --defaults)
            INTERACTIVE=false
            ;;
        --sqlite)
            DB_BACKEND="sqlite"
            INTERACTIVE=false
            ;;
        --postgres)
            DB_BACKEND="postgres"
            INTERACTIVE=false
            ;;
        --cpu)
            GPU_MODE="cpu"
            INTERACTIVE=false
            ;;
        --gpu)
            GPU_MODE="gpu"
            INTERACTIVE=false
            ;;
        --vocab=*)
            VOCAB_PACK="${arg#*=}"
            INTERACTIVE=false
            ;;
        --build-type=*)
            BUILD_TYPE="${arg#*=}"
            INTERACTIVE=false
            ;;
        --help|-h)
            echo "HCP Source Workstation Configuration"
            echo ""
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --defaults        Use default settings (non-interactive)"
            echo "  --sqlite          Use SQLite database backend (default: postgres)"
            echo "  --postgres        Use PostgreSQL database backend"
            echo "  --cpu             Force CPU-only mode"
            echo "  --gpu             Force GPU mode (requires CUDA)"
            echo "  --vocab=PACK      Vocabulary pack: core, english, custom"
            echo "  --build-type=TYPE Build type: debug, profile, release (default: profile)"
            echo "  --help            Show this help"
            exit 0
            ;;
    esac
done

# ---- Interactive menu ----
if [ "$INTERACTIVE" = true ]; then
    echo -e "${BOLD}HCP Source Workstation — Build Configuration${NC}"
    echo ""

    # Database backend
    header "Database Backend"
    PG_STATUS=$(detect_postgres)
    case "$PG_STATUS" in
        available)
            info "PostgreSQL detected and running"
            ;;
        lib_only)
            warn "PostgreSQL library found but server not running"
            ;;
        none)
            warn "PostgreSQL not detected — SQLite recommended"
            ;;
    esac

    echo "  1) PostgreSQL (production, requires running server)"
    echo "  2) SQLite (bundled, no server required)"
    read -rp "  Select database backend [1]: " db_choice
    case "${db_choice:-1}" in
        2) DB_BACKEND="sqlite" ;;
        *) DB_BACKEND="postgres" ;;
    esac

    # GPU acceleration
    header "GPU Acceleration"
    GPU_STATUS=$(detect_gpu)
    case "$GPU_STATUS" in
        gpu)
            info "NVIDIA GPU + CUDA toolkit detected"
            echo "  1) GPU mode (recommended)"
            echo "  2) CPU-only mode"
            read -rp "  Select GPU mode [1]: " gpu_choice
            case "${gpu_choice:-1}" in
                2) GPU_MODE="cpu" ;;
                *) GPU_MODE="gpu" ;;
            esac
            ;;
        gpu_no_cuda)
            warn "NVIDIA GPU detected but CUDA toolkit not found"
            warn "Install CUDA for GPU acceleration, or use CPU mode"
            echo "  1) CPU-only mode"
            echo "  2) GPU mode (will attempt to find CUDA)"
            read -rp "  Select GPU mode [1]: " gpu_choice
            case "${gpu_choice:-1}" in
                2) GPU_MODE="gpu" ;;
                *) GPU_MODE="cpu" ;;
            esac
            ;;
        cpu)
            info "No NVIDIA GPU detected — using CPU mode"
            GPU_MODE="cpu"
            ;;
    esac

    # Vocabulary pack
    header "Vocabulary Pack"
    echo "  1) Core only (~2MB — structural tokens, labels)"
    echo "  2) English pack (~934MB — full hcp_english dictionary)"
    echo "  3) Custom shard selection"
    read -rp "  Select vocabulary [1]: " vocab_choice
    case "${vocab_choice:-1}" in
        2) VOCAB_PACK="english" ;;
        3) VOCAB_PACK="custom" ;;
        *) VOCAB_PACK="core" ;;
    esac

    # Build type
    header "Build Type"
    echo "  1) Profile (default — optimized + debug symbols)"
    echo "  2) Debug (no optimization, full debug)"
    echo "  3) Release (full optimization, no debug)"
    read -rp "  Select build type [1]: " bt_choice
    case "${bt_choice:-1}" in
        2) BUILD_TYPE="debug" ;;
        3) BUILD_TYPE="release" ;;
        *) BUILD_TYPE="profile" ;;
    esac
fi

# ---- Auto-detect GPU if mode is auto ----
if [ "$GPU_MODE" = "auto" ]; then
    GPU_STATUS=$(detect_gpu)
    if [ "$GPU_STATUS" = "gpu" ]; then
        GPU_MODE="gpu"
    else
        GPU_MODE="cpu"
    fi
fi

# ---- Summary ----
header "Configuration Summary"
info "Database:    ${DB_BACKEND}"
info "GPU:         ${GPU_MODE}"
info "Vocabulary:  ${VOCAB_PACK}"
info "Build type:  ${BUILD_TYPE}"
info "Build dir:   ${BUILD_DIR}"

# ---- Generate CMake configuration ----
CMAKE_ARGS=(
    "-S" "${SCRIPT_DIR}"
    "-B" "${BUILD_DIR}"
    "-DHCP_BUILD_WORKSTATION=ON"
)

# Database backend
if [ "$DB_BACKEND" = "sqlite" ]; then
    CMAKE_ARGS+=("-DHCP_DB_BACKEND=sqlite")
else
    CMAKE_ARGS+=("-DHCP_DB_BACKEND=postgres")
fi

# GPU mode — PhysX dispatch file
if [ "$GPU_MODE" = "gpu" ]; then
    CMAKE_ARGS+=("-DHCP_GPU_MODE=ON")
    # Create PhysX GPU dispatch marker (engine checks file presence)
    mkdir -p "${BUILD_DIR}"
    touch "${BUILD_DIR}/physx_gpu_dispatch"
    info "GPU dispatch file created: ${BUILD_DIR}/physx_gpu_dispatch"
else
    CMAKE_ARGS+=("-DHCP_GPU_MODE=OFF")
    rm -f "${BUILD_DIR}/physx_gpu_dispatch" 2>/dev/null || true
fi

# Build type
case "$BUILD_TYPE" in
    debug)   CMAKE_ARGS+=("-DCMAKE_BUILD_TYPE=Debug") ;;
    release) CMAKE_ARGS+=("-DCMAKE_BUILD_TYPE=Release") ;;
    *)       CMAKE_ARGS+=("-DCMAKE_BUILD_TYPE=RelWithDebInfo") ;;
esac

# ---- Write config cache file ----
mkdir -p "${BUILD_DIR}"
cat > "${BUILD_DIR}/hcp_config.cmake" << EOF
# HCP Workstation Build Configuration
# Generated by configure.sh — do not edit manually
set(HCP_DB_BACKEND "${DB_BACKEND}" CACHE STRING "Database backend")
set(HCP_GPU_MODE ${GPU_MODE^^} CACHE BOOL "GPU acceleration")
set(HCP_VOCAB_PACK "${VOCAB_PACK}" CACHE STRING "Vocabulary pack")
set(HCP_BUILD_TYPE "${BUILD_TYPE}" CACHE STRING "Build type")
EOF

info "Configuration written to: ${BUILD_DIR}/hcp_config.cmake"
echo ""

# ---- Print build instructions ----
header "Next Steps"
echo "  To build the workstation:"
echo ""
echo "    cd ${BUILD_DIR}"
echo "    cmake --build . --target HCPEngine.Workstation --config ${BUILD_TYPE} -j\$(nproc)"
echo ""
echo "  Or use the O3DE build system:"
echo ""
echo "    cmake ${CMAKE_ARGS[*]}"
echo "    cmake --build ${BUILD_DIR} --target HCPEngine.Workstation -j\$(nproc)"
echo ""

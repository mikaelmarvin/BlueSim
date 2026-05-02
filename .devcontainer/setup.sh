#!/bin/bash
set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"
NCS_VERSION="${NCS_VERSION:-v2.6.1}"
NCS_IMAGE_ROOT="/opt/ncs/${NCS_VERSION}"
NCS_WORKSPACE_LINK="/workspace/ncs"
NCS_ROOT="${NCS_WORKSPACE_LINK}/${NCS_VERSION}"

ensure_starship_bashrc() {
    local line='[ -n "$BASH_VERSION" ] && command -v starship >/dev/null 2>&1 && eval "$(starship init bash)"'
    if ! grep -q 'starship init bash' "${HOME}/.bashrc" 2>/dev/null; then
        echo "" >> "${HOME}/.bashrc"
        echo "# Starship prompt (BlueSim devcontainer)" >> "${HOME}/.bashrc"
        echo "$line" >> "${HOME}/.bashrc"
        echo "Added Starship to ~/.bashrc"
    fi
}

ensure_starship_config() {
    local config_dir="${HOME}/.config"
    local config_file="${config_dir}/starship.toml"
    local begin_marker="# BlueSim managed: disable cmake module - begin"
    local end_marker="# BlueSim managed: disable cmake module - end"

    mkdir -p "${config_dir}"
    touch "${config_file}"

    if ! grep -q "${begin_marker}" "${config_file}" 2>/dev/null; then
        cat >> "${config_file}" <<EOF

${begin_marker}
[cmake]
disabled = true
${end_marker}
EOF
        echo "Added CMake disable block to ${config_file}"
    fi
}

ensure_ncs_bashrc() {
    if ! grep -q 'BlueSim NCS environment (workspace link)' "${HOME}/.bashrc" 2>/dev/null; then
        cat >> "${HOME}/.bashrc" <<EOF

# BlueSim NCS environment (workspace link)
export NCS_VERSION="${NCS_VERSION}"
export NCS_ROOT="/workspace/ncs/\${NCS_VERSION}"
if [[ -d "\${NCS_ROOT}/zephyr" ]]; then
    export ZEPHYR_BASE="\${NCS_ROOT}/zephyr"
    fi
EOF
        echo "Added NCS environment exports to ~/.bashrc"
    fi
}

ensure_workspace_ncs_link() {
    mkdir -p "$(dirname "${NCS_WORKSPACE_LINK}")"
    if [[ -e "${NCS_WORKSPACE_LINK}" && ! -L "${NCS_WORKSPACE_LINK}" ]]; then
        echo "Warning: ${NCS_WORKSPACE_LINK} exists and is not a symlink; keeping existing path."
        return 0
    fi
    ln -sfn "/opt/ncs" "${NCS_WORKSPACE_LINK}"
}

# Fail fast if NCS is half-initialized (common after interrupted west update).
# Without this, postCreate can "succeed" while west build is permanently missing.
verify_ncs_workspace() {
    local root="${NCS_IMAGE_ROOT}"
    if [[ ! -d "${root}/.west" ]]; then
        echo "ERROR: No west workspace at ${root} (.west missing)." >&2
        return 1
    fi
    if [[ ! -f "${root}/zephyr/scripts/west-commands.yml" || ! -f "${root}/zephyr/CMakeLists.txt" ]]; then
        echo "ERROR: Zephyr tree at ${root}/zephyr looks incomplete (interrupted west update?)." >&2
        return 1
    fi
    cd "${root}"
    if ! west list >/dev/null 2>&1; then
        echo "ERROR: west list failed — manifest or imported projects are incomplete. Run: cd ${root} && west update" >&2
        return 1
    fi
    if ! west build -h >/dev/null 2>&1; then
        echo "ERROR: west build is not available (Zephyr west extensions not loaded)." >&2
        return 1
    fi
    cd "${REPO_ROOT}"
    return 0
}

setup_pinned_ncs() {
    echo "west: $(west --version 2>/dev/null || echo 'not found')"
    echo "Ensuring nRF Connect SDK ${NCS_VERSION} in ${NCS_IMAGE_ROOT}"

    mkdir -p "${NCS_IMAGE_ROOT}"
    ensure_workspace_ncs_link

    if [[ ! -d "${NCS_IMAGE_ROOT}/.west" ]]; then
        west init -m https://github.com/nrfconnect/sdk-nrf --mr "${NCS_VERSION}" "${NCS_IMAGE_ROOT}"
    fi

    cd "${NCS_IMAGE_ROOT}"
    west update
    west zephyr-export

    if [[ -f "zephyr/scripts/requirements.txt" ]]; then
        pip3 install --no-cache-dir -r zephyr/scripts/requirements.txt
    fi
    if [[ -f "nrf/scripts/requirements.txt" ]]; then
        pip3 install --no-cache-dir -r nrf/scripts/requirements.txt
    fi

    cd "${REPO_ROOT}"
    verify_ncs_workspace
    echo "Pinned NCS ready: ${NCS_ROOT}"
}

# Idempotent: no-op when healthy; runs west update when a previous run left a broken tree.
repair_ncs_workspace_if_needed() {
    mkdir -p "${NCS_IMAGE_ROOT}"
    ensure_workspace_ncs_link

    if [[ ! -d "${NCS_IMAGE_ROOT}/.west" ]]; then
        echo "NCS west workspace missing; running full bootstrap..."
        setup_pinned_ncs
        return
    fi

    if verify_ncs_workspace; then
        echo "NCS workspace OK (${NCS_IMAGE_ROOT})"
        return 0
    fi

    echo "NCS workspace incomplete; running west update + west zephyr-export (this may take a while)..."
    cd "${NCS_IMAGE_ROOT}"
    west update
    west zephyr-export
    cd "${REPO_ROOT}"
    verify_ncs_workspace
}

if [[ "${1:-}" == "--post-start" ]]; then
    ensure_starship_bashrc || true
    ensure_starship_config || true
    ensure_ncs_bashrc || true
    ensure_workspace_ncs_link || true
    repair_ncs_workspace_if_needed
    exit 0
fi

ensure_starship_bashrc || true
ensure_starship_config || true
ensure_ncs_bashrc || true
ensure_workspace_ncs_link || true
setup_pinned_ncs
echo "Devcontainer setup done."

#!/bin/bash
set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"
# Pinned NCS tag for this repo (override with NCS_VERSION when running setup, or set in compose/CI).
NCS_VERSION="${NCS_VERSION:-v2.6.1}"
# Persist on dev-home volume (not under /opt — that is lost when the container is recreated).
NCS_WEST_ROOT="${HOME}/ncs/${NCS_VERSION}"
NCS_WORKSPACE_LINK="/workspace/ncs"

# Optional: NCS_FORCE_UPDATE=1 or NCS_REPAIR=1 to re-run a full west update.
if [[ "${NCS_REPAIR:-}" == "1" ]]; then
    export NCS_FORCE_UPDATE=1
fi

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

# Starship + NCS exports for the dev user (postCreate and post-start).
ensure_user_shell_extras() {
    ensure_starship_bashrc || true
    ensure_starship_config || true
    ensure_ncs_bashrc || true
}

ensure_workspace_ncs_link() {
    mkdir -p "$(dirname "${NCS_WORKSPACE_LINK}")"
    if [[ -e "${NCS_WORKSPACE_LINK}" && ! -L "${NCS_WORKSPACE_LINK}" ]]; then
        echo "Warning: ${NCS_WORKSPACE_LINK} exists and is not a symlink; keeping existing path."
        return 0
    fi
    ln -sfn "${HOME}/ncs" "${NCS_WORKSPACE_LINK}"
}

verify_ncs_workspace() {
    local root="${NCS_WEST_ROOT}"
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
        echo "ERROR: west list failed — manifest or imported projects are incomplete." >&2
        return 1
    fi
    if ! west build -h >/dev/null 2>&1; then
        echo "ERROR: west build is not available (Zephyr west extensions not loaded)." >&2
        return 1
    fi
    cd "${REPO_ROOT}"
    return 0
}

install_ncs_python_deps() {
    cd "${NCS_WEST_ROOT}"
    if [[ -f "zephyr/scripts/requirements.txt" ]]; then
        pip3 install --no-cache-dir -r zephyr/scripts/requirements.txt
    fi
    if [[ -f "nrf/scripts/requirements.txt" ]]; then
        pip3 install --no-cache-dir -r nrf/scripts/requirements.txt
    fi
    cd "${REPO_ROOT}"
}

# Full bootstrap / repair (postCreate, or manual with NCS_REPAIR=1). Runs west update only when needed.
setup_pinned_ncs() {
    echo "west: $(west --version 2>/dev/null || echo 'not found')"
    echo "NCS ${NCS_VERSION} west root: ${NCS_WEST_ROOT} (persistent under ~/ncs)"

    mkdir -p "${HOME}/ncs"
    ensure_workspace_ncs_link

    local need_update=0
    if [[ ! -d "${NCS_WEST_ROOT}/.west" ]]; then
        west init -m https://github.com/nrfconnect/sdk-nrf --mr "${NCS_VERSION}" "${NCS_WEST_ROOT}"
        need_update=1
    elif [[ "${NCS_FORCE_UPDATE:-}" == "1" ]]; then
        need_update=1
    elif ! verify_ncs_workspace 2>/dev/null; then
        echo "Workspace incomplete; running west update once to repair."
        need_update=1
    fi

    cd "${NCS_WEST_ROOT}"
    if [[ "${need_update}" == "1" ]]; then
        west update
        install_ncs_python_deps
    else
        echo "NCS already complete — skipping west update (saves time and disk churn)."
    fi
    west zephyr-export
    cd "${REPO_ROOT}"
    verify_ncs_workspace
    echo "Pinned NCS ready (symlink): ${NCS_WORKSPACE_LINK}/${NCS_VERSION}"
}

# Every container start: fast — no west update, no heavy I/O.
post_start_light() {
    ensure_workspace_ncs_link || true
    if verify_ncs_workspace 2>/dev/null; then
        echo "NCS workspace OK (${NCS_WEST_ROOT})"
    else
        echo "NCS workspace missing or incomplete under ${NCS_WEST_ROOT}." >&2
        echo "First time: rebuild the dev container (postCreate), or run:" >&2
        echo "  NCS_REPAIR=1 bash .devcontainer/setup.sh" >&2
        echo "To refresh all west projects intentionally:" >&2
        echo "  NCS_FORCE_UPDATE=1 bash .devcontainer/setup.sh" >&2
    fi
}

if [[ "${1:-}" == "--post-start" ]]; then
    ensure_user_shell_extras
    post_start_light || true
    exit 0
fi

ensure_user_shell_extras
setup_pinned_ncs
echo "Devcontainer setup done."

# Completion script for LK (Little Kernel) projects, bash implementation.

_lk_completion_bash() {
    local cur projects
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"

    # Check if we're in an LK-like directory (has project/ and a makefile mentioning engine.mk)
    if [[ ! -d project || ! -f makefile ]] || ! grep -q "engine.mk" makefile 2>/dev/null; then
        return 0
    fi

    # Handle PROJECT=... completion
    if [[ "$cur" == PROJECT=* ]]; then
        local p_prefix="${cur#PROJECT=}"
        projects=$(find project -maxdepth 1 -name "*.mk" -exec basename {} .mk \; 2>/dev/null)
        COMPREPLY=( $(compgen -W "${projects}" -P "PROJECT=" -- "${p_prefix}") )
        return 0
    fi

    # List of all .mk files in project/
    projects=$(find project -maxdepth 1 -name "*.mk" -exec basename {} .mk \; 2>/dev/null)

    # Common make targets from make/help.mk
    local common_targets="help list clean spotless tags list-arch list-toolchain"
    local all_targets="${projects} ${common_targets}"

    COMPREPLY=( $(compgen -W "${all_targets}" -- "${cur}") )
    return 0
}

complete -F _lk_completion_bash make

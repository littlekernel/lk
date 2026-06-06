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

# Completion for do-qemu* scripts
_lk_qemu_completion_bash() {
    local cur prev projects page_sizes gic_versions flags_with_args
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    # Check if we're in an LK-like directory
    if [[ ! -d project || ! -f makefile ]] || ! grep -q "engine.mk" makefile 2>/dev/null; then
        return 0
    fi

    # Flags that take arguments
    flags_with_args=(-p -P -m -s -A -d -f -G)

    # If previous word is a flag that takes an argument, provide completions
    case "$prev" in
        -p)
            # Project name completion
            projects=$(find project -maxdepth 1 -name "*.mk" -exec basename {} .mk \; 2>/dev/null | sort)
            COMPREPLY=( $(compgen -W "${projects}" -- "${cur}") )
            return 0
            ;;
        -P)
            # Page size completion
            page_sizes="4k 4096 16k 16384 64k 65536"
            COMPREPLY=( $(compgen -W "${page_sizes}" -- "${cur}") )
            return 0
            ;;
        -m)
            # Memory size - just allow any number
            return 0
            ;;
        -s)
            # SMP count - suggest common values
            COMPREPLY=( $(compgen -W "1 2 4 8 16" -- "${cur}") )
            return 0
            ;;
        -G)
            # GIC version
            gic_versions="2 3 host"
            COMPREPLY=( $(compgen -W "${gic_versions}" -- "${cur}") )
            return 0
            ;;
        -A|-d|-f)
            # Append cmdline, disk image, or 9p shared dir - free text
            return 0
            ;;
    esac

    # If current word starts with a dash, complete available flags
    if [[ "$cur" == -* ]]; then
        local all_flags="-3 -6 -v -k -c -M -n -t -g -C -X -h -p -P -m -s -A -d -f -G"
        COMPREPLY=( $(compgen -W "${all_flags}" -- "${cur}") )
        return 0
    fi

    # No other completions
    return 0
}

complete -F _lk_completion_bash make
complete -F _lk_qemu_completion_bash do-qemuarm
complete -F _lk_qemu_completion_bash do-qemux86
complete -F _lk_qemu_completion_bash do-qemuriscv
complete -F _lk_qemu_completion_bash do-qemum68k
complete -F _lk_qemu_completion_bash do-qemum6
complete -F _lk_qemu_completion_bash do-qemum4
complete -F _lk_qemu_completion_bash do-qemumips

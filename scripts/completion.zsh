# Completion script for LK (Little Kernel) projects, zsh implementation.

# compdef is provided by zsh's completion system; initialize it if needed.
if ! typeset -f compdef >/dev/null 2>&1; then
    autoload -Uz compinit
    compinit -i >/dev/null 2>&1
fi

if typeset -f compdef >/dev/null 2>&1; then
    _lk_completion_zsh() {
        # Check if we're in an LK-like directory
        if [[ ! -d project || ! -f makefile ]] || ! grep -q 'engine.mk' makefile 2>/dev/null; then
            return 0
        fi

        local -a projects
        projects=( project/*.mk(N:t:r) )
        local -a common_targets
        common_targets=( help list clean spotless tags list-arch list-toolchain )
        local -a all_targets
        all_targets=( "${projects[@]}" "${common_targets[@]}" )

        # Handle PROJECT=... completion to mirror bash behavior.
        if [[ "${words[CURRENT]}" == PROJECT=* ]]; then
            local prefix="${words[CURRENT]#PROJECT=}"
            compadd -P 'PROJECT=' -- ${${projects:#${~prefix}*}}
            return 0
        fi

        compadd -- "${all_targets[@]}"
    }

    compdef _lk_completion_zsh make

    _lk_qemu_completion_zsh() {
        # Check if we're in an LK-like directory
        if [[ ! -d project || ! -f makefile ]] || ! grep -q 'engine.mk' makefile 2>/dev/null; then
            return 0
        fi

        local -a projects
        projects=( project/*.mk(N:t:r) )

        local -a page_sizes
        page_sizes=( 4k 4096 16k 16384 64k 65536 )

        local -a gic_versions
        gic_versions=( 2 3 host )

        local -a smp_counts
        smp_counts=( 1 2 4 8 16 )

        # Get the current word and previous word for context
        local cur="${words[CURRENT]}"
        local prev="${words[CURRENT-1]}"

        # If previous word is a flag that takes an argument, provide appropriate completions
        case "$prev" in
            -p)
                compadd -- "${projects[@]}"
                return 0
                ;;
            -P)
                compadd -- "${page_sizes[@]}"
                return 0
                ;;
            -s)
                compadd -- "${smp_counts[@]}"
                return 0
                ;;
            -G)
                compadd -- "${gic_versions[@]}"
                return 0
                ;;
            -m|-A|-d|-f)
                # Free text arguments, no specific completions
                return 0
                ;;
        esac

        # If current word starts with -, offer flag completions
        if [[ "$cur" == -* ]]; then
            local -a all_flags
            all_flags=( -3 -6 -v -k -c -M -n -t -g -C -X -h -p -P -m -s -A -d -f -G )
            compadd -- "${all_flags[@]}"
            return 0
        fi

        return 0
    }

    # Register completions for all do-qemu* scripts
    compdef _lk_qemu_completion_zsh do-qemuarm
    compdef _lk_qemu_completion_zsh do-qemux86
    compdef _lk_qemu_completion_zsh do-qemuriscv
    compdef _lk_qemu_completion_zsh do-qemum68k
    compdef _lk_qemu_completion_zsh do-qemum6
    compdef _lk_qemu_completion_zsh do-qemum4
    compdef _lk_qemu_completion_zsh do-qemumips
fi

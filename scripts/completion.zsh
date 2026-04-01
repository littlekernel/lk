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
fi

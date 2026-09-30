#!/usr/bin/env bash

# Shared helper sourced by the QEMU launcher scripts.
# This file is not intended to be run directly.

function quote_cmdline_arg {
    local arg=$1
    local lhs rhs
    if [[ "$arg" == *"="* ]]; then
        lhs=${arg%%=*}
        rhs=${arg#*=}
        case "$rhs" in
            *[[:space:]\"\\]*)
                rhs=${rhs//\\/\\\\}
                rhs=${rhs//\"/\\\"}
                printf '%s="%s"' "$lhs" "$rhs"
                ;;
            *)
                printf '%s=%s' "$lhs" "$rhs"
                ;;
        esac
        return
    fi

    case "$arg" in
        *[[:space:]\"\\]*)
            arg=${arg//\\/\\\\}
            arg=${arg//\"/\\\"}
            printf '"%s"' "$arg"
            ;;
        *)
            printf '%s' "$arg"
            ;;
    esac
}

# Build directory for a project, matching where make puts it: BUILDROOT and
# BUILDDIR_SUFFIX are read from the environment, as make itself does.
function lk_builddir {
    printf '%s/build-%s%s' "${BUILDROOT:-.}" "$1" "${BUILDDIR_SUFFIX:-}"
}

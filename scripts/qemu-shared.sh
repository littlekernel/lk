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
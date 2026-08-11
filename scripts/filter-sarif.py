#!/usr/bin/env python3
"""Drop SARIF results that live under a given path prefix, in place.

Used by .github/workflows/codeql.yml to remove alerts in vendored third party
code before uploading them to GitHub code scanning.

CodeQL has a paths-ignore option, but it is only honored for languages analyzed
without a build. The C/C++ analysis builds LK for real, so the filtering has to
happen on the SARIF afterwards.
"""

import argparse
import json
import sys


def result_uris(result):
    """Yield every artifact URI a single SARIF result points at."""
    for location in result.get("locations") or []:
        uri = (location.get("physicalLocation", {})
                       .get("artifactLocation", {})
                       .get("uri"))
        if uri:
            yield uri


def filter_file(path, prefixes):
    """Rewrite one SARIF file, returning the number of results dropped."""
    with open(path) as f:
        sarif = json.load(f)

    dropped = 0
    for run in sarif.get("runs") or []:
        results = run.get("results")
        if results is None:
            continue
        kept = [r for r in results
                if not any(u.startswith(prefixes) for u in result_uris(r))]
        dropped += len(results) - len(kept)
        run["results"] = kept

    if dropped:
        with open(path, "w") as f:
            json.dump(sarif, f)

    return dropped


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--prefix", action="append", required=True, metavar="PATH",
                        help="drop results whose path starts with this; repeatable")
    parser.add_argument("files", nargs="+", metavar="FILE.sarif")
    args = parser.parse_args()

    prefixes = tuple(args.prefix)
    for path in args.files:
        dropped = filter_file(path, prefixes)
        print(f"{path}: dropped {dropped} result(s) under {', '.join(prefixes)}")

    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3

# A utility for installing LK toolchains.

from __future__ import annotations

import argparse
import html.parser
import io
import os
import pathlib
import sys
import tarfile
import threading
import urllib.request
from typing import Self

BASE_URL = "https://newos.org/toolchains"

HOST_OS = os.uname().sysname
HOST_CPU = os.uname().machine

LK_ROOT = pathlib.Path(os.path.realpath(__file__)).parent.parent
DEFAULT_TOOLCHAIN_DIR = LK_ROOT.joinpath("toolchain")

TAR_EXT = ".tar.xz"


def main() -> int:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        description="Installs the matching LK toolchains from the official host, "
        + BASE_URL,
    )
    parser.add_argument(
        "--list",
        help="just list the matching toolchains; don't download them",
        action="store_true",
    )
    parser.add_argument(
        "--prefix",
        help="a toolchain prefix on which to match. If none are specified, all prefixes"
        " will match",
        nargs="*",
    )
    parser.add_argument(
        "--version",
        help='the exact toolchain version to match, or "latest" to specify only the '
        'latest version, or "all" for all versions',
        type=str,
        default="latest",
    )
    parser.add_argument(
        "--install-dir",
        help="the directory at which to install the toolchains",
        type=pathlib.Path,
        default=DEFAULT_TOOLCHAIN_DIR,
    )
    parser.add_argument(
        "--force",
        help="whether to overwrite past installed versions of matching toolchains",
        action="store_true",
    )
    parser.add_argument(
        "--host-os",
        help="the toolchains' host OS",
        type=str,
        default=HOST_OS,
    )
    parser.add_argument(
        "--host-cpu",
        help="the toolchains' host architecture",
        type=str,
        default=HOST_CPU,
    )
    args = parser.parse_args()

    # Get the full list of remote toolchains available for the provided host.
    response = urllib.request.urlopen(BASE_URL)
    if response.status != 200:
        print(f"Error accessing {BASE_URL}: {response.status}")
        return 1
    parser = RemoteToolchainHTMLParser(args.host_os, args.host_cpu)
    parser.feed(response.read().decode("utf-8"))
    toolchains = parser.toolchains

    # Filter them given --prefix and --version selections.
    toolchains.sort()
    if args.prefix:
        toolchains = [t for t in toolchains if t.prefix in args.prefix]
    if args.version == "latest":
        # Since we sorted lexicographically on (prefix, version tokens), to pick out the
        # latest versions we need only iterate through and pick out the last entry for a
        # given prefix.
        toolchains = [
            toolchains[i]
            for i in range(len(toolchains))
            if (
                i == len(toolchains) - 1
                or toolchains[i].prefix != toolchains[i + 1].prefix
            )
        ]
    elif args.version != "all":
        toolchains = [t for t in toolchains if t.version == args.version]

    if not toolchains:
        print("No matching toolchains")
        return 0

    if args.list:
        print("Matching toolchains:")
        for toolchain in toolchains:
            print(toolchain.name)
        return 0

    # The download routine for a given toolchain, factored out for
    # multithreading below.
    def download(toolchain: RemoteToolchain) -> None:
        response = urllib.request.urlopen(toolchain.url)
        if response.status != 200:
            print(f"Error while downloading {toolchain.name}: {response.status}")
            return
        with tarfile.open(fileobj=io.BytesIO(response.read()), mode="r:xz") as f:
            f.extractall(path=args.install_dir, filter="data")

    downloads = []
    for toolchain in toolchains:
        local = args.install_dir.joinpath(toolchain.name)
        if local.exists() and not args.force:
            print(
                f"{toolchain.name} already installed; "
                "skipping... (pass --force to overwrite)",
            )
            continue
        print(f"Downloading {toolchain.name} to {local}...")
        downloads.append(threading.Thread(target=download, args=(toolchain,)))
        downloads[-1].start()

    for thread in downloads:
        thread.join()

    return 0


class RemoteToolchain:
    def __init__(self, prefix: str, version: str, host_os: str, host_cpu: str) -> None:
        self._prefix = prefix
        self._version = [int(token) for token in version.split(".")]
        self._host = f"{host_os}-{host_cpu}"

    # Orders toolchains lexicographically on (prefix, version tokens).
    def __lt__(self, other: Self) -> bool:
        return self._prefix < other.prefix or (
            self._prefix == other.prefix and self._version < other._version
        )

    @property
    def prefix(self) -> str:
        return self._prefix

    @property
    def version(self) -> str:
        return ".".join(map(str, self._version))

    @property
    def name(self) -> str:
        return f"{self._prefix}-{self.version}-{self._host}"

    @property
    def url(self) -> str:
        return f"{BASE_URL}/{self.name}{TAR_EXT}"


# A simple HTML parser for extracting the toolchain names found at BASE_URL.
#
# It expects toolchains to be available as hyperlinks on that page. Once the
# HTML has been passed to feed(), the parsed toolchains will be accessible via
# toolchains().
class RemoteToolchainHTMLParser(html.parser.HTMLParser):
    def __init__(self, host_os: str, host_cpu: str) -> None:
        html.parser.HTMLParser.__init__(self)
        self._toolchains = []
        self._tags = []
        self._host_os = host_os
        self._host_cpu = host_cpu

    # The parsed toolchains.
    @property
    def toolchains(self) -> list[RemoteToolchain]:
        return self._toolchains

    #
    # The following methods implement the parsing, overriding those defined in
    # the base class.
    #

    def handle_starttag(self, tag: str, _: str) -> None:
        self._tags.append(tag)

    def handle_endtag(self, _: str) -> None:
        self._tags.pop()

    def handle_data(self, data: str) -> None:
        # Only process hyperlinks with tarball names.
        if not self._tags or self._tags[-1] != "a" or not data.endswith(TAR_EXT):
            return
        tokens = data.removesuffix(TAR_EXT).split("-")
        if len(tokens) != 5:
            print(f"Warning: malformed toolchain name: {data}")
            return
        prefix = tokens[0] + "-" + tokens[1]
        version = tokens[2]
        host_os = tokens[3]
        host_cpu = tokens[4]
        if host_os != self._host_os or host_cpu != self._host_cpu:
            return
        self._toolchains.append(RemoteToolchain(prefix, version, host_os, host_cpu))


if __name__ == "__main__":
    sys.exit(main())

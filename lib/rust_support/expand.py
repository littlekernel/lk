#! /usr/bin/env python3

import fileinput
import os

# This script reads a template from standard input or the named file, and
# outputs this with some template insertions, based on some values coming from
# the make invocation.

class Substr():
    def __init__(self):
        self.subs = []

    def add(self, name):
        value = os.environ.get(name)
        if value is None:
            raise Exception(f"Environment variable {name} not set")
        self.subs.append( (f"@{name}@", value) )

        if name == "BUILDROOT":
            self.buildroot = value

    # Generate a substitution rules for a list of crates.
    def add_depcrates(self, name):
        crates = os.environ.get(name)
        if crates is None:
            raise Exception(f"Environment variable {name} not set")
        value = ""
        for crate in crates.split():
            base = os.path.basename(crate)
            value += f"[dependencies.{base}]\npath = \"{self.buildroot}/{crate}\"\n"
            # TODO: Get the crate version from the crate's Cargo.toml file.
            value += f"version = \"0.1.0\"\n\n"
        self.subs.append( ("@DEPCRATES@", value) )

    def add_deplinks(self, name):
        crates = os.environ.get(name)
        if crates is None:
            raise Exception(f"Environment variable {name} not set")
        value = ""
        for crate in crates.split():
            base = os.path.basename(crate)
            base = base.replace("-", "_")
            if value != "":
                value += "    "
            value += f"{base}::must_link();"
        self.subs.append( (f"@DEPLINKS@", value) )

    def sub(self, line):
        for (k,v) in self.subs:
            line = line.replace(k, v)
        return line

def subst(name):
    value = os.environ.get(name)

subber = Substr();

subber.add("BUILDROOT")
subber.add_depcrates("RUST_CRATES")
subber.add_deplinks("RUST_CRATES")

for line in fileinput.input():
    line = subber.sub(line.rstrip())
    print(line)

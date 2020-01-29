let
  sources = import ./nix/sources.nix;
  pkgs = import (builtins.fetchTarball https://github.com/input-output-hk/nixpkgs/archive/0ee0489d42e.tar.gz) {};
  lib = pkgs.lib;
  overlay = self: super: {
    littlekernel = self.stdenv.mkDerivation {
      name = "littlekernel";
      src = lib.cleanSource ./.;
      nativeBuildInputs = [ x86_64.uart-manager ];
    };
    uart-manager = self.stdenv.mkDerivation {
      name = "uart-manager";
      src = sources.rpi-open-firmware + "/uart-manager";
    };
  };
  vc4 = pkgs.pkgsCross.vc4.extend overlay;
  x86_64 = pkgs.extend overlay;
  arm7 = pkgs.pkgsCross.armv7l-hf-multiplatform.extend overlay;
in {
  arm7 = {
    inherit (arm7) littlekernel;
  };
  vc4 = {
    inherit (vc4) littlekernel;
  };
  x86_64 = {
    inherit (x86_64) uart-manager;
  };
}

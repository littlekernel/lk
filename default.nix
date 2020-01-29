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
in lib.fix (self: {
  arm7 = {
    inherit (arm7) littlekernel;
  };
  vc4 = {
    shell = vc4.littlekernel;
    rpi3.bootcode = vc4.callPackage ./lk.nix { project = "rpi3-bootcode"; };
    rpi4.start4 = vc4.callPackage ./lk.nix { project = "rpi4-start4"; };
  };
  x86_64 = {
    inherit (x86_64) uart-manager;
  };
  testcycle = pkgs.writeShellScript "testcycle" ''
    set -e
    scp ${self.vc4.rpi3.bootcode}/lk.bin root@router.localnet:/tftproot/open-firmware/bootcode.bin
    exec ${x86_64.uart-manager}/bin/uart-manager
  '';
})

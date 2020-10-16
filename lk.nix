{ stdenv, project, which, imagemagick, python }:

stdenv.mkDerivation {
  name = "littlekernel-${project}";
  src = builtins.path {
    filter = stdenv.lib.cleanSourceFilter;
    path = ./.;
    name = "lk-src";
  };
  makeFlags = [ "PROJECT=${project}" ];
  hardeningDisable = [ "format" ];
  nativeBuildInputs = [
    which
    imagemagick.__spliced.buildBuild # work around a bug in nixpkgs
    python
  ];
  installPhase = ''
    mkdir -p $out/nix-support
    cp -r build-${project}/{config.h,lk.*} $out
    cat <<EOF > $out/nix-support/hydra-metrics
    lk.bin $(stat --printf=%s $out/lk.bin) bytes
    lk.elf $(stat --printf=%s $out/lk.elf) bytes
    EOF
    echo "file binary-dist $out/lk.bin" >> $out/nix-support/hydra-build-products
    echo "file binary-dist $out/lk.elf" >> $out/nix-support/hydra-build-products
  '';
  ARCH_arm64_TOOLCHAIN_PREFIX = "aarch64-none-elf-";
}

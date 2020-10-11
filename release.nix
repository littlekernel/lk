let
  self = import ./.;
in {
  arm = {
    inherit (self.arm) rpi1-test rpi2-test rpi3-test;
  };
  vc4 = {
    inherit (self.vc4) rpi3 rpi4 vc4;
  };
}

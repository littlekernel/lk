let
  self = import ./.;
in {
  arm = {
    inherit (self.arm) rpi2-test;
  };
  vc4 = {
    inherit (self.vc4) rpi3 rpi4 vc4;
  };
}

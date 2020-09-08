let
  self = import ./.;
in {
  vc4 = {
    inherit (self.vc4) rpi3 rpi4 vc4;
  };
}

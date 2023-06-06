let
  name = "BenchMIAOes";
  # pin nixpkgs to current stable version (22.11)
  pkgsSrc = builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/50c23cd4ff6c8344e0b4d438b027b3afabfe58dd.tar.gz";
    sha256 = "07rs6d06sg84b5nf9zwjczzbam1f3fax3s2i9kp65fdbbskqp5zs";
  };
  pkgs = import pkgsSrc {};
in pkgs.mkShell {
  inherit name;
  packages = with pkgs; [
    stdenv.cc
    glibc.static
  ];
}

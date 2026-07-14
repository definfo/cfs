{
  description = "C/C++ development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    flake-compat = {
      url = "github:NixOS/flake-compat";
      flake = false;
    };
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{
      self,
      nixpkgs,
      flake-compat,
      flake-parts,
      treefmt-nix,
    }:
    # See https://flake.parts/module-arguments for module arguments
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        inputs.treefmt-nix.flakeModule
      ];

      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
      ];

      perSystem =
        {
          config,
          pkgs,
          ...
        }:
        {
          # https://github.com/numtide/treefmt-nix
          treefmt = {
            projectRootFile = "flake.nix";
            programs = {
              oxfmt.enable = true;
            };
          };

          devShells.default = pkgs.mkShell {
            inputsFrom = [ config.treefmt.build.devShell ];
            packages = [ pkgs.cmake ];
          };
        };
    };
}

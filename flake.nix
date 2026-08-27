{
  description = "Gores Client DDNet";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "gores-client";
        version = "1.5";

        src = pkgs.fetchurl {
          url = "https://github.com/RoflikBEST/bestdownload/releases/download/v1.5/bestclient.tar.xz";
          hash = "sha256-fDjtdj6mC76IXJLul0wfj7+7GETU/vveJ8G6S4LskLk";
        };

        nativeBuildInputs = [ 
          pkgs.autoPatchelfHook
          pkgs.makeWrapper
        ];

        buildInputs = [
          pkgs.stdenv.cc.cc.lib
          pkgs.SDL2
          pkgs.freetype
          pkgs.libGL
          pkgs.curl
          pkgs.openssl
	        pkgs.vulkan-loader
	        pkgs.libnotify
        ];

        installPhase = ''
        mkdir -p $out/bin $out/share/applications
        cp -r . $out/
        chmod +x $out/DDNet

        cat > $out/share/applications/gores-client.desktop <<EOF
[Desktop Entry]
Name=Gores Client
Comment=DDNet client with extra features
Exec=$out/bin/GoresClient
Icon=$out/data/BestClient/bc_icon.png
Type=Application
Categories=Game;
EOF

        # wrap the binary so it runs from the right directory
        makeWrapper $out/DDNet $out/bin/GoresClient \
          --run "cd $out"
        '';
      };

      apps.${system}.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/GoresClient";
      };
    };
}

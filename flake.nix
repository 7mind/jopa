{
  description = "Jikes Java compiler with Java 5 support";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Build tools
            gcc
            gnumake
            autoconf
            automake
            libtool
            pkg-config

            # Development tools
            gdb
            valgrind

            # Java for testing
            openjdk8

            # Utilities
            git
            which
            file
          ];

          shellHook = ''
            echo "================================================"
            echo "Jikes Java 5 Development Environment"
            echo "================================================"
            echo "C++ Compiler: $(gcc --version | head -1)"
            echo "Java: $(java -version 2>&1 | head -1)"
            echo ""
            echo "Available commands:"
            echo "  - Build Jikes: cd jikes && ./configure && make"
            echo "  - Run tests: make check"
            echo "  - Test Java 5: cd test-generics && ./run-tests.sh"
            echo ""
          '';
        };

        # Package definition for building Jikes
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "jikes";
          version = "1.22-java5";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            autoconf
            automake
            libtool
          ];

          buildInputs = with pkgs; [
            gcc
          ];

          preConfigure = ''
            # Run autoreconf if configure doesn't exist
            if [ ! -f configure ]; then
              autoreconf -fi
            fi
          '';

          configureFlags = [
            "--enable-source-15"
          ];

          buildPhase = ''
            cd src
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            install -m 755 src/jikes $out/bin/jikes
          '';

          meta = with pkgs.lib; {
            description = "Jikes Java compiler with Java 5 (generics, foreach, varargs, enums) support";
            homepage = "https://github.com/jikes/jikes";
            license = licenses.ipl10;
            platforms = platforms.unix;
            maintainers = [];
          };
        };
      }
    );
}

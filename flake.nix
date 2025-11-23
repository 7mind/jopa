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
            cmake
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
            echo "CMake: $(${pkgs.cmake}/bin/cmake --version | head -1)"
            echo "Java: $(java -version 2>&1 | head -1)"
            echo ""
            echo "Available commands:"
            echo "  - Build with CMake: cmake -S . -B build && cmake --build build"
            echo "  - Run tests: cd build && ctest"
            echo "  - Legacy build: ./configure && make"
            echo ""
          '';
        };

        # Package definition for building Jikes with CMake
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "jikes";
          version = "1.22-java5";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
          ];

          buildInputs = with pkgs; [
            gcc
          ];

          cmakeFlags = [
            "-DJIKES_ENABLE_SOURCE_15=ON"
            "-DCMAKE_BUILD_TYPE=Release"
          ];

          # CMake will handle the build automatically

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

{
  description = "Jopa Java compiler (based on Jikes) with Java 7 support";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        jdk = pkgs.openjdk8;
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Build tools
            clang
            llvm
            cmake
            gnumake
            pkg-config

            # Libraries
            libzip
            cpptrace

            # Development tools
            lldb
            valgrind

            # Java for testing
            openjdk8
            zip
            unzip

            # Utilities
            git
            which
            file

            # JamVM + GNU Classpath build dependencies
            autoconf
            automake
            libtool
            libffi
            zlib
            antlr
          ];

          shellHook = ''
            echo "================================================"
            echo "Jopa Development Environment (Clang)"
            echo "================================================"
            export JAVA_HOME=${jdk.home}
            echo "C++ Compiler: $(clang++ --version | head -1)"
            echo "CMake: $(${pkgs.cmake}/bin/cmake --version | head -1)"
            echo "Java: $(java -version 2>&1 | head -1)"
            echo ""
            echo "Available commands:"
            echo "  - Build with CMake: cmake -S . -B build && cmake --build build"
            echo "  - Run tests: cd build && ctest"
            echo ""
          '';
        };

        # Package definition for building Jopa with CMake
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "jopa";
          version = "1.22-java7";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
          ];

          buildInputs = with pkgs; [
            clang
            libzip
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
          ];

          # CMake will handle the build automatically

          meta = with pkgs.lib; {
            description = "Jopa Java compiler (based on Jikes) with Java 7 support";
            homepage = "https://github.com/pshirshov/jopa";
            license = licenses.ipl10;
            platforms = platforms.unix;
            maintainers = [];
          };
        };
      }
    );
}

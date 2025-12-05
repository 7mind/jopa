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

        # Common build dependencies for JOPA
        jopaBuildInputs = with pkgs; [
          clang
          llvm
          cmake
          gnumake
          pkg-config
          libzip
          cpptrace
        ];

        # Build dependencies for GNU Classpath and JamVM
        bootstrapBuildInputs = with pkgs; [
          autoconf
          automake
          libtool
          libffi
          zlib
          zip
          unzip
          coreutils
          gnused
          gnugrep
          findutils
          gawk
          patch
          cpio
          which
          file
        ];

        # Helper function to build JOPA compiler
        mkJopa = { debug ? false }: pkgs.clangStdenv.mkDerivation {
          pname = "jopa${if debug then "-debug" else ""}";
          version = "2.0.1";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
            openjdk8  # Required for running tests
            zip       # Required for building stub runtime
            python3   # Required for post-processing parser headers
          ];

          buildInputs = with pkgs; [
            libzip
            cpptrace
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"}"
          ] ++ (if debug then [
            "-DJOPA_ENABLE_SANITIZERS=ON"
            "-DJOPA_ENABLE_LEAK_SANITIZER=OFF"
          ] else []);

          installPhase = ''
            mkdir -p $out/bin $out/lib
            cp src/jopa $out/bin/
            cp runtime/jopa-stub-rt.jar $out/lib/
          '';

          doCheck = true;
          checkPhase = ''
            ctest --output-on-failure -j$NIX_BUILD_CORES
          '';

          meta = with pkgs.lib; {
            description = "Jopa Java compiler (based on Jikes) with Java 7 support${if debug then " (debug build with sanitizers)" else ""}";
            homepage = "https://github.com/pshirshov/jopa";
            license = licenses.ipl10;
            platforms = platforms.unix;
          };
        };

        jopa = mkJopa { debug = false; };
        jopa-debug = mkJopa { debug = true; };

        # Helper function to build devjopak variants
        mkDevjopak = { name, ecjVersion ? null, debug ? false }: pkgs.clangStdenv.mkDerivation {
          pname = name;
          version = "2.0.1";

          src = ./.;

          nativeBuildInputs = jopaBuildInputs ++ bootstrapBuildInputs ++ [
            pkgs.openjdk8  # Required for JOPA tests
            pkgs.python3   # Required for parser header post-processing
            pkgs.gcc       # Required for JamVM (clang 17+ rejects its computed gotos)
          ];

          buildInputs = with pkgs; [
            libffi
            zlib
          ];

          # Disable automatic cmake configure - we do it manually in buildPhase
          dontConfigure = true;

          postPatch = ''
            patchShebangs vendor/*.sh devjopak/*.sh
          '';

          buildPhase = ''
            export HOME=$TMPDIR

            # Build JOPA compiler and stub runtime first
            cmake -S . -B build-jopa \
              -DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"} \
              ${if debug then "-DJOPA_ENABLE_SANITIZERS=ON -DJOPA_ENABLE_LEAK_SANITIZER=OFF" else ""}
            cmake --build build-jopa --target jopa jopa-stub-rt -j$NIX_BUILD_CORES

            # Build devjopak
            cmake -S devjopak -B build-devjopak \
              -DJOPA_BUILD_DIR=$PWD/build-jopa \
              -DJOPA_CLASSPATH_VERSION=0.99 \
              ${if ecjVersion != null then "-DECJ_VERSION=${ecjVersion}" else ""} \
              -DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"}

            # Build the target
            cmake --build build-devjopak --target ${if ecjVersion != null then "devjopak-ecj" else "devjopak"} -j1

            # Find the archive
            if [ -n "${if ecjVersion != null then ecjVersion else ""}" ]; then
              ARCHIVE=$(find build-devjopak -name "devjopak-*-ecj-*.tar.gz" | head -n1)
            else
              ARCHIVE=$(find build-devjopak -name "devjopak-*-jopa-*.tar.gz" | head -n1)
            fi

            if [ -z "$ARCHIVE" ]; then
              echo "Error: Could not find devjopak archive"
              exit 1
            fi

            mkdir -p $TMPDIR/dist
            tar -xzf "$ARCHIVE" -C $TMPDIR/dist
          '';

          installPhase = ''
            # Find the extracted directory
            DEVJOPAK_DIR=$(find $TMPDIR/dist -maxdepth 1 -type d -name "devjopak*" | head -n1)
            if [ -z "$DEVJOPAK_DIR" ]; then
              echo "Error: Could not find extracted devjopak directory"
              exit 1
            fi

            mkdir -p $out
            cp -r "$DEVJOPAK_DIR"/* $out/
            chmod +x $out/bin/*
          '';

          meta = with pkgs.lib; {
            description = "DevJopaK - Self-contained Java Development Kit${if debug then " (debug)" else ""}";
            homepage = "https://github.com/pshirshov/jopa";
            license = licenses.ipl10;
            platforms = platforms.unix;
          };
        };

        # DevJopaK variants (Release)
        devjopak-gnucp099-jopa201 = mkDevjopak {
          name = "devjopak-gnucp099-jopa201";
        };

        devjopak-gnucp099-ecj421 = mkDevjopak {
          name = "devjopak-gnucp099-ecj421";
          ecjVersion = "4.2.1";
        };

        devjopak-gnucp099-ecj422 = mkDevjopak {
          name = "devjopak-gnucp099-ecj422";
          ecjVersion = "4.2.2";
        };

        # DevJopaK variants (Debug)
        devjopak-gnucp099-jopa201-debug = mkDevjopak {
          name = "devjopak-gnucp099-jopa201-debug";
          debug = true;
        };

        devjopak-gnucp099-ecj421-debug = mkDevjopak {
          name = "devjopak-gnucp099-ecj421-debug";
          ecjVersion = "4.2.1";
          debug = true;
        };

        devjopak-gnucp099-ecj422-debug = mkDevjopak {
          name = "devjopak-gnucp099-ecj422-debug";
          ecjVersion = "4.2.2";
          debug = true;
        };

        # ECJ standalone builds
        mkEcj = { ecjVersion, debug ? false }: pkgs.clangStdenv.mkDerivation {
          pname = "ecj${if debug then "-debug" else ""}";
          version = ecjVersion;

          src = ./.;

          nativeBuildInputs = jopaBuildInputs ++ bootstrapBuildInputs ++ [
            pkgs.openjdk8  # Required for JOPA tests
            pkgs.python3   # Required for parser header post-processing
            pkgs.gcc       # Required for JamVM (clang 17+ rejects its computed gotos)
          ];

          buildInputs = with pkgs; [
            libffi
            zlib
          ];

          dontConfigure = true;

          postPatch = ''
            patchShebangs vendor/*.sh devjopak/*.sh
          '';

          buildPhase = ''
            export HOME=$TMPDIR

            # Build JOPA compiler and stub runtime first
            cmake -S . -B build-jopa \
              -DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"} \
              ${if debug then "-DJOPA_ENABLE_SANITIZERS=ON -DJOPA_ENABLE_LEAK_SANITIZER=OFF" else ""}
            cmake --build build-jopa --target jopa jopa-stub-rt -j$NIX_BUILD_CORES

            # Build devjopak with ECJ
            cmake -S devjopak -B build-devjopak \
              -DJOPA_BUILD_DIR=$PWD/build-jopa \
              -DJOPA_CLASSPATH_VERSION=0.99 \
              -DECJ_VERSION=${ecjVersion} \
              -DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"}

            # Build ECJ target
            cmake --build build-devjopak --target ecj -j1
          '';

          installPhase = ''
            mkdir -p $out/lib
            cp build-devjopak/vendor-install/ecj/lib/ecj.jar $out/lib/
          '';

          meta = with pkgs.lib; {
            description = "Eclipse Compiler for Java ${ecjVersion} built with JOPA${if debug then " (debug)" else ""}";
            homepage = "https://www.eclipse.org/jdt/core/";
            license = licenses.epl10;
            platforms = platforms.unix;
          };
        };

        ecj_421 = mkEcj { ecjVersion = "4.2.1"; };
        ecj_422 = mkEcj { ecjVersion = "4.2.2"; };
        ecj_421-debug = mkEcj { ecjVersion = "4.2.1"; debug = true; };
        ecj_422-debug = mkEcj { ecjVersion = "4.2.2"; debug = true; };

        # Apache Ant built with JOPA
        mkAnt = { debug ? false }: pkgs.clangStdenv.mkDerivation {
          pname = "ant-jopa${if debug then "-debug" else ""}";
          version = "1.8.4";

          src = ./.;

          nativeBuildInputs = jopaBuildInputs ++ bootstrapBuildInputs ++ [
            pkgs.openjdk8  # Required for JOPA tests
            pkgs.python3   # Required for parser header post-processing
            pkgs.gcc       # Required for JamVM (clang 17+ rejects its computed gotos)
          ];

          buildInputs = with pkgs; [
            libffi
            zlib
          ];

          dontConfigure = true;

          postPatch = ''
            patchShebangs vendor/*.sh devjopak/*.sh
          '';

          buildPhase = ''
            export HOME=$TMPDIR

            # Build JOPA compiler and stub runtime first
            cmake -S . -B build-jopa \
              -DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"} \
              ${if debug then "-DJOPA_ENABLE_SANITIZERS=ON -DJOPA_ENABLE_LEAK_SANITIZER=OFF" else ""}
            cmake --build build-jopa --target jopa jopa-stub-rt -j$NIX_BUILD_CORES

            # Build devjopak (which includes Ant)
            cmake -S devjopak -B build-devjopak \
              -DJOPA_BUILD_DIR=$PWD/build-jopa \
              -DJOPA_CLASSPATH_VERSION=0.99 \
              -DCMAKE_BUILD_TYPE=${if debug then "Debug" else "Release"}

            # Build apache_ant target
            cmake --build build-devjopak --target apache_ant -j1
          '';

          installPhase = ''
            mkdir -p $out/bin $out/lib
            cp -r build-devjopak/vendor-install/ant/* $out/
          '';

          meta = with pkgs.lib; {
            description = "Apache Ant 1.8.4 built with JOPA${if debug then " (debug)" else ""}";
            homepage = "https://ant.apache.org/";
            license = licenses.asl20;
            platforms = platforms.unix;
          };
        };

        ant = mkAnt { debug = false; };
        ant-debug = mkAnt { debug = true; };

      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = jopaBuildInputs ++ bootstrapBuildInputs ++ [
            pkgs.lldb
            pkgs.valgrind
            jdk
            pkgs.git
            pkgs.bc
            pkgs.python3
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
            echo "Available packages (nix build .#<name>):"
            echo ""
            echo "  Release builds:"
            echo "    - default (jopa)            : JOPA compiler"
            echo "    - ecj_421, ecj_422          : ECJ built with JOPA"
            echo "    - ant                       : Apache Ant 1.8.4"
            echo "    - devjopak-gnucp099-jopa201 : DevJopaK with JOPA"
            echo "    - devjopak-gnucp099-ecj421  : DevJopaK with ECJ 4.2.1"
            echo "    - devjopak-gnucp099-ecj422  : DevJopaK with ECJ 4.2.2"
            echo ""
            echo "  Debug builds (with sanitizers):"
            echo "    - jopa-debug                      : JOPA compiler"
            echo "    - ecj_421-debug, ecj_422-debug    : ECJ"
            echo "    - ant-debug                       : Apache Ant"
            echo "    - devjopak-gnucp099-jopa201-debug : DevJopaK with JOPA"
            echo "    - devjopak-gnucp099-ecj421-debug  : DevJopaK with ECJ 4.2.1"
            echo "    - devjopak-gnucp099-ecj422-debug  : DevJopaK with ECJ 4.2.2"
            echo ""
          '';
        };

        packages = {
          default = jopa;
          inherit jopa jopa-debug;
          inherit ecj_421 ecj_422 ecj_421-debug ecj_422-debug;
          inherit ant ant-debug;
          inherit devjopak-gnucp099-jopa201 devjopak-gnucp099-ecj421 devjopak-gnucp099-ecj422;
          inherit devjopak-gnucp099-jopa201-debug devjopak-gnucp099-ecj421-debug devjopak-gnucp099-ecj422-debug;
        };

        # Checks run by `nix flake check`
        checks = {
          # Run primary test suite
          jopa-tests = jopa;
          jopa-debug-tests = jopa-debug;
        };
      }
    );
}

#!/bin/sh
# Arranca el loader de Skater Nation en Linux ARM32 softfp (Docker, armel
# emulado) para probar sin consola. Ver "../../Prueba-ARM32-sin-consola.md".
#
#   hosttest/run.sh build             compila (contenedor arm64 + gcc cruzado armel)
#   hosttest/run.sh run [segundos]    arranca el juego (por defecto 300 s)
#
# Ventana en vivo: HOSTTEST_WINDOW=1 (XQuartz con "Permitir conexiones de clientes
# de red" y `xhost +localhost`). Teclado/ratón en window_sdl.c.
# Logs del loader y capturas (frames/*.ppm): build/hosttest-data/.
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
VITAINC="${VITASDK:-$HOME/vitasdk}/arm-vita-eabi/include"
DATA="$ROOT/build/hosttest-data"
IMAGE=skaternation-hosttest-cross
RUNIMAGE=skaternation-hosttest-run
VOLUME=skaternation-hosttest-build

case "${1:-}" in
build)
  docker build -q -t "$IMAGE" - <<'EOF' >/dev/null
FROM debian:trixie-slim
RUN dpkg --add-architecture armel && apt-get update && apt-get install -y --no-install-recommends \
    gcc-arm-linux-gnueabi g++-arm-linux-gnueabi libc6-dev-armel-cross cmake make \
    libsdl2-dev:armel libegl-dev:armel libgles-dev:armel libopengl-dev:armel libopenal-dev:armel \
    libfreetype-dev:armel libpng-dev:armel zlib1g-dev:armel libbz2-dev:armel \
    && rm -rf /var/lib/apt/lists/*
EOF
  docker build -q --platform linux/arm/v5 -t "$RUNIMAGE" - <<'EOF' >/dev/null
FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    libsdl2-2.0-0 libegl1 libegl-mesa0 libopengl0 libgl1-mesa-dri libopenal1 \
    libfreetype6 libpng16-16t64 libbz2-1.0 && rm -rf /var/lib/apt/lists/*
EOF
  docker run --rm -v "$ROOT":/port:ro -v "$VITAINC":/vitainc:ro -v "$VOLUME":/build "$IMAGE" sh -ec '
    mkdir -p /build/psp2inc
    cp -r /vitainc/psp2 /vitainc/psp2common /vitainc/vitasdk /vitainc/kubridge.h /build/psp2inc/
    cmake -S /port/hosttest -B /build/out -DPSP2_INCLUDE=/build/psp2inc \
      -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=arm \
      -DCMAKE_C_COMPILER=arm-linux-gnueabi-gcc -DCMAKE_ASM_COMPILER=arm-linux-gnueabi-gcc \
      -DCMAKE_LIBRARY_ARCHITECTURE=arm-linux-gnueabi >/build/configure.log
    make -k -C /build/out -j"$(nproc)" 2>&1 | grep -E "error|undefined|Linking|Built" || true
    test -x /build/out/skaternation'
  ;;
run)
  mkdir -p "$DATA"
  WINDOW=
  if [ -n "${HOSTTEST_WINDOW:-}" ]; then WINDOW="-e DISPLAY=host.docker.internal:0"; fi
  # Datos del juego de solo lectura; logs/saves/frames en build/hosttest-data.
  docker run --rm --platform linux/arm/v5 $WINDOW -e HOSTTEST_FRAME_S \
    -e ALSOFT_DRIVERS=null -e EGL_PLATFORM=surfaceless -e LIBGL_ALWAYS_SOFTWARE=1 \
    -v "$VOLUME":/build:ro -v "$ROOT/ux0_data/skaternation":/game:ro -v "$DATA":/data \
    "$RUNIMAGE" sh -c '
      mkdir -p /data/skaternation /ur0:/data && touch /ur0:/data/libshacccg.suprx
      for f in /game/*; do case "${f##*/}" in logs|saves) ;; *) ln -sfn "$f" /data/skaternation/ ;; esac; done
      ln -sfn /data /ux0:data
      cd / && timeout '"${2:-300}"' /build/out/skaternation; echo "salida: $?"'
  echo "logs: $DATA/skaternation/logs  capturas: $DATA/frames"
  ;;
*)
  sed -n 2,11p "$0"; exit 2 ;;
esac

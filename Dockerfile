# syntax=docker/dockerfile:1
#
# Betrock++ Docker build
FROM ubuntu:24.04 AS builder

ARG BUILD_TARGET=server
ARG BUILD_TYPE=Release
ARG COMPILER=clang
ENV DEBIAN_FRONTEND=noninteractive

# Base toolchain + every dependency listed in the project's README for
# Debian/Ubuntu, covering both the server and client build.
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    ca-certificates \
    cmake \
    ninja-build \
    build-essential \
    clang \
    pkg-config \
    python3 \
    python3-pip \
    libdeflate-dev \
    libglfw3-dev \
    libglm-dev \
    libopenal-dev \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

# Used only to generate the GLAD OpenGL loader for client builds, offline
# and reproducibly (no network access needed, no git submodule required).
RUN pip install --break-system-packages --no-cache-dir glad

WORKDIR /src
COPY . .

# The upstream repo ships GLAD as a git submodule (src+headers) at
# external/glad. This zip export doesn't include submodule content, so we
# (re)generate the exact same classic-GLAD OpenGL 3.3 core loader locally
# whenever a client build is requested. This is a no-op for server builds.
RUN if [ "$BUILD_TARGET" = "client" ]; then \
    mkdir -p external/glad && \
    python3 -m glad \
    --profile core \
    --api "gl=3.3" \
    --generator c \
    --spec gl \
    --reproducible \
    --quiet \
    --out-path external/glad ; \
    fi

RUN if [ "$COMPILER" = "gcc" ]; then \
    export CC=gcc CXX=g++ ; \
    else \
    export CC=clang CXX=clang++ ; \
    fi ; \
    BUILD_SERVER=ON ; \
    [ "$BUILD_TARGET" = "client" ] && BUILD_SERVER=OFF ; \
    cmake -S . -B build -G Ninja \
    -DBUILD_SERVER=${BUILD_SERVER} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    && cmake --build build -j"$(nproc)"

# ---------------------------------------------------------------------------
# Stage 2: runtime (small image, only the pieces needed to run the server)
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

ARG BUILD_TARGET=server
ENV DEBIAN_FRONTEND=noninteractive

# Runtime-only dependency: libdeflate is dynamically linked by both targets.
# (The client also needs libglfw3/libopenal/libgl at runtime; installed
# below only when a client image is built.)
RUN apt-get update && apt-get install -y --no-install-recommends \
    libdeflate0 \
    && if [ "$BUILD_TARGET" = "client" ]; then \
    apt-get install -y --no-install-recommends \
    libglfw3 libopenal1 libgl1 libglx0 ; \
    fi \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /data
COPY --from=builder /src/build/BetrockPlusPlus /usr/local/bin/BetrockPlusPlus

# Minecraft Beta 1.7.3 default port
EXPOSE 25565/tcp

ENTRYPOINT ["/usr/local/bin/BetrockPlusPlus"]
CMD []

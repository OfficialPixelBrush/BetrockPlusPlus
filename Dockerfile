# syntax=docker/dockerfile:1
#
# Betrock++ Docker build
FROM ubuntu:24.04 AS builder

ARG BUILD_TARGET=server
ARG BUILD_TYPE=Release
ARG COMPILER=clang
ARG LIBDEFLATE_VERSION=1.23
ENV DEBIAN_FRONTEND=noninteractive

# Mitigate intermittent "Hash Sum mismatch" errors common with Docker Desktop
# / flaky mirrors (disable pipelining, retry, avoid broken proxies).
RUN printf '%s\n' \
    'Acquire::Retries "5";' \
    'Acquire::http::Pipeline-Depth "0";' \
    'Acquire::http::No-Cache "true";' \
    'Acquire::BrokenProxy "true";' \
    > /etc/apt/apt.conf.d/99docker-apt-fix

# Base toolchain. Server needs libdeflate (built from source below for CMake
# CONFIG packages). Client also needs glm + OpenGL headers; SDL3 is fetched
# by CMake (SDL_VENDORED=ON) at configure time.
RUN apt-get update && apt-get install -y --no-install-recommends \
    git \
    ca-certificates \
    cmake \
    ninja-build \
    build-essential \
    clang \
    pkg-config \
    libcurl4-openssl-dev \
    && if [ "$BUILD_TARGET" = "client" ]; then \
    apt-get install -y --no-install-recommends \
    libglm-dev \
    libgl1-mesa-dev ; \
    fi \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu's libdeflate-dev does not ship libdeflate-config.cmake, but
# CMakeLists.txt requires `find_package(libdeflate CONFIG)`. Build+install
# from source (static only) so the CONFIG package exists and the runtime
# image does not need a separate libdeflate package.
RUN git clone --depth 1 --branch "v${LIBDEFLATE_VERSION}" \
    https://github.com/ebiggers/libdeflate.git /tmp/libdeflate \
    && cmake -S /tmp/libdeflate -B /tmp/libdeflate/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DLIBDEFLATE_BUILD_STATIC_LIB=ON \
    -DLIBDEFLATE_BUILD_SHARED_LIB=OFF \
    -DLIBDEFLATE_BUILD_GZIP=OFF \
    && cmake --build /tmp/libdeflate/build -j"$(nproc)" \
    && cmake --install /tmp/libdeflate/build \
    && rm -rf /tmp/libdeflate

WORKDIR /src
COPY . .

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
    -DCMAKE_PREFIX_PATH=/usr/local \
    && cmake --build build -j"$(nproc)"

# ---------------------------------------------------------------------------
# Stage 2: runtime (small image, only the pieces needed to run the server)
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS runtime

ARG BUILD_TARGET=server
ENV DEBIAN_FRONTEND=noninteractive

RUN printf '%s\n' \
    'Acquire::Retries "5";' \
    'Acquire::http::Pipeline-Depth "0";' \
    'Acquire::http::No-Cache "true";' \
    'Acquire::BrokenProxy "true";' \
    > /etc/apt/apt.conf.d/99docker-apt-fix

# libdeflate is statically linked from the builder stage. Client runtime
# still needs OpenGL/SDL display libs if you actually run the GUI in-container
# (unusual; see DOCKER.md).
RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 \
    && if [ "$BUILD_TARGET" = "client" ]; then \
    apt-get install -y --no-install-recommends \
    libgl1 libglx0 \
    ; fi \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /data
COPY --from=builder /src/build/BetrockPlusPlus /usr/local/bin/BetrockPlusPlus

# Minecraft Beta 1.7.3 default port
EXPOSE 25565/tcp

ENTRYPOINT ["/usr/local/bin/BetrockPlusPlus"]
CMD []

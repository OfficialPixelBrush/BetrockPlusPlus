> [!NOTE]
> This whole file, alongside the docker-compose and Dockerfile were AI generated.
> These and instructions and these files have only been generated because none of
> us have sufficient experience with Docker yet. If anyone wants to rewrite these
> files in a more approachable or cleaner way,
> we'd be more than happy to accept those changes instead.

# Building Betrock++ with Docker

This lets you build (and run) Betrock++ the same way on **Windows** and
**Linux** hosts, without installing CMake, a C++23 compiler, or any of the
libraries from the README yourself. The compiler and libraries all live
inside the container - only Docker needs to be on your machine.

## Prerequisites

- **Windows 10/11**: install [Docker Desktop](https://www.docker.com/products/docker-desktop/)
  (uses Linux containers by default - that's what you want, don't switch to
  Windows containers).
- **Linux**: install `docker` (e.g. `sudo apt install docker.io` or your
  distro's equivalent), or Docker Desktop for Linux.

Both cases build the exact same image, so the resulting binary behaves
identically regardless of host OS.

## Building the server (default)

```bash
docker build -t betrockpp:server .
```

Or with Compose (also sets up a persistent world/config folder, see below):

```bash
docker compose build
```

## Building the client

> [!NOTE]
> The client is currently unfinished and not really usable. We apologize for the inconvenience!!

The client needs OpenGL/GLFW/GLM/OpenAL, which the image installs
automatically:

```bash
docker build --build-arg BUILD_TARGET=client -t betrockpp:client .
```

> **Note on GLAD**: the upstream repository includes the `external/glad`
> OpenGL loader as a git submodule, which isn't present in a plain source
> export/zip (submodule content is never included in a zip download). The
> Dockerfile regenerates the identical OpenGL 3.3 core loader at build time
> using the offline, reproducible `glad` Python generator - no internet
> access or `git submodule` step required.
>
> Running a GUI client *inside* a container still needs a display (X11/Wayland
> forwarding or a VNC-in-container setup), which this Dockerfile doesn't set
> up, since it's rarely worth the complexity. Use this target to confirm the
> client **compiles**, then copy the binary out (see below) and run it
> natively if you want to actually play.

## Running the server

With plain `docker run`:

```bash
mkdir -p data
docker run -it --rm \
  -p 25565:25565 \
  -v "$PWD/data:/data" \
  betrockpp:server --port 25565
```

On Windows PowerShell, replace `$PWD` with `${PWD}` or an absolute path,
e.g. `-v "${PWD}\data:/data"`.

With Compose:

```bash
docker compose up
```

The world save and config file are written to the container's working
directory (`/data`), which is bind-mounted to `./data` on your host, so
your world persists across container restarts/rebuilds.

Any of the server's normal CLI flags (see `--help`) can be appended, e.g.:

```bash
docker run -it --rm -p 25565:25565 -v "$PWD/data:/data" \
  betrockpp:server --port 25565 --max_players 10 --whitelist
```

## Extracting a built binary (e.g. the client)

If you just want the compiled executable rather than a running container:

```bash
docker build --build-arg BUILD_TARGET=client -t betrockpp:client-builder --target builder .
docker create --name bpp-extract betrockpp:client-builder
docker cp bpp-extract:/src/build/BetrockPlusPlus ./BetrockPlusPlus
docker rm bpp-extract
```

## Build arguments reference

| Arg           | Values                          | Default   | Effect                                   |
|---------------|----------------------------------|-----------|-------------------------------------------|
| `BUILD_TARGET`| `server`, `client`               | `server`  | Which CMake target to compile             |
| `BUILD_TYPE`  | `Release`, `Debug`, `MinSizeRel` | `Release` | CMake build type                          |
| `COMPILER`    | `clang`, `gcc`                   | `clang`   | Which compiler to use inside the container|
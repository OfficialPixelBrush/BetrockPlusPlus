> [!NOTE]
> This whole file, alongside the docker-compose and Dockerfile were AI generated.
> These instructions and files have only been generated because none of
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

## Building the server

```bash
docker build -t betrockpp:server .
```

Or with Compose (also sets up a persistent world/config folder, see below):

```bash
docker compose build
```

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

## Build arguments reference

| Arg          | Values                           | Default   | Effect                                     |
| ------------ | -------------------------------- | --------- | ------------------------------------------ |
| `BUILD_TYPE` | `Release`, `Debug`, `MinSizeRel` | `Release` | CMake build type                           |
| `COMPILER`   | `clang`, `gcc`                   | `clang`   | Which compiler to use inside the container |

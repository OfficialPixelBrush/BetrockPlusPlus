# Optional Flags

Optional Features are settings that we expose at compile-time for people that want specific features, without unnecessarily inflating compile time, binary size or the number of necessary dependencies for those that don't want them.

For example, if you'd like to disable Online Mode Authencation, but enable Discord Integration, you'd type:

```bash
cmake -S . -B build -DONLINE_MODE_AUTHENTICATION=OFF -DDISCORD_INTEGRATION=ON
```

### Online Mode Authentication (ON)

Online Mode Authentication is **on by default**. It allows users to be authenticated via the legacy Minecraft Login protocol. By default this goes through the Betacraft.uk proxy, since the original authentication servers got shut down long ago. It requires `libcurl`.

Simply add `-DONLINE_MODE_AUTHENTICATION=OFF` to the first build command, if you'd like to disable it. This removes all auth-related code.

### Betacraft Server List Heartbeat (OFF)

Betacraft heartbeat is **off by default**. Enabling it lets a dedicated server ping [the Betacraft list](https://betacraft.uk/serverlist). It requires `libcurl` and [nlohmann/json](https://github.com/nlohmann/json) (via vcpkg feature `betacraft-heartbeat`, or fetched by CMake).

Add `-DBETACRAFT_HEARTBEAT=ON` to the first build command.

To appear on the list, contact Moresteck on the [Betacraft Discord](https://betacraft.uk/) for a private key assigned to your address, then set these in `server.properties`:

| Key                      | Purpose                                                                                |
| ------------------------ | -------------------------------------------------------------------------------------- |
| `betacraft-heartbeat`    | `true` to ping `https://api.betacraft.uk/v2/server_update` every 60 seconds            |
| `betacraft-name`         | List name (max 64 characters)                                                          |
| `betacraft-description`  | List description (max 256 characters). State whitelist/anarchy clearly if that applies |
| `betacraft-socket`       | Public `host:port` players should join. Empty auto-detects public IP + `server-port`   |
| `betacraft-private-key`  | Verification key from Betacraft (required for category, domain names, and icons)       |
| `betacraft-category`     | `classic`, `indev`, `infdev`, `alpha`, `beta`, or `release`                            |
| `betacraft-game-version` | Version ID as shown in the Betacraft launcher (default `b1.7.3`)                       |
| `betacraft-protocol`     | Protocol ID from Betacraft version metadata (default `beta_14` for Beta 1.7.3)         |
| `betacraft-v1-version`   | Version string for Betacraft launcher v1                                               |
| `betacraft-send-players` | `true` shares online usernames; `false` only shares the player count                   |
| `betacraft-icon`         | Optional path to a PNG icon (max 128×128 and 64 KiB)                                   |

### Discord Integration (OFF)

Discord support is **off by default**. Enabling it pulls in [DPP](https://dpp.dev/) (Gateway WebSocket bot) via `vcpkg` or `FetchContent`, and requires OpenSSL.

Simply add `-DDISCORD_INTEGRATION=ON` to the first build command, then resume as normal.

In `server.properties`:

| Key                     | Purpose                                                                                      |
| ----------------------- | -------------------------------------------------------------------------------------------- |
| `discord-token`         | Bot token                                                                                    |
| `discord-channel-id`    | Channel used for chat bridge + crash uploads                                                 |
| `discord-webhook-url`   | Makes it so player PFPs and names are used in messages, integrating them better with Discord |
| `discord-admin-role-id` | Optional. Allows for more invasive commands to be run from Discord (i.e. `stop`)             |
| `discord-guild-id`      | Optional. When set, slash commands register to that guild instantly                          |

In the [Discord Developer Portal](https://discord.com/developers/applications), enable the **Message Content Intent**, invite the bot with `applications.commands` + `bot` scopes, and grant read/send message permissions in the bridge channel.

> **Windows note:** if installing DPP through `vcpkg`, use a non-static triplet (`x64-windows`, not `x64-windows-static`).

### Terrain Precision (DOUBLE)

Terrain Precision is set to **`DOUBLE` by default**. Setting it to another value changes the numerical precision of the Perlin and Simplex noise generators.

There exist four options, listed from most to least supported/usable.

| Value    | Description                                                                                                                                                                                                                                                                                                                                                                   | Consequences                                                                                                                                                                                                                              |
| -------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `DOUBLE` | Uses a **64-Bit floating point number**. The default, supported mode                                                                                                                                                                                                                                                                                                          | None. This is identical to Vanilla Minecraft                                                                                                                                                                                              |
| `FLOAT`  | Uses a **32-Bit floating point number**. This shouldn't do much on systems with a dedicated floating-point co-processor, such as an Intel 8087, or integrated floating-point functionality, like most x86 CPUs made after ~1987, as they use the same 80-Bit registers for 32-bit and 64-bit floating-point math, the only difference being potential memory bandwidth usage. | _VERY_ slightly different terrain. The only major difference is that the farlands do not generate, and they just become an infinite ocean with a bedrock floor along the X-Axis, and the same but with a grid of blocks along the Z-Axis. |
| `LONG`   | Uses a **36.28 Fixed-point number**, removing some floating-point overhead                                                                                                                                                                                                                                                                                                    | Generation is idential to `DOUBLE`, except at the farlands, where they appear more compressed.                                                                                                                                            |
| `INT`    | Uses a **18.14 Fixed-point number**, removing some floating-point overhead                                                                                                                                                                                                                                                                                                    | Generation can differ noticably under some circumstances, even near 0,0. The farlands do not generate at their usual location.                                                                                                            |

The main benefits for this are for some microcontrollers or cost-reduced x86 chips that don't have integrated floating-point support, and thus need to emulate it all in software. Examples for such include RISC-V cores that lack the F (float) and D (double) extensions (e.g. RV32I, RV64IM) or the i486SX.

Simply add `-DGENERATION_PRECISION=`, followed by your desired precision type, to the first build command, then resume as normal.

## Continuing

[Please continue after the first command of Step #3 in BUILDING](./BUILDING.md#3-building).

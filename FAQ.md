# FAQ

We get a lot of repeated questions or bug reports in our Discord server, so this'll serve as a quick reference for the most commonly asked questions or reported issues that aren't issues.

## Questions

In this category go some commonly asked questions. These won't cover questions relating to why the project exists or its legal sitation.

### When Client?

Whenever we get to it. There's no set timeline on this yet. As it stands its not a priority until the Server is feature-complete.

### Why does this crafting recipe not work?

There are two options.

1. We haven't implemented the recipe yet (VERY unlikely at this point)
2. You're running a modified client that changes the crafting recipe

#2 is the most common reason, as the client tends to (visually) prioritize what it thinks is right, even if the server disagrees.

### Support for older Operating Systems (e.g. WinXP or 9x)

While this isn't impossible, these older OS' lack features we rely on at a fundamental level, such as sockets. It'd require a very extensive rewrite, or at least one that'd force us to use an older C++ version, which would just make the codebase a lot more painful to work with.

In the worst case, it'd even degrade performance on more modern systems, since we'd likely need to reoptimize/restructure our code for OS' that don't support multi-threading very well, if at all.

Install a more up-to-date, supported OS. Linux can run on a lot of ancient hardware, and can be made to run on pretty much anything made after 1995, or make your own fork specifically for your platform of choice.

## Unsolvable bugs

These are "bugs" or issues we cannot resolve, as they are either client-side bugs that Beta 1.7.3 had, packets or features Beta 1.7.3 lacked or issues caused by mods people like to use that we have no control over.

### Mob Spawners always show up as Pig Spawners

This is a protocol issue, as there wasn't a Packet to tell what mob a spawner spawned until Release 1.2.1

### A lot of things don't play sounds

Very few sounds were networked by the time Beta 1.7.3 rolled around. A few examples include:

- Buttons + Pressure plates (un)clicking
- Players taking damage/dying
- Other players walking around
- Block placements done by other players

### I can't craft

This often happens when a mod is used that modifies the Clients crafting recipies. The client tends to prefer it's own crafting recipe results over whatever the server thinks.

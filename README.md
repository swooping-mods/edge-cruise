# Edge Cruise Upgrade

<img src="media/icon.png" alt="Edge Cruise Upgrade icon" width="128" height="128">

Purchased car upgrade follows cleared dust edges during cruise.

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [edge-cruise-0.2.0.msmod](content/edge-cruise-0.2.0.msmod), then drag and drop the `.msmod` file onto the [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) window. You can also use **Add mods…** (older loader builds: **Install mod package**).

Select the mod and its required frameworks, click **Prepare files**, then **Launch**.

Required mods: [shop-framework](https://github.com/swooping-mods/shop-framework), [hints-framework](https://github.com/swooping-mods/hints-framework), [vehicle-control-framework](https://github.com/swooping-mods/vehicle-control-framework).

## Controls

- Board a cruise-equipped car once, then buy its $350 Edge Cruise upgrade in Modded Shop (Ctrl+Shift+F2).
- Drive beside a cleared dust edge, enable cleaning and vanilla cruise, then press J.
- Manual steering, J, edge loss or a sharp curve releases assist and stops cruise.

## Compatibility

Version 0.2.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).

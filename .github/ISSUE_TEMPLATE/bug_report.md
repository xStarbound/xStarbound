---
name: Bug report
about: Report a bug or crash
title: "[BUG] <Short description of bug here>"
labels: bug
assignees: FezzedOne

---

## Description

[Describe the bug here.]

## Reproduction steps

1. [Describe any steps needed to reproduce the bug here. Please be as detailed as you can.]
2. [...]
3. [...]

< Delete this text and everything below it before posting your issue! >

## Logs and screenshots

In order to make debugging easier on the devs, you should upload and attach any appropriate log files. The following log files are used by xStarbound, where `$n` is a backup rotation number for logs from previous sessions (the higher the number, the *older*):

- **Your `xclient.log` and `xclient.log.$n`:** Your own client's `xclient.log`. These are required to debug client-side issues and single-player server-side issues.
- **Steam host's `xclient.log` and `xclient.log.$n`:** Your *host*'s `xclient.log`. These are required to debug server-side issues on hosted games. These are located at the same paths as for your client (above).
- **`xserver.log` and `xserver.log.$n`:** The logs for the dedicated xServer server. These are required to debug server-side issues on dedicated servers running xServer.

Depending on your OS and what version of Starbound you have, the log files are located in the following directories:

- **Linux/SteamOS (Steam):** Defaults to `~/.local/share/Steam/steamapps/common/Starbound/storage/`. Replace `~/.local/share/Steam/steamapps/common/` with the path to your custom installation directory for Starbound, if you use one.
- **Linux/SteamOS (other):** `$starboundDir/storage/`, where `$starboundDir` is the path to your Starbound install.
- **Windows (Steam):** Defaults to `C:\Program Files\Steam\steamapps\common\Starbound\storage\`. Replace `C:\Program Files (x86)\Steam\steamapps\common\` with the path to your custom installation directory for Starbound, if you use one.
- **Windows (GOG):** Defaults to `C:\GOG Games\Starbound\storage\`.
- **Windows (Xbox Live):** Defaults to `C:\Program Files\WindowsApps\Starbound\storage\`, since `xsbinit.config` by default does *not* use the saves in your `Documents\` folder.
- **Windows (other):** `%starboundDir%\storage\`, where `%starboundDir%` is the path to your Starbound install.

**IF YOU'RE ON WINDOWS, READ THE FOLLOWING!**

If you're reporting a crash on the MSVC / Visual Studio build on Windows, your log files are virtually **USELESS** if the `.pdb` files are not installed. If the crash happened on an MSVC build without the `.pdb` files installed, install those files and then attempt to reproduce the crash before submitting the issue or uploading logs.

To install the PDB files:

1. Download the `windows-pdbs.zip` file for the version of xStarbound you're using and then extract it.
2. Once extracted, copy the `.pdb` files into the folder containing the executables of the same names. Depending on the version of Starbound you own, this folder is located at:
  - **Windows (Steam):** Defaults to `C:\Program Files\Steam\steamapps\common\Starbound\xsb-win64\`. Replace `C:\Program Files (x86)\Steam\steamapps\common\` with the path to your custom installation directory for Starbound, if you use one.
  - **Windows (GOG):** Defaults to `C:\GOG Games\Starbound\xsb-win64\`.
  - **Windows (Xbox Live):** Defaults to `C:\Program Files\WindowsApps\Starbound\xsb-win64\`.
  - **Windows (other):** If you've followed the manual installation instructions or used the installer, the executables will be in `%starboundDir%\xsb-win64\`, where `%starboundDir%` is the path to your Starbound install.
3. Restart xStarbound and attempt to reproduce the crash.

**If you're reporting a crash on the dynamically linked Linux build, try to reproduce the crash on the statically linked build — or on a `RelWithDebInfo` or `Debug` build you've built yourself — before submitting a bug report.**
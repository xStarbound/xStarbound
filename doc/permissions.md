# xServer's permissions system

This file explains how xServer's build permissions system works.

## Basic configuration

The basic configuration is defined under `"buildPermissionSettings"` in the server's `xserver.config` file or the host's `xclient.config` file. Shown below are the defaults written to a server or host's `xserver.config` if there's no existing config file (or it is invalid JSON).

```json
"buildPermissionSettings" : {
  "enabled" : false,
  "guestWorldSpawnsAllowed" : true,
  "guestBuildingAllowed" : true,
  "accountClaimsEnabled" : false,
  "disallowServerGriefingWhenOwned" : false,
  "containerModificationProtection" : false,
  "containerOpenProtection" : false
}
```

> **Note:** The `"buildPermissionSettings"` object is not required to be present in a server's config, so it is safe to copy your vanilla server config. Obviously, you'll need to set up your server's build permission settings and command scripts if you wish to use xServer's build protection system.

A description of each of the settings:

- **`"enabled"`:** Whether the build permissions system is enabled. Required for all other permissions settings to take effect. If enabled, any configured UUID-based world and system claims are handled, and ship worlds are automatically implicitly claimed by the clients that own them, protecting them from modifications by non-owning clients.
- **`"guestWorldSpawnsAllowed"`:** Whether non-admin clients are allowed to spawn stations and other worlds in unclaimed systems. If `false`, any client that wants to spawn a system world or object must first claim the system or get build access to it.
- **`"guestBuildingAllowed"`:** Whether non-admin clients are allowed to build (and/or open or modify containers) on unclaimed worlds. If `false`, any client that wants to build on a world (and/or open or modify containers on it) must first claim the world or get build access to it.
- **`"accountClaimsEnabled"`:** Whether claims tied to server accounts are handled. Account claims take precedence over UUID-based claims. Account-based claims are more secure and convenient than UUID-based claims — the _account_ owns the claims, no matter which character the client logs in under, and are immune to potential UUID spoofing.
- **`"disallowServerGriefingWhenOwned"`:** Whether server-side entity or world projectiles, scripted entities, etc., are allowed to add, damage or remove tiles, matmods or objects on claimed worlds. Enabling this prevents random weather events, NPC/monster projectiles, etc., from damaging players' (or admins') claimed builds, but may interfere with certain server-side mod scripts that expect to be able to «build» on claimed worlds. This setting, if enabled, can be overridden on a per-world-claim basis if `false` (allowing server-side «griefing» and «building») or `true` (allowing anyone, including the server, to build) is present in the claim's `"allowedBuilders"` array.
- **`"containerModificationProtection"`:** Whether containers on claimed worlds are protected from modification by clients who do not have build access to claimed worlds. This setting, if enabled, can be overridden on a per-world-claim basis if `"allowGuestContainerModification": true` is present in the claim's settings. This protects against _all_ forms of container modification, but not against modded container script message handlers that aren't aware of xServer's permissions system. Modders, take note (and see `$docs/lua/message.md` for permission control prefixes on your messages)! (No such handlers exist in the vanilla assets or in most mods.)
- **`"containerOpenProtection"`:** Whether containers on claimed worlds are protected from being opened and viewed by clients who do not have build access to claimed worlds. If enabled, this also implicitly enables `"containerModificationProtection"`, regardless of its explicit status in the configuration. This setting, if enabled, can be overridden on a per-world-claim basis if `"allowGuestContainerOpening": true` or `"allowGuestContainerModification": true` is present in the claim's settings.
  - _Caveats:_ Client-side mods may allow clients to view and copy the contents of containers without interacting with them. xServer does not stop this from happening, but _does_ stop clients from changing the contents of protected containers. If you're worried about stealthy item duping for any reason, consider telling players to be careful of who they invite to their ships, or banning or restricting client-side mods.

## `"ownedSystemsByAccount"` and `"ownedSystemsByUuid"`

These two optional JSON objects define the configured star system claims on the server. You must have account claims enabled for `"ownedSystemsByAccount"` to take effect. The claim configurations have the following format:

```json
"ownedSystemsByAccount" : { // Is a JSON object that acts as a system coordinate map for claims.
  "34:-12:665833932" : { // The system's coordinates in `"X:Y:Z"` format.
    "owner" : "fezzedone", // The owning account. Use this for claim ownership checks in your command scripts.
    "allowedBuilders" : [ "sanjay", "michael" ] // Accounts that are allowed to spawn objects in the system.
      // If `true` is present in this array, it acts as a wildcard that gives permissions to everyone,
        // including clients logged in without an account.
  }
},
"ownedSystemsByUUID" : { // Is a JSON object that acts as a system coordinate map for claims.
  "38:-16:54372272" : { // The system's coordinates in `"X:Y:Z"` format.
    "owner" : "3cc245c853cd4e8e83dce8de71602b65", // The owning client uuidString. Use this for claim ownership checks in your command scripts.
    "allowedBuilders" : [ "1616ea01ba454d23a426a899d0123c17", "a3906c12984142e5a3865b8f31408e13" ]
      // Client UUIDs that are allowed to spawn objects in the system.
      // If `true` is present in this array, it acts as a wildcard that gives permissions to everyone,
        // including clients logged in without an account.
  }
},
```

Note that claiming a system does _not_ automatically claim the worlds in that system, which must be claimed separately.

## `"ownedWorldsByAccount"` and `"ownedWorldsByUuid"`

These two optional JSON objects define the configured world claims on the server. You must have account claims enabled for `"ownedWorldsByAccount"` to take effect. The claim configurations have the following format:

```json
"ownedWorldsByAccount" : { // Is a JSON object that acts as a world ID map for claims.
  "CelestialWorld:-201777753:-398423631:8256823:11" : {
    "owner" : "fezzedone", // The owning account. Use this for claim ownership checks in your command scripts.
    "allowedBuilders" : [ "john", "rodriguez", "meilin", false ] // Accounts that are allowed to build
        // on the world. They are also allowed to open and modify containers if this would overwise be
        // disallowed for non-owning clients.
      // If `false` is present in this array, server-side projectiles, scripts, etc., are allowed to «build»
        // on the world or «grief» it if this would otherwise be disallowed for claimed worlds. Here,
        // FezzedOne has enabled «griefing» because he has a few Frackin' Universe quarries on this world.
      // If `true` is present in this array, it acts as a wildcard that gives permissions to everyone,
        // including server-side scripts and clients without an account, to build on the world; it also
        // gives every client permission to open or modify containers if this would otherwise be disallowed
        // for non-owning clients.
  }
},
"ownedWorldsByUuid" : { // Is a JSON object that acts as a world ID map for claims.
  "CelestialWorld:-201777753:-398423631:8256823:8:2" : {
    "owner" : "3cc245c853cd4e8e83dce8de71602b65", // The owning client UUID. Use this for claim ownership checks in your command scripts.
    "allowedBuilders" : [ "2e49a7b16636439f815f3dec89fed405", "fcedfa377c1146cba4206ee123e90848" ]
      // Client UUIDs that are allowed to build on the world. They are also allowed to open and modify
        // containers if this would overwise be disallowed for non-owning clients.
      // If `false` is present in this array, server-side projectiles,
        // scripts, etc., are allowed to «build» on the world or «grief» it if this would otherwise
        // be disallowed for claimed worlds.
      // If `true` is present in this array, it acts as a wildcard that gives permissions to everyone,
        // including server-side scripts and clients without an account, to build on the world; it also
        // gives every client permission to open or modify containers if this would otherwise be disallowed
        // for non-owning clients.
  }
}
```

## Updating claims and configuration

If the configuration or claim maps are updated via `root.setConfiguration` or `root.setConfigurationPath`, the changes take effect immediately. If updated with an external editor, the server should be shut down while changes are made in order to prevent it potentially resetting the configuration file back to its state before your changes.

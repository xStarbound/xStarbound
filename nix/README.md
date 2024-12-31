# xStarbound with Nix

xStarbound features first-class [Nix](https://nixos.org/) support. Everything is mostly OOTB and painless. 

Basic familiarity with 

  - [flakes](https://www.tweag.io/blog/2020-05-25-flakes/) 
  - [overriding](https://nixos.org/guides/nix-pills/14-override-design-pattern)
  - [overlays](https://wiki.nixos.org/wiki/Overlays)

is highly recommended. 

Only flake usage is *officially* supported but you could probably make flakeless usage work with some elbow grease.

Assuming a relatively standad system, you can simply run xStarbound like so:
```
$ nix run github:xstarbound/xstarbound
```

To make configuration changes however, you should add xStarbound to your configuration.  
Any configuration is done through normal Nix overriding. The configuration is resolved
by chaining overlays.

An example can be found in `./flake-example.nix`.

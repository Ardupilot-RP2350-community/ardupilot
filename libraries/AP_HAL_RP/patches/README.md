Place patch files for pico-sdk in this directory to have them auto-applied
during RP2350 configure/build.

Behavior:
- Files ending with `.patch` are processed in lexical order.
- Each patch is applied with `git -C $PICO_SDK_PATH apply <patch>`.
- If a patch is already applied, it is detected and skipped.
- If a patch no longer matches the installed pico-sdk version, configure fails.

Typical usage:
1. Create a patch against your pico-sdk checkout.
2. Copy it here with a numbered name, for example:
   `0001-pico-runtime-no-malloc-wrappers.patch`
3. Re-run `./waf configure --board <rp2350_board>` and build.

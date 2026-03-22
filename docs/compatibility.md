# Compatibility

## Platform Requirements

|                  |                                                         |
| ---------------- | ------------------------------------------------------- |
| **OS**           | macOS only (Windows / Linux not supported at this time) |
| **Architecture** | Apple Silicon (arm64) — Intel Macs are not supported    |
| **Formats**      | AUv2, VST3                                              |

> The distributed binary is arm64-only and is not a universal build.  
> Intel Macs cannot run arm64 binaries — there is no workaround.  
> Note: if you are on Apple Silicon but running an Intel-only DAW under Rosetta 2,
> the plugin must also be an Intel (x86_64) build, which is currently not provided.

---

Tested environments are listed below. If you have tested BoomBaby on a configuration not listed below, please share your results in [GitHub Discussions](https://github.com/auditive-tokyo/BoomBaby/discussions/1) — your contribution will help the community. Thank you!

## Tested Configurations

| DAW             | OS                   | CPU          | Format     | Status   |
| --------------- | -------------------- | ------------ | ---------- | -------- |
| Ableton Live 12 | macOS 15.3.1 (24D70) | Apple M4 Pro | AUv2, VST3 | ✅ Works |
| Logic Pro       | —                    | —            | AUv2       | —        |
| Cubase          | —                    | —            | VST3       | —        |
| Bitwig Studio   | —                    | —            | VST3       | —        |
| Reaper          | —                    | —            | AUv2, VST3 | —        |
| Studio One      | —                    | —            | AUv2, VST3 | —        |

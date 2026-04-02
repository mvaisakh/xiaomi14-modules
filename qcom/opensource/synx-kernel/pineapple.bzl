load(":synx_modules.bzl", "synx_modules")
load(":synx_module_build.bzl", "define_consolidate_gki_modules")

def define_pineapple():
  for platform in ["pineapple", "houji"]:
    define_consolidate_gki_modules(
        target = platform,
        registry = synx_modules,
        modules = [
            "synx-driver",
            "ipclite",
            "ipclite_test",
        ],
        config_options = [
            "TARGET_SYNX_ENABLE",
        ],
    )

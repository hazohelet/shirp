filegroup(
    name = "tools",
    srcs = glob(
        [
            "*.c",
            "*.h",
        ],
    ),
    data = [
        "prelude.scm",
    ],
    visibility = ["//test:__pkg__"],
)

cc_binary(
    name = "shirp",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    copts = [
        "--undefine-macro DEBUG",
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Werror",
    ],
    data = [
        "prelude.scm",
    ],
)

cc_binary(
    name = "shirp_debug",
    srcs = glob([
        "*.c",
        "*.h",
    ]),
    copts = [
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Werror",
    ],
    data = [
        "prelude.scm",
    ],
    defines = ["DEBUG"],
)

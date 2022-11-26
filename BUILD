cc_binary(
    name = "shirp",
    srcs = [
        "main.c",
        "object.c",
        "parser.c",
        "scan.c",
        "shirp.h",
        "utils.c",
    ],
    copts = [
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Werror",
    ],
)

cc_binary(
    name = "shirp_debug",
    srcs = [
        "main.c",
        "object.c",
        "parser.c",
        "scan.c",
        "shirp.h",
        "utils.c",
    ],
    copts = [
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Werror",
    ],
    defines = ["DEBUG"],
)

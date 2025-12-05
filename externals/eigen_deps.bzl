load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _eigen_ext_impl(module_ctx):
    # 보통 root 모듈(= 현재 workspace)에서만 레포를 만들도록 하는 패턴
    http_archive(
        name = "eigen",   # 나중에 @eigen 으로 참조
        urls = [
            "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz",
        ],
        strip_prefix = "eigen-3.4.0",

        # 이 tarball 안에는 BUILD 파일이 없으니, 여기서 직접 하나 만들어 줍니다.
        build_file_content = """
cc_library(
    name = "eigen",
    hdrs = glob(["Eigen/**"]),
    includes = ["."],
    visibility = ["//visibility:public"],
)
""",
    )

eigen_ext = module_extension(
    implementation = _eigen_ext_impl,
)

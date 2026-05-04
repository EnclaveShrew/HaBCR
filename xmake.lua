set_xmakever("3.0.0")

set_project("HaBCR_AE")
set_languages("c++23")
set_encodings("utf-8")
set_warnings("allextra")

add_rules("mode.debug", "mode.releasedbg")

includes("../../CommonLibF4-AE")

target("HaBCR_AE", function()
    set_kind("shared")
    set_version("1.0.0")

    add_deps("commonlibf4", { public = true })

    add_rules("commonlibf4.plugin", {
        name = "HaBCR_AE",
        author = "",
        description = "Havok-Aware Bullet Counted Reload (AE 1.11.191)",
    })

    add_defines("HABCR_VARIANT_AE=1", "_UNICODE")

    add_files("src/**.cpp")

    add_headerfiles(
        "(src/**.h)",
        "(extras_ae/**.h)"
    )

    add_includedirs("src", "extras_ae")

    set_pcxxheader("src/PCH.h")
end)

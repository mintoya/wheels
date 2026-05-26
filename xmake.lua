target("wheels")
    set_kind("headeronly")
    add_headerfiles("*.h", {prefixdir = "wheels"})
    add_includedirs("..", {public = true})

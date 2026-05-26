set_project("wheels")
set_languages("c23")

add_rules("mode.debug", "mode.release")

target("wheels")
  set_kind("headeronly")
  add_includedirs(".")
  add_headerfiles("*.h")

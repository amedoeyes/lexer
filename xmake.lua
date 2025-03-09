set_project("lexer")
set_version("0.1.0")
set_languages("cxx23")
set_toolchains("clang")
set_runtimes("c++_shared")

add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.check", "mode.profile")
add_rules("plugin.compile_commands.autoupdate", { lsp = "clangd", outputdir = "build" })

target("lexer", function()
	set_kind("static")
	add_files("src/**.cppm", { public = true })
end)

for _, file in ipairs(os.files("examples/**.cpp")) do
	target(path.basename(file), { files = file, deps = "lexer" })
end

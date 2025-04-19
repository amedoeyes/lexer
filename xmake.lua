set_project("lexer")
set_version("2.0.0")
set_languages("cxx23")
set_toolchains("clang")
set_runtimes("c++_shared")

add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.check", "mode.profile")
add_rules("plugin.compile_commands.autoupdate", { lsp = "clangd", outputdir = "build" })

target("lexer", function()
	set_kind("static")
	add_files("src/**.cppm", { public = true })
	add_cxxflags(
		"-Weverything",
		"-Wno-c++98-compat",
		"-Wno-c++98-compat-pedantic",
		"-Wno-ctad-maybe-unsupported",
		"-Wno-exit-time-destructors",
		"-Wno-float-equal",
		"-Wno-global-constructors",
		"-Wno-missing-designated-field-initializers",
		"-Wno-missing-prototypes",
		"-Wno-missing-variable-declarations", -- dosen't work with modules
		"-Wno-padded",
		"-Wno-reserved-identifier", --doesn't work with modules
		"-Wno-shadow-field-in-constructor",
		"-Wno-switch-default",
		"-Wno-switch-enum",
		"-Wno-unsafe-buffer-usage-in-container",
		"-Wno-weak-vtables"
	)
end)

option("examples", {
	description = "Build examples",
	showmenu = true,
	default = false,
	type = "boolean",
})

if has_config("examples") then
	for _, example in ipairs(os.files("examples/*.cpp")) do
		target(path.basename(example), { files = example, deps = "lexer" })
	end
end

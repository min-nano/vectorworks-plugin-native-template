//
//	UpdaterParseTests.cpp
//
//	Unit tests for the SDK-independent updater helpers in src/UpdaterParse.h:
//	the script-output parsers (Trim / ValueOf / ParseDevBuilds), the two
//	command-line quoters (ShellQuote / CmdQuote), and the path derivations
//	(Mac*/Win* FromBinary/FromPath/FromDir).
//
//	These are the parts of Updater.cpp that carry real branching logic yet do NOT
//	touch the Vectorworks SDK, so they can be exercised exhaustively on a plain
//	toolchain — which is what the coverage job measures.
//

#include "TestFramework.h"
#include "UpdaterParse.h"

#include <string>
#include <vector>

using namespace SamplePlugin::UpdaterParse;

// ---------------------------------------------------------------------------
// Trim
// ---------------------------------------------------------------------------

TEST(trim_removes_surrounding_whitespace)
{
	CHECK_EQ(Trim("  hello  "), "hello");
	CHECK_EQ(Trim("\t\r\n value \n\t"), "value");
	CHECK_EQ(Trim("no-padding"), "no-padding");
}

TEST(trim_all_whitespace_or_empty_is_empty)
{
	CHECK_EQ(Trim(""), "");
	CHECK_EQ(Trim("   \t\r\n  "), "");
}

TEST(trim_preserves_interior_whitespace)
{
	CHECK_EQ(Trim("  a b\tc  "), "a b\tc");
}

// ---------------------------------------------------------------------------
// ValueOf
// ---------------------------------------------------------------------------

TEST(valueof_finds_key_and_trims_value)
{
	const std::string out =
		"installed=abc123\n"
		"latest=def456\n"
		"url=https://example.com/x.zip\n";
	CHECK_EQ(ValueOf(out, "installed"), "abc123");
	CHECK_EQ(ValueOf(out, "latest"), "def456");
	CHECK_EQ(ValueOf(out, "url"), "https://example.com/x.zip");
}

TEST(valueof_missing_key_is_empty)
{
	const std::string out = "installed=abc123\nlatest=def456\n";
	CHECK_EQ(ValueOf(out, "error"), "");
	CHECK_EQ(ValueOf(out, "nope"), "");
}

TEST(valueof_returns_first_match)
{
	const std::string out = "key=first\nkey=second\n";
	CHECK_EQ(ValueOf(out, "key"), "first");
}

TEST(valueof_trims_value_whitespace)
{
	const std::string out = "key=   spaced value   \n";
	CHECK_EQ(ValueOf(out, "key"), "spaced value");
}

TEST(valueof_handles_last_line_without_newline)
{
	// No trailing '\n' on the final line: the loop must still match it.
	const std::string out = "a=1\nlatest=v9";
	CHECK_EQ(ValueOf(out, "latest"), "v9");
}

TEST(valueof_empty_value)
{
	const std::string out = "error=\ninstalled=abc\n";
	// error is present but blank -> empty string (distinct from "missing", but
	// both render as "").
	CHECK_EQ(ValueOf(out, "error"), "");
	CHECK_EQ(ValueOf(out, "installed"), "abc");
}

TEST(valueof_does_not_match_key_as_substring)
{
	// "installed" must not be returned when asking for "install".
	const std::string out = "installed=abc\n";
	CHECK_EQ(ValueOf(out, "install"), "");
}

TEST(valueof_empty_input)
{
	CHECK_EQ(ValueOf("", "key"), "");
}

// ---------------------------------------------------------------------------
// ParseDevBuilds
// ---------------------------------------------------------------------------

TEST(parsedevbuilds_parses_multiple_rows)
{
	const std::string out =
		"channel=dev\n"
		"build\tc0ffee1\tfeature/one\thttps://ex.com/one.zip\n"
		"build\tdeadbee\tfeature/two\thttps://ex.com/two.zip\n";

	std::vector<DevBuild> builds = ParseDevBuilds(out);
	CHECK_EQ(builds.size(), static_cast<std::size_t>(2));
	if (builds.size() == 2)
	{
		CHECK_EQ(builds[0].commit, "c0ffee1");
		CHECK_EQ(builds[0].name, "feature/one");
		CHECK_EQ(builds[0].url, "https://ex.com/one.zip");
		CHECK_EQ(builds[1].commit, "deadbee");
		CHECK_EQ(builds[1].name, "feature/two");
		CHECK_EQ(builds[1].url, "https://ex.com/two.zip");
	}
}

TEST(parsedevbuilds_ignores_non_build_lines)
{
	const std::string out =
		"error=\n"
		"note: this is not a build line\n"
		"build\tc1\tbranch\thttps://ex.com/a.zip\n";
	std::vector<DevBuild> builds = ParseDevBuilds(out);
	CHECK_EQ(builds.size(), static_cast<std::size_t>(1));
	if (!builds.empty())
		CHECK_EQ(builds[0].commit, "c1");
}

TEST(parsedevbuilds_skips_rows_missing_fields)
{
	const std::string out =
		"build\tonlyonefield\n"                       // no tabs after -> skip
		"build\tc1\tbranch-no-url\n"                   // only two fields -> skip
		"build\tc2\tbranch\thttps://ex.com/ok.zip\n";  // complete -> keep
	std::vector<DevBuild> builds = ParseDevBuilds(out);
	CHECK_EQ(builds.size(), static_cast<std::size_t>(1));
	if (!builds.empty())
	{
		CHECK_EQ(builds[0].commit, "c2");
		CHECK_EQ(builds[0].url, "https://ex.com/ok.zip");
	}
}

TEST(parsedevbuilds_skips_row_with_empty_url)
{
	// Three tab-separated fields present, but the url field is blank -> skipped.
	const std::string out = "build\tc1\tbranch\t\n";
	std::vector<DevBuild> builds = ParseDevBuilds(out);
	CHECK_EQ(builds.size(), static_cast<std::size_t>(0));
}

TEST(parsedevbuilds_trims_fields)
{
	const std::string out = "build\t  c1  \t  branch  \t  https://ex.com/a.zip  \n";
	std::vector<DevBuild> builds = ParseDevBuilds(out);
	CHECK_EQ(builds.size(), static_cast<std::size_t>(1));
	if (!builds.empty())
	{
		CHECK_EQ(builds[0].commit, "c1");
		CHECK_EQ(builds[0].name, "branch");
		CHECK_EQ(builds[0].url, "https://ex.com/a.zip");
	}
}

TEST(parsedevbuilds_handles_last_row_without_newline)
{
	const std::string out = "build\tc1\tbranch\thttps://ex.com/a.zip";
	std::vector<DevBuild> builds = ParseDevBuilds(out);
	CHECK_EQ(builds.size(), static_cast<std::size_t>(1));
	if (!builds.empty())
		CHECK_EQ(builds[0].url, "https://ex.com/a.zip");
}

TEST(parsedevbuilds_empty_and_no_matches)
{
	CHECK_EQ(ParseDevBuilds("").size(), static_cast<std::size_t>(0));
	CHECK_EQ(ParseDevBuilds("nothing here\nat all\n").size(),
			 static_cast<std::size_t>(0));
}

// ---------------------------------------------------------------------------
// ShellQuote
// ---------------------------------------------------------------------------

TEST(shellquote_wraps_in_single_quotes)
{
	CHECK_EQ(ShellQuote("hello"), "'hello'");
	CHECK_EQ(ShellQuote("/path/to/thing"), "'/path/to/thing'");
	CHECK_EQ(ShellQuote(""), "''");
}

TEST(shellquote_escapes_embedded_single_quote)
{
	// The classic '\'' escape sequence: close, escaped quote, reopen.
	CHECK_EQ(ShellQuote("a'b"), "'a'\\''b'");
	CHECK_EQ(ShellQuote("'"), "''\\'''");
}

TEST(shellquote_passes_through_spaces_and_specials)
{
	// Inside single quotes these are all literal, so nothing extra is escaped.
	CHECK_EQ(ShellQuote("a b c"), "'a b c'");
	CHECK_EQ(ShellQuote("$HOME `x` \"y\""), "'$HOME `x` \"y\"'");
}

// ---------------------------------------------------------------------------
// CmdQuote
// ---------------------------------------------------------------------------

TEST(cmdquote_wraps_in_double_quotes)
{
	CHECK_EQ(CmdQuote("hello"), "\"hello\"");
	CHECK_EQ(CmdQuote("C:\\Plug-Ins\\x.vlb"), "\"C:\\Plug-Ins\\x.vlb\"");
	CHECK_EQ(CmdQuote(""), "\"\"");
}

TEST(cmdquote_drops_embedded_double_quotes)
{
	CHECK_EQ(CmdQuote("a\"b\"c"), "\"abc\"");
	CHECK_EQ(CmdQuote("\"\""), "\"\"");
}

TEST(cmdquote_keeps_spaces)
{
	CHECK_EQ(CmdQuote("a b"), "\"a b\"");
}

// ---------------------------------------------------------------------------
// MacScriptPathFromBinary
// ---------------------------------------------------------------------------

TEST(mac_script_path_from_binary_typical)
{
	const std::string bin =
		"/Users/me/Vectorworks/Plug-Ins/SamplePlugin.vwlibrary/Contents/MacOS/SamplePlugin";
	CHECK_EQ(MacScriptPathFromBinary(bin),
		"/Users/me/Vectorworks/Plug-Ins/SamplePlugin.vwlibrary/Contents/Resources/vw-update.sh");
}

TEST(mac_script_path_from_binary_uses_last_marker)
{
	// A stray "/Contents/MacOS/" earlier in the path must not fool rfind.
	const std::string bin =
		"/Contents/MacOS/weird/Plug-Ins/X.vwlibrary/Contents/MacOS/X";
	CHECK_EQ(MacScriptPathFromBinary(bin),
		"/Contents/MacOS/weird/Plug-Ins/X.vwlibrary/Contents/Resources/vw-update.sh");
}

TEST(mac_script_path_from_binary_no_marker_is_empty)
{
	CHECK_EQ(MacScriptPathFromBinary("/some/other/path/binary"), "");
	CHECK_EQ(MacScriptPathFromBinary(""), "");
}

// ---------------------------------------------------------------------------
// MacPluginsDirFromBinary
// ---------------------------------------------------------------------------

TEST(mac_plugins_dir_from_binary_typical)
{
	const std::string bin =
		"/Users/me/Vectorworks/Plug-Ins/SamplePlugin.vwlibrary/Contents/MacOS/SamplePlugin";
	CHECK_EQ(MacPluginsDirFromBinary(bin), "/Users/me/Vectorworks/Plug-Ins");
}

TEST(mac_plugins_dir_from_binary_no_marker_is_empty)
{
	CHECK_EQ(MacPluginsDirFromBinary("/no/marker/here"), "");
	CHECK_EQ(MacPluginsDirFromBinary(""), "");
}

TEST(mac_plugins_dir_from_binary_bundle_at_root)
{
	// Bundle directly under root: stripping to the containing dir yields "".
	const std::string bin = "/X.vwlibrary/Contents/MacOS/X";
	CHECK_EQ(MacPluginsDirFromBinary(bin), "");
}

// ---------------------------------------------------------------------------
// WinModuleDirFromPath
// ---------------------------------------------------------------------------

TEST(win_module_dir_from_path_backslashes)
{
	CHECK_EQ(WinModuleDirFromPath("C:\\Users\\me\\Plug-Ins\\SamplePlugin.vlb"),
			 "C:\\Users\\me\\Plug-Ins");
}

TEST(win_module_dir_from_path_forward_slashes)
{
	CHECK_EQ(WinModuleDirFromPath("C:/Users/me/Plug-Ins/SamplePlugin.vlb"),
			 "C:/Users/me/Plug-Ins");
}

TEST(win_module_dir_from_path_no_separator_is_empty)
{
	CHECK_EQ(WinModuleDirFromPath("SamplePlugin.vlb"), "");
	CHECK_EQ(WinModuleDirFromPath(""), "");
}

// ---------------------------------------------------------------------------
// WinScriptPathFromDir
// ---------------------------------------------------------------------------

TEST(win_script_path_from_dir_appends_script)
{
	CHECK_EQ(WinScriptPathFromDir("C:\\Users\\me\\Plug-Ins"),
			 "C:\\Users\\me\\Plug-Ins\\vw-update.ps1");
}

TEST(win_script_path_from_dir_empty_is_empty)
{
	CHECK_EQ(WinScriptPathFromDir(""), "");
}

// ---------------------------------------------------------------------------

TEST_MAIN();

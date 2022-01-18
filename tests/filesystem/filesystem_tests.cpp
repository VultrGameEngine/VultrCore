#include <gtest/gtest.h>

#include <filesystem/path.h>
#include "filesystem/filestream.h"

using namespace Vultr;
TEST(Filesystem, CreatePath)
{
	Path p("./src");
	ASSERT_TRUE(p.m_path == "./src");

	ASSERT_TRUE(exists(p));
	ASSERT_TRUE(p.is_directory().value());
	ASSERT_FALSE(p.is_file().value());

	p = Path("/src");
	ASSERT_TRUE(p.m_path == "/src");
	ASSERT_FALSE(exists(p));
}

TEST(Filesystem, FileInputStream)
{
	Path p("./src/renderer/shaders/basic.frag");
	ASSERT_TRUE(exists(p));
	ASSERT_TRUE(p.is_file());

	String buf;
	ASSERT_TRUE(fread_all(p, &buf).has_value());

	printf("Read shader file: %s", buf.c_str());
}
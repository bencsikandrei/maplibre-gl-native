#include <mbgl/test/util.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/style/conversion_impl.hpp>
#include <mbgl/util/rapidjson.hpp>
#include <mbgl/style/rapidjson_conversion.hpp>
#include <mbgl/style/expression/is_expression.hpp>

#include <rapidjson/document.h>

#include <iostream>
#include <fstream>

#if defined(_MSC_VER) && !defined(__clang__)
#include <Windows.h>
#ifdef GetObject
#undef GetObject
#endif
#else
#include <dirent.h>
#endif


using namespace mbgl;
using namespace mbgl::style;

TEST(Expression, IsExpression) {
    rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator> spec;
    spec.Parse<0>(util::read_file("scripts/style-spec-reference/v8.json").c_str());
    ASSERT_FALSE(spec.HasParseError());
    ASSERT_TRUE(spec.IsObject() &&
                spec.HasMember("expression_name") &&
                spec["expression_name"].IsObject() &&
                spec["expression_name"].HasMember("values") &&
                spec["expression_name"]["values"].IsObject());

    const auto& allExpressions = spec["expression_name"]["values"];

    for(auto& entry : allExpressions.GetObject()) {
        const std::string name { entry.name.GetString(), entry.name.GetStringLength() };
        JSDocument document;
        document.Parse<0>(R"([")" + name + R"("])");
        const JSValue* expression = &document;

        // TODO: "interpolate-hcl": https://github.com/mapbox/mapbox-gl-native/issues/8720
        // TODO: "interpolate-lab": https://github.com/mapbox/mapbox-gl-native/issues/8720
        if (name == "interpolate-hcl" || name == "interpolate-lab") {
            if (expression::isExpression(conversion::Convertible(expression))) {
                ASSERT_TRUE(false) << "Expression name" << name << "is implemented - please update Expression.IsExpression test.";
            }
            continue;
        }

        EXPECT_TRUE(expression::isExpression(conversion::Convertible(expression))) << name;
    }
}

class ExpressionEqualityTest : public ::testing::TestWithParam<std::string> {};

TEST_P(ExpressionEqualityTest, ExpressionEquality) {
    const std::string base = std::string("test/fixtures/expression_equality/") + GetParam();

    std::string error;
    auto parse = [&](std::string filename, std::string& error_) -> std::unique_ptr<expression::Expression> {
        rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::CrtAllocator> document;
        document.Parse<0>(util::read_file(filename).c_str());
        assert(!document.HasParseError());
        const JSValue* expression = &document;
        expression::ParsingContext ctx;
        expression::ParseResult parsed = ctx.parseExpression(conversion::Convertible(expression));
        if (!parsed) {
            error_ = ctx.getErrors().size() > 0 ? ctx.getErrors()[0].message : "failed to parse";
        };
        return std::move(*parsed);
    };

    std::unique_ptr<expression::Expression> expression_a1 = parse(base + ".a.json", error);
    ASSERT_TRUE(expression_a1) << GetParam() << ": " << error;

    std::unique_ptr<expression::Expression> expression_a2 = parse(base + ".a.json", error);
    ASSERT_TRUE(expression_a2) << GetParam() << ": " << error;

    std::unique_ptr<expression::Expression> expression_b = parse(base + ".b.json", error);
    ASSERT_TRUE(expression_b) << GetParam() << ": " << error;


    EXPECT_TRUE(*expression_a1 == *expression_a2);
    EXPECT_TRUE(*expression_a1 != *expression_b);
}

static void populateNames(std::vector<std::string>& names) {
    const std::string ending = ".a.json";

    std::string style_directory = "test/fixtures/expression_equality";

    auto testName = [&](const std::string& name) {
        if (name.length() >= ending.length() &&
            name.compare(name.length() - ending.length(), ending.length(), ending) == 0) {
            names.push_back(name.substr(0, name.length() - ending.length()));
        }
    };

#if defined(_MSC_VER) && !defined(__clang__)
    style_directory += "/*";
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(style_directory.c_str(), &ffd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            const std::string name = ffd.cFileName;
            testName(name);
        } while (FindNextFileA(hFind, &ffd) != 0);
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(style_directory.c_str());
    if (dir != nullptr) {
        for (dirent* dp = nullptr; (dp = readdir(dir)) != nullptr;) {
            const std::string name = dp->d_name;
#if ANDROID
            // Android unit test uses number-format stub implementation so skip the tests
            if (name.find("number-format") != std::string::npos) {
                continue;
            }
#endif
            testName(name);
        }
        closedir(dir);
    }
#endif
}

INSTANTIATE_TEST_SUITE_P(Expression, ExpressionEqualityTest, ::testing::ValuesIn([] {
                             std::vector<std::string> names;
                             populateNames(names);
                             EXPECT_GT(names.size(), 0u);
                             return names;
                         }()));

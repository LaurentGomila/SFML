#include <SFML/Graphics/Shader.hpp>

// Other 1st party headers
#include <SFML/System/Exception.hpp>
#include <SFML/System/FileInputStream.hpp>

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

namespace
{
constexpr auto vertexSource = R"(
uniform vec2 storm_position;
uniform float storm_total_radius;
uniform float storm_inner_radius;

void main()
{
    vec4 vertex = gl_ModelViewMatrix * gl_Vertex;
    vec2 offset = vertex.xy - storm_position;
    float len = length(offset);
    if (len < storm_total_radius)
    {
        float push_distance = storm_inner_radius + len / storm_total_radius * (storm_total_radius - storm_inner_radius);
        vertex.xy = storm_position + normalize(offset) * push_distance;
    }

    gl_Position = gl_ProjectionMatrix * vertex;
    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
    gl_FrontColor = gl_Color;
}
)";

constexpr auto geometrySource = R"(
#version 150

// The render target's resolution (used for scaling)
uniform vec2 resolution;

// The billboards' size
uniform vec2 size;

// Input is the passed point cloud
layout (points) in;

// The output will consist of triangle strips with four vertices each
layout (triangle_strip, max_vertices = 4) out;

// Output texture coordinates
out vec2 tex_coord;

// Main entry point
void main()
{
    // Calculate the half width/height of the billboards
    vec2 half_size = size / 2.f;

    // Scale the size based on resolution (1 would be full width/height)
    half_size /= resolution;

    // Iterate over all vertices
    for (int i = 0; i < gl_in.length(); ++i)
    {
        // Retrieve the passed vertex position
        vec2 pos = gl_in[i].gl_Position.xy;

        // Bottom left vertex
        gl_Position = vec4(pos - half_size, 0.f, 1.f);
        tex_coord = vec2(1.f, 1.f);
        EmitVertex();

        // Bottom right vertex
        gl_Position = vec4(pos.x + half_size.x, pos.y - half_size.y, 0.f, 1.f);
        tex_coord = vec2(0.f, 1.f);
        EmitVertex();

        // Top left vertex
        gl_Position = vec4(pos.x - half_size.x, pos.y + half_size.y, 0.f, 1.f);
        tex_coord = vec2(1.f, 0.f);
        EmitVertex();

        // Top right vertex
        gl_Position = vec4(pos + half_size, 0.f, 1.f);
        tex_coord = vec2(0.f, 0.f);
        EmitVertex();

        // And finalize the primitive
        EndPrimitive();
    }
}
)";

constexpr auto fragmentSource = R"(
uniform sampler2D texture;
uniform float blink_alpha;

void main()
{
    vec4 pixel = gl_Color;
    pixel.a = blink_alpha;
    gl_FragColor = pixel;
}
)";

#ifdef SFML_RUN_DISPLAY_TESTS
#ifdef SFML_OPENGL_ES
constexpr bool skipShaderDummyTest = false;
constexpr bool skipShaderFullTest  = true;
#else
constexpr bool skipShaderDummyTest = true;
constexpr bool skipShaderFullTest  = false;
#endif
#else
constexpr bool skipShaderDummyTest = true;
constexpr bool skipShaderFullTest  = true;
#endif

std::string skipShaderDummyTests()
{
    if constexpr (skipShaderDummyTest)
        // https://github.com/catchorg/Catch2/blob/devel/docs/test-cases-and-sections.md#special-tags
        // This tag tells Catch2 to not run a given TEST_CASE
        return "[.shaderDummy]";
    else
        return "";
}

std::string skipShaderFullTests()
{
    if constexpr (skipShaderFullTest)
        // https://github.com/catchorg/Catch2/blob/devel/docs/test-cases-and-sections.md#special-tags
        // This tag tells Catch2 to not run a given TEST_CASE
        return "[.shaderFull]";
    else
        return "";
}

} // namespace

TEST_CASE("[Graphics] sf::Shader (Dummy Implementation)", skipShaderDummyTests())
{
    SECTION("Available")
    {
        CHECK_FALSE(sf::Shader::isAvailable());
        CHECK_FALSE(sf::Shader::isGeometryAvailable());
    }

    SECTION("Construct from memory")
    {
        CHECK_THROWS_AS(sf::Shader(std::string_view(vertexSource), sf::Shader::Type::Vertex), sf::Exception);
        CHECK_THROWS_AS(sf::Shader(std::string_view(geometrySource), sf::Shader::Type::Geometry), sf::Exception);
        CHECK_THROWS_AS(sf::Shader(std::string_view(fragmentSource), sf::Shader::Type::Fragment), sf::Exception);
        CHECK_THROWS_AS(sf::Shader(std::string_view(vertexSource), std::string_view(fragmentSource)), sf::Exception);
        CHECK_THROWS_AS(sf::Shader(std::string_view(vertexSource),
                                   std::string_view(geometrySource),
                                   std::string_view(fragmentSource)),
                        sf::Exception);
    }

    SECTION("loadFromMemory()")
    {
        sf::Shader shader;
        CHECK_FALSE(shader.loadFromMemory(vertexSource, sf::Shader::Type::Vertex));
        CHECK_FALSE(shader.loadFromMemory(geometrySource, sf::Shader::Type::Geometry));
        CHECK_FALSE(shader.loadFromMemory(fragmentSource, sf::Shader::Type::Fragment));
        CHECK_FALSE(shader.loadFromMemory(vertexSource, fragmentSource));
        CHECK_FALSE(shader.loadFromMemory(vertexSource, geometrySource, fragmentSource));
    }
}

TEST_CASE("[Graphics] sf::Shader", skipShaderFullTests())
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_copy_constructible_v<sf::Shader>);
        STATIC_CHECK(!std::is_copy_assignable_v<sf::Shader>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::Shader>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::Shader>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::Shader shader;
            CHECK(shader.getNativeHandle() == 0);
        }

        SECTION("File")
        {
            SECTION("One shader")
            {
                CHECK_THROWS_AS(sf::Shader(std::filesystem::path("does-not-exist.vert"), sf::Shader::Type::Vertex),
                                sf::Exception);

                if (sf::Shader::isAvailable())
                {
                    CHECK(sf::Shader(std::filesystem::path("Graphics/shader.vert"), sf::Shader::Type::Vertex).getNativeHandle() !=
                          0);
                    CHECK(sf::Shader(std::filesystem::path("Graphics/shader.frag"), sf::Shader::Type::Fragment)
                              .getNativeHandle() != 0);
                }
                else
                {
                    CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.vert"), sf::Shader::Type::Vertex),
                                    sf::Exception);
                    CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.frag"), sf::Shader::Type::Fragment),
                                    sf::Exception);
                }
            }

            SECTION("Two shaders")
            {
                CHECK_THROWS_AS(sf::Shader(std::filesystem::path("does-not-exist.vert"),
                                           std::filesystem::path("Graphics/shader.frag")),
                                sf::Exception);
                CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                           std::filesystem::path("does-not-exist.frag")),
                                sf::Exception);

                if (sf::Shader::isAvailable())
                {
                    CHECK(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                     std::filesystem::path("Graphics/shader.frag"))
                              .getNativeHandle() != 0);
                }
                else
                {
                    CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                               std::filesystem::path("Graphics/shader.frag")),
                                    sf::Exception);
                }
            }

            SECTION("Three shaders")
            {
                CHECK_THROWS_AS(sf::Shader(std::filesystem::path("does-not-exist.vert"),
                                           std::filesystem::path("Graphics/shader.geom"),
                                           std::filesystem::path("Graphics/shader.frag")),
                                sf::Exception);
                CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                           std::filesystem::path("does-not-exist.geom"),
                                           std::filesystem::path("Graphics/shader.frag")),
                                sf::Exception);
                CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                           std::filesystem::path("Graphics/shader.geom"),
                                           std::filesystem::path("does-not-exist.frag")),
                                sf::Exception);

                if (sf::Shader::isGeometryAvailable())
                {
                    CHECK(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                     std::filesystem::path("Graphics/shader.geom"),
                                     std::filesystem::path("Graphics/shader.frag"))
                              .getNativeHandle() != 0);
                }
                else
                {
                    CHECK_THROWS_AS(sf::Shader(std::filesystem::path("Graphics/shader.vert"),
                                               std::filesystem::path("Graphics/shader.geom"),
                                               std::filesystem::path("Graphics/shader.frag")),
                                    sf::Exception);
                }
            }
        }

        SECTION("Memory")
        {
            if (sf::Shader::isAvailable())
            {
                CHECK(sf::Shader(std::string_view(vertexSource), sf::Shader::Type::Vertex).getNativeHandle() != 0);
                CHECK_THROWS_AS(sf::Shader(std::string_view(geometrySource), sf::Shader::Type::Geometry), sf::Exception);
                CHECK(sf::Shader(std::string_view(fragmentSource), sf::Shader::Type::Fragment).getNativeHandle() != 0);
                CHECK(sf::Shader(std::string_view(vertexSource), std::string_view(fragmentSource)).getNativeHandle() != 0);
            }
            else
            {
                CHECK_THROWS_AS(sf::Shader(std::string_view(vertexSource), sf::Shader::Type::Vertex), sf::Exception);
                CHECK_THROWS_AS(sf::Shader(std::string_view(geometrySource), sf::Shader::Type::Geometry), sf::Exception);
                CHECK_THROWS_AS(sf::Shader(std::string_view(fragmentSource), sf::Shader::Type::Fragment), sf::Exception);
                CHECK_THROWS_AS(sf::Shader(std::string_view(vertexSource), std::string_view(fragmentSource)), sf::Exception);
            }

            if (sf::Shader::isGeometryAvailable())
            {
                CHECK(sf::Shader(std::string_view(vertexSource), std::string_view(geometrySource), std::string_view(fragmentSource))
                          .getNativeHandle() != 0);
            }
            else
            {
                CHECK_THROWS_AS(sf::Shader(std::string_view(vertexSource),
                                           std::string_view(geometrySource),
                                           std::string_view(fragmentSource)),
                                sf::Exception);
            }
        }

        SECTION("Stream")
        {
            sf::FileInputStream vertexShaderStream("Graphics/shader.vert");
            sf::FileInputStream fragmentShaderStream("Graphics/shader.frag");
            sf::FileInputStream geometryShaderStream("Graphics/shader.geom");

            sf::FileInputStream emptyStream("Graphics/invalid_shader.vert");

            SECTION("One shader")
            {
                CHECK_THROWS_AS(sf::Shader(emptyStream, sf::Shader::Type::Vertex), sf::Exception);

                if (sf::Shader::isAvailable())
                {
                    CHECK(sf::Shader(vertexShaderStream, sf::Shader::Type::Vertex).getNativeHandle() != 0);
                    CHECK(sf::Shader(fragmentShaderStream, sf::Shader::Type::Fragment).getNativeHandle() != 0);
                }
                else
                {
                    CHECK_THROWS_AS(sf::Shader(vertexShaderStream, sf::Shader::Type::Vertex), sf::Exception);
                    CHECK_THROWS_AS(sf::Shader(fragmentShaderStream, sf::Shader::Type::Fragment), sf::Exception);
                }
            }

            SECTION("Two shaders")
            {
                CHECK_THROWS_AS(sf::Shader(emptyStream, fragmentShaderStream), sf::Exception);
                CHECK_THROWS_AS(sf::Shader(vertexShaderStream, emptyStream), sf::Exception);

                if (sf::Shader::isAvailable())
                {
                    CHECK(sf::Shader(vertexShaderStream, fragmentShaderStream).getNativeHandle() != 0);
                }
                else
                {
                    CHECK_THROWS_AS(sf::Shader(vertexShaderStream, fragmentShaderStream), sf::Exception);
                }
            }

            SECTION("Three shaders")
            {
                CHECK_THROWS_AS(sf::Shader(emptyStream, geometryShaderStream, fragmentShaderStream), sf::Exception);
                CHECK_THROWS_AS(sf::Shader(vertexShaderStream, emptyStream, fragmentShaderStream), sf::Exception);
                CHECK_THROWS_AS(sf::Shader(vertexShaderStream, geometryShaderStream, emptyStream), sf::Exception);

                if (sf::Shader::isGeometryAvailable())
                {
                    CHECK(sf::Shader(vertexShaderStream, geometryShaderStream, fragmentShaderStream).getNativeHandle() != 0);
                }
                else
                {
                    CHECK_THROWS_AS(sf::Shader(vertexShaderStream, geometryShaderStream, fragmentShaderStream),
                                    sf::Exception);
                }
            }
        }
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            sf::Shader       movedShader;
            const sf::Shader shader = std::move(movedShader);
            CHECK(shader.getNativeHandle() == 0);
        }

        SECTION("Assignment")
        {
            sf::Shader movedShader;
            sf::Shader shader;
            shader = std::move(movedShader);
            CHECK(shader.getNativeHandle() == 0);
        }
    }

    SECTION("Move semantics (create)")
    {
        SECTION("Construction")
        {
            sf::Shader       movedShader(std::filesystem::path("Graphics/shader.vert"), sf::Shader::Type::Vertex);
            const sf::Shader shader = std::move(movedShader);
            CHECK(shader.getNativeHandle() != 0);
        }

        SECTION("Assignment")
        {
            sf::Shader movedShader(std::filesystem::path("Graphics/shader.vert"), sf::Shader::Type::Vertex);
            sf::Shader shader(std::filesystem::path("Graphics/shader.frag"), sf::Shader::Type::Fragment);
            shader = std::move(movedShader);
            CHECK(shader.getNativeHandle() != 0);
        }
    }

    SECTION("loadFromFile()")
    {
        sf::Shader shader;

        SECTION("One shader")
        {
            CHECK(!shader.loadFromFile("does-not-exist.vert", sf::Shader::Type::Vertex));

            CHECK(shader.loadFromFile("Graphics/shader.vert", sf::Shader::Type::Vertex) == sf::Shader::isAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isAvailable());

            CHECK(shader.loadFromFile("Graphics/shader.frag", sf::Shader::Type::Fragment) == sf::Shader::isAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isAvailable());
        }

        SECTION("Two shaders")
        {
            CHECK(!shader.loadFromFile("does-not-exist.vert", "Graphics/shader.frag"));
            CHECK(!shader.loadFromFile("Graphics/shader.vert", "does-not-exist.frag"));
            CHECK(shader.loadFromFile("Graphics/shader.vert", "Graphics/shader.frag") == sf::Shader::isAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isAvailable());
        }

        SECTION("Three shaders")
        {
            CHECK(!shader.loadFromFile("does-not-exist.vert", "Graphics/shader.geom", "Graphics/shader.frag"));
            CHECK(!shader.loadFromFile("Graphics/shader.vert", "does-not-exist.geom", "Graphics/shader.frag"));
            CHECK(!shader.loadFromFile("Graphics/shader.vert", "Graphics/shader.geom", "does-not-exist.frag"));
            CHECK(shader.loadFromFile("Graphics/shader.vert", "Graphics/shader.geom", "Graphics/shader.frag") ==
                  sf::Shader::isGeometryAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isGeometryAvailable());
        }

        SECTION("One shader with non-ASCII filename")
        {
            CHECK(shader.loadFromFile(U"Graphics/shader-ń.vert", sf::Shader::Type::Vertex) == sf::Shader::isAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isAvailable());

            CHECK(shader.loadFromFile(U"Graphics/shader-🐌.vert", sf::Shader::Type::Vertex) == sf::Shader::isAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isAvailable());
        }
    }

    SECTION("loadFromMemory()")
    {
        sf::Shader shader;
        CHECK(shader.loadFromMemory(vertexSource, sf::Shader::Type::Vertex) == sf::Shader::isAvailable());
        CHECK(!shader.loadFromMemory(geometrySource, sf::Shader::Type::Geometry));
        CHECK(shader.loadFromMemory(fragmentSource, sf::Shader::Type::Fragment) == sf::Shader::isAvailable());
        CHECK(shader.loadFromMemory(vertexSource, fragmentSource) == sf::Shader::isAvailable());
        CHECK(shader.loadFromMemory(vertexSource, geometrySource, fragmentSource) == sf::Shader::isGeometryAvailable());
        CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isAvailable());
    }

    SECTION("loadFromStream()")
    {
        sf::Shader          shader;
        sf::FileInputStream vertexShaderStream;
        REQUIRE(vertexShaderStream.open("Graphics/shader.vert"));

        sf::FileInputStream fragmentShaderStream;
        REQUIRE(fragmentShaderStream.open("Graphics/shader.frag"));

        sf::FileInputStream geometryShaderStream;
        REQUIRE(geometryShaderStream.open("Graphics/shader.geom"));

        sf::FileInputStream emptyStream;

        SECTION("One shader")
        {
            REQUIRE(!shader.loadFromStream(emptyStream, sf::Shader::Type::Vertex));
            REQUIRE(shader.loadFromStream(vertexShaderStream, sf::Shader::Type::Vertex) == sf::Shader::isAvailable());
            REQUIRE(shader.loadFromStream(fragmentShaderStream, sf::Shader::Type::Fragment) == sf::Shader::isAvailable());
        }

        SECTION("Two shaders")
        {
            REQUIRE(!shader.loadFromStream(emptyStream, fragmentShaderStream));
            REQUIRE(!shader.loadFromStream(vertexShaderStream, emptyStream));
            REQUIRE(shader.loadFromStream(vertexShaderStream, fragmentShaderStream) == sf::Shader::isAvailable());
        }

        SECTION("Three shaders")
        {
            REQUIRE(!shader.loadFromStream(emptyStream, geometryShaderStream, fragmentShaderStream));
            REQUIRE(!shader.loadFromStream(vertexShaderStream, emptyStream, fragmentShaderStream));
            REQUIRE(!shader.loadFromStream(vertexShaderStream, geometryShaderStream, emptyStream));
            REQUIRE(shader.loadFromStream(vertexShaderStream, geometryShaderStream, fragmentShaderStream) ==
                    sf::Shader::isGeometryAvailable());
            CHECK(static_cast<bool>(shader.getNativeHandle()) == sf::Shader::isGeometryAvailable());
        }
    }
}

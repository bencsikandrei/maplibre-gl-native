#pragma once

#include <mbgl/programs/program.hpp>
#include <mbgl/programs/attributes.hpp>
#include <mbgl/programs/uniforms.hpp>
#include <mbgl/programs/textures.hpp>
#include <mbgl/style/properties.hpp>
#include <mbgl/util/geometry.hpp>

namespace mbgl {

class HeatmapTextureProgram final : public Program<
    HeatmapTextureProgram,
    shaders::BuiltIn::HeatmapTextureProgram,
    gfx::PrimitiveType::Triangle,
    TypeList<attributes::pos>,
    TypeList<
        uniforms::matrix,
        uniforms::world,
        uniforms::opacity>,
    TypeList<
        textures::image,
        textures::color_ramp>,
    style::Properties<>> {
public:
    static constexpr std::string_view Name{"HeatmapTextureProgram"};
    const std::string_view typeName() const noexcept override {
        return Name;
    }

    using Program::Program;

    static LayoutVertex layoutVertex(Point<int16_t> p) {
        return LayoutVertex{
            {{
                p.x,
                p.y
            }}
        };
    }
};

using HeatmapTextureLayoutVertex = HeatmapTextureProgram::LayoutVertex;
using HeatmapTextureAttributes = HeatmapTextureProgram::AttributeList;

} // namespace mbgl

#include <mbgl/style/conversion/layer.hpp>
#include <mbgl/style/conversion/constant.hpp>
#include <mbgl/style/conversion/filter.hpp>
#include <mbgl/style/conversion_impl.hpp>

#include <mbgl/layermanager/layer_manager.hpp>

namespace mbgl {
namespace style {
namespace conversion {

namespace {
bool setObjectMember(std::unique_ptr<Layer>& layer, const Convertible& value, const char* member, Error& error) {
    if (auto memberValue = objectMember(value, member)) {
        if (auto error_ = layer->setProperty(member, *memberValue)) {
            error = *error_;
            return false;
        }
    }
    return true;
}
} // namespace

std::optional<Error> setPaintProperties(Layer& layer, const Convertible& value) {
    auto paintValue = objectMember(value, "paint");
    if (!paintValue) {
        return std::nullopt;
    }
    if (!isObject(*paintValue)) {
        return { { "paint must be an object" } };
    }
    return eachMember(*paintValue, [&](const std::string& k, const Convertible& v) { return layer.setProperty(k, v); });
}

std::optional<std::unique_ptr<Layer>> Converter<std::unique_ptr<Layer>>::operator()(const Convertible& value, Error& error) const {
    if (!isObject(value)) {
        error.message = "layer must be an object";
        return std::nullopt;
    }

    auto idValue = objectMember(value, "id");
    if (!idValue) {
        error.message = "layer must have an id";
        return std::nullopt;
    }

    std::optional<std::string> id = toString(*idValue);
    if (!id) {
        error.message = "layer id must be a string";
        return std::nullopt;
    }

    auto typeValue = objectMember(value, "type");
    if (!typeValue) {
        error.message = "layer must have a type";
        return std::nullopt;
    }

    auto type = toString(*typeValue);
    if (!type) {
        error.message = "layer type must be a string";
        return std::nullopt;
    }

    std::unique_ptr<Layer> layer = LayerManager::get()->createLayer(*type, *id, value, error);
    if (!layer) return std::nullopt;

    if (!setObjectMember(layer, value, "minzoom", error)) return std::nullopt;
    if (!setObjectMember(layer, value, "maxzoom", error)) return std::nullopt;
    if (!setObjectMember(layer, value, "filter", error)) return std::nullopt;
    if (layer->getTypeInfo()->source == LayerTypeInfo::Source::Required) {
        if (!setObjectMember(layer, value, "source-layer", error)) return std::nullopt;
    }

    auto layoutValue = objectMember(value, "layout");
    if (layoutValue) {
        if (!isObject(*layoutValue)) {
            error.message = "layout must be an object";
            return std::nullopt;
        }
        auto error_ = eachMember(
            *layoutValue, [&](const std::string& k, const Convertible& v) { return layer->setProperty(k, v); });
        if (error_) {
            error = *error_;
            return std::nullopt;
        }
    }

    auto error_ = setPaintProperties(*layer, value);
    if (error_) {
        error = *error_;
        return std::nullopt;
    }

    return layer;
}

} // namespace conversion
} // namespace style
} // namespace mbgl

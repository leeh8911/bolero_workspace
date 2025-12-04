#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include "bolero/config.hpp"

namespace bolero {

// Tag 로 서로 다른 Factory 를 분리하기 위한 템플릿
template <typename Tag, typename Base>
class ClassFactory {
   public:
    using BaseType = Base;
    using Creator = std::function<std::unique_ptr<BaseType>(const Config&)>;

    static void Register(const std::string& key, Creator creator) {
        // 같은 key 가 다시 등록되면 덮어쓰기 (원하면 여기서 예외를 던지도록 바꿀 수 있음)
        Registry()[key] = std::move(creator);
    }

    static std::unique_ptr<BaseType> Create(const Config& config) {
        std::string type_name;
        try {
            // "type" 키가 없거나 string 이 아니면 여기서 예외 발생
            type_name = std::string(config["type"]);
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string{"ClassFactory: missing or invalid 'type' in config: "} +
                                     e.what());
        }

        auto& registry = Registry();
        auto it = registry.find(type_name);
        if (it == registry.end()) {
            throw std::runtime_error("ClassFactory: type is not registered: " + type_name);
        }

        return it->second(config);
    }

   private:
    static std::unordered_map<std::string, Creator>& Registry() {
        static std::unordered_map<std::string, Creator> instance;
        return instance;
    }
};

}  // namespace bolero

// --------------------
// Public macros
// --------------------

// NAME:     BASIC_FACTORY 같은 팩토리 이름
// BASE_TYPE: BaseClass
#define BUILD_FACTORY(NAME, BASE_TYPE)                                                                  \
    struct NAME##_FactoryTag {};                                                                        \
    struct NAME {                                                                                       \
        using BaseType = BASE_TYPE;                                                                     \
        using Config = bolero::Config;                                                                  \
        using Factory = bolero::ClassFactory<NAME##_FactoryTag, BaseType>;                              \
        using Creator = typename Factory::Creator;                                                      \
        static std::unique_ptr<BaseType> Make(const Config& config) { return Factory::Create(config); } \
        static void Register(const std::string& key, Creator creator) {                                 \
            Factory::Register(key, std::move(creator));                                                 \
        }                                                                                               \
    };

// NAME: BASIC_FACTORY
// TYPE: DerivedClassA, DerivedClassB ...
#define REGIST_CLASS(NAME, TYPE)                                                                          \
    namespace {                                                                                           \
    struct AutoRegister_##NAME##_##TYPE {                                                                 \
        AutoRegister_##NAME##_##TYPE() {                                                                  \
            NAME::Register(#TYPE, [](const bolero::Config& cfg) { return std::make_unique<TYPE>(cfg); }); \
        }                                                                                                 \
    };                                                                                                    \
    static AutoRegister_##NAME##_##TYPE s_auto_register_##NAME##_##TYPE;                                  \
    }

// NAME: BASIC_FACTORY
// CONFIG: bolero::Config 객체
#define MAKE_CLASS(NAME, CONFIG) NAME::Make(CONFIG)

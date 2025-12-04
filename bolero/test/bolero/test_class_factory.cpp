#include <gtest/gtest.h>

#include "bolero/class_factory.hpp"
#include "bolero/config.hpp"

class BaseClass {
   public:
    virtual ~BaseClass() = default;
    virtual int32_t Value() const = 0;
};
BUILD_FACTORY(BASIC_FACTORY, BaseClass);

class DerivedClassA : public BaseClass {
   public:
    DerivedClassA(const bolero::Config& config) : value{config["value"]} {}
    int32_t Value() const override { return this->value; }

   private:
    int32_t value;
};
REGIST_CLASS(BASIC_FACTORY, DerivedClassA);

class DerivedClassB : public BaseClass {
   public:
    DerivedClassB(const bolero::Config& config) : value{config["value"]} {}
    int32_t Value() const override { return this->value + 1; }

   private:
    int32_t value;
};
REGIST_CLASS(BASIC_FACTORY, DerivedClassB);

TEST(TestClassFactory, BasicUsage) {
    bolero::Config config_a;
    config_a["type"] = "DerivedClassA";
    config_a["value"] = 42;

    bolero::Config config_b;
    config_b["type"] = "DerivedClassB";
    config_b["value"] = 42;

    auto derived_a = MAKE_CLASS(BASIC_FACTORY, config_a);
    auto derived_b = MAKE_CLASS(BASIC_FACTORY, config_b);

    EXPECT_EQ(derived_a->Value(), 42);
    EXPECT_EQ(derived_b->Value(), 43);
}
TEST(TestClassFactory, NotRegisteredClass) {
    bolero::Config unknown_config;
    unknown_config["type"] = "UnknownClass";

    EXPECT_THROW(MAKE_CLASS(BASIC_FACTORY, unknown_config), std::runtime_error);
}
TEST(TestClassFactory, MissTypeKey) {
    bolero::Config miss_config;

    EXPECT_THROW(MAKE_CLASS(BASIC_FACTORY, miss_config), std::runtime_error);
}
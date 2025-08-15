
#include <cstdint>
#include <string>
#include <algorithm>

namespace metric {

enum class TypeOfGroup : int {
  General = 0,
  CanMessage = 1,
  Device = 2
};

class MetricGroup {
 public:
  void Name(std::string name) { name_ = std::move(name); }
  [[nodiscard]] const std::string& Name() const { return name_; }

  void Description(std::string desc) { description_ = std::move(desc); }
  [[nodiscard]] const std::string& Description() const { return description_; }

  void Type(TypeOfGroup type) { type_ = type; }
  [[nodiscard]] TypeOfGroup Type() const { return type_; }

  void Identity(int64_t identity) { identity_ = identity; }
  [[nodiscard]] int64_t Identity() const { return identity_; }

 private:
  std::string name_;
  std::string description_;
  TypeOfGroup type_ = TypeOfGroup::General;
  int64_t identity_ = 0;

};

}  // namespace metric


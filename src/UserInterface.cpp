#include "UserInterface.hpp"

void UserInterface::SetVariant(const QString& name, const QVariant& value) {
  variant[name] = value;
  emit VariantChanged();
}

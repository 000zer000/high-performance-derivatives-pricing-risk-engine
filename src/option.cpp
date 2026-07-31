#include "derivatives/option.hpp"

#include <stdexcept>

namespace derivatives {

void validate(const EuropeanOption&)
{
    throw std::logic_error("European option validation is not implemented");
}

}  // namespace derivatives

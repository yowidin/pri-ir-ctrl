#ifndef IR_UTIL_H__
#define IR_UTIL_H__

#include <outcome/outcome.hpp>
#include <boost/program_options.hpp>

namespace ir {

namespace outcome = OUTCOME_V2_NAMESPACE;

template <class R,
          class S = std::error_code,
          class NoValuePolicy = outcome::policy::default_policy<R, S, void>>
using result_t = outcome::result<R, S, NoValuePolicy>;

using po_desc_t = boost::program_options::options_description;
using po_vars_t = boost::program_options::variables_map;

} // namespace ir

#endif // IR_UTIL_H__

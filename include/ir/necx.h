/**
 * @file   necx.h
 * @author Dennis Sitelew
 * @date   Nov. 09, 2021
 */
#ifndef INCLUDE_IR_NECX_H
#define INCLUDE_IR_NECX_H

#include <ir/wave.h>

#include <cstdint>

namespace ir {

/**
 * Extended NEC encoder.
 *
 * Protocol details:
 * - https://techdocs.altium.com/display/FPGA/NEC+Infrared+Transmission+Protocol
 * - https://www.sbprojects.net/knowledge/ir/nec.php
 *
 * In short:
 * - Pulse distance encoding
 * - Carrier frequency: 38kHZ
 * - Logical '0' - 562.5µs pulse burst + 562.5µs space
 * - Logical '1' - 562.5µs pulse burst + 1.6875ms space
 * See nec_parameters for details.
 */
class necx : public ir::wave {
public:
   necx(int pin_number, std::uint32_t code);

public:
   std::string name() const override { return "necx"; }

protected:
   void add_payload() override;

private:
   const std::uint32_t code_;
};

} // namespace ir

#endif /* INCLUDE_IR_NECX_H */

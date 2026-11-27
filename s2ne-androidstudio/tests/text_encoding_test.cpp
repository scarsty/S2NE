#include "text_encoding.hpp"
#include <array>
#include <stdexcept>

int main()
{
    const std::array<uint8_t, 4> big5{ 0xa4, 0xa4, 0xa4, 0xe5 };
    const std::vector<uint8_t> utf8{ 0xe4, 0xb8, 0xad, 0xe6, 0x96, 0x87 };
    const auto decoded = s2ne::convert_text(big5, false);
    if (!decoded || *decoded != utf8) { throw std::runtime_error("Big5 decoding failed"); }
    const auto encoded = s2ne::convert_text(utf8, true);
    if (!encoded || *encoded != std::vector<uint8_t>(big5.begin(), big5.end())) { throw std::runtime_error("Big5 round trip failed"); }
    const std::array<uint8_t, 1> invalid{ 0xff };
    if (s2ne::convert_text(invalid, true)) { throw std::runtime_error("Invalid UTF-8 accepted"); }
    if (s2ne::convert_text({}, false) != std::vector<uint8_t>{}) { throw std::runtime_error("Empty conversion failed"); }
    std::vector<uint8_t> output;
    if (!s2ne::utf8_to_big5("ASCII", output) || output != std::vector<uint8_t>{ 'A', 'S', 'C', 'I', 'I' })
    {
        throw std::runtime_error("ASCII encoding failed");
    }
}

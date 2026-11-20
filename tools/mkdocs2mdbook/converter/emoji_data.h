#ifndef __CONVERTER_EMOJI_DATA_H__
#define __CONVERTER_EMOJI_DATA_H__

#include <string_view>

namespace converter {

/**
 * @brief Looks up an emoji shortcode (without colons, e.g. "memo", "+1", "rocket").
 * 
 * @param name The shortcode name.
 * @return const char* Pointer to the null-terminated UTF-8 encoded emoji, or nullptr if not found.
 */
const char* find_emoji(std::string_view name);

} // namespace converter

#endif // __CONVERTER_EMOJI_DATA_H__

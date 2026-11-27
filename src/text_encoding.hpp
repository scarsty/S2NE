#pragma once
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>
#ifdef __ANDROID__
#include <SDL3/SDL_system.h>
#include <jni.h>
#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace s2ne
{
inline std::optional<std::vector<uint8_t>> convert_text(std::span<const uint8_t> input, bool to_big5)
{
    if (input.empty()) { return std::vector<uint8_t>{}; }
    if (input.size() > size_t((std::numeric_limits<int>::max)())) { return std::nullopt; }
#ifdef __ANDROID__
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env || env->PushLocalFrame(32) != JNI_OK) { return std::nullopt; }
    struct Frame
    {
        JNIEnv* env;
        ~Frame() { env->PopLocalFrame(nullptr); }
    } frame{ env };
    auto failed = [&]()
    {
        if (!env->ExceptionCheck()) { return false; }
        env->ExceptionClear();
        return true;
    };
    auto charset = env->FindClass("java/nio/charset/Charset");
    if (failed() || !charset) { return std::nullopt; }
    const auto for_name = env->GetStaticMethodID(charset, "forName", "(Ljava/lang/String;)Ljava/nio/charset/Charset;");
    const auto decoder_method = env->GetMethodID(charset, "newDecoder", "()Ljava/nio/charset/CharsetDecoder;");
    const auto encoder_method = env->GetMethodID(charset, "newEncoder", "()Ljava/nio/charset/CharsetEncoder;");
    if (failed() || !for_name || !decoder_method || !encoder_method) { return std::nullopt; }
    auto from_name = env->NewStringUTF(to_big5 ? "UTF-8" : "Big5");
    auto to_name = env->NewStringUTF(to_big5 ? "Big5" : "UTF-8");
    auto from = env->CallStaticObjectMethod(charset, for_name, from_name);
    auto to = env->CallStaticObjectMethod(charset, for_name, to_name);
    if (failed() || !from || !to) { return std::nullopt; }
    auto decoder = env->CallObjectMethod(from, decoder_method);
    auto encoder = env->CallObjectMethod(to, encoder_method);
    if (failed() || !decoder || !encoder) { return std::nullopt; }
    auto buffer_class = env->FindClass("java/nio/ByteBuffer");
    auto decoder_class = env->FindClass("java/nio/charset/CharsetDecoder");
    auto encoder_class = env->FindClass("java/nio/charset/CharsetEncoder");
    if (failed() || !buffer_class || !decoder_class || !encoder_class) { return std::nullopt; }
    auto wrap = env->GetStaticMethodID(buffer_class, "wrap", "([B)Ljava/nio/ByteBuffer;");
    auto decode = env->GetMethodID(decoder_class, "decode", "(Ljava/nio/ByteBuffer;)Ljava/nio/CharBuffer;");
    auto encode = env->GetMethodID(encoder_class, "encode", "(Ljava/nio/CharBuffer;)Ljava/nio/ByteBuffer;");
    auto remaining = env->GetMethodID(buffer_class, "remaining", "()I");
    auto get = env->GetMethodID(buffer_class, "get", "([B)Ljava/nio/ByteBuffer;");
    if (failed() || !wrap || !decode || !encode || !remaining || !get) { return std::nullopt; }
    auto bytes = env->NewByteArray(jsize(input.size()));
    if (failed() || !bytes) { return std::nullopt; }
    env->SetByteArrayRegion(bytes, 0, jsize(input.size()), reinterpret_cast<const jbyte*>(input.data()));
    auto buffer = env->CallStaticObjectMethod(buffer_class, wrap, bytes);
    if (failed() || !buffer) { return std::nullopt; }
    auto chars = env->CallObjectMethod(decoder, decode, buffer);
    if (failed() || !chars) { return std::nullopt; }
    auto encoded = env->CallObjectMethod(encoder, encode, chars);
    if (failed() || !encoded) { return std::nullopt; }
    const jint count = env->CallIntMethod(encoded, remaining);
    if (failed() || count < 0) { return std::nullopt; }
    auto output = env->NewByteArray(count);
    if (failed() || !output) { return std::nullopt; }
    env->CallObjectMethod(encoded, get, output);
    if (failed()) { return std::nullopt; }
    std::vector<uint8_t> result(size_t(count), 0);
    env->GetByteArrayRegion(output, 0, count, reinterpret_cast<jbyte*>(result.data()));
    if (failed()) { return std::nullopt; }
    return result;
#else
    const unsigned source = to_big5 ? CP_UTF8 : 950;
    const unsigned target = to_big5 ? 950 : CP_UTF8;
    int count = MultiByteToWideChar(source, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(input.data()), int(input.size()), nullptr, 0);
    if (count <= 0) { return std::nullopt; }
    std::wstring wide(size_t(count), L'\0');
    if (!MultiByteToWideChar(source, MB_ERR_INVALID_CHARS, reinterpret_cast<const char*>(input.data()), int(input.size()), wide.data(), count)) { return std::nullopt; }
    const DWORD flags = to_big5 ? WC_NO_BEST_FIT_CHARS : 0;
    int length = WideCharToMultiByte(target, flags, wide.data(), count, nullptr, 0, nullptr, nullptr);
    if (length <= 0) { return std::nullopt; }
    std::vector<uint8_t> result(size_t(length), 0);
    if (!WideCharToMultiByte(target, flags, wide.data(), count, reinterpret_cast<char*>(result.data()), length, nullptr, nullptr)) { return std::nullopt; }
    return result;
#endif
}
inline bool utf8_to_big5(const std::string& input, std::vector<uint8_t>& output)
{
    auto result = convert_text({ reinterpret_cast<const uint8_t*>(input.data()), input.size() }, true);
    if (!result) { return false; }
    output = std::move(*result);
    return true;
}
}    // namespace s2ne

#pragma once
#include "original_save.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace s2ne
{
class OriginalSaveStore
{
public:
    static Result<bool> try_replace(const std::filesystem::path& path, std::span<const uint8_t> bytes);

    // 43619F preserves the overwritten original as slot53 before4362B4 writes.
    static Result<bool> try_write(const std::filesystem::path& root, unsigned slot, const OriginalSave::State& state, size_t items);
    static void write(const std::filesystem::path& root, unsigned slot, const OriginalSave::State& state, size_t items);
};
}    //namespace s2ne

#pragma once
#include "original_save.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace s2ne
{
class OriginalSaveStore
{
    static Result<bool> replace(const std::filesystem::path& path, std::span<const uint8_t> bytes)
    {
        auto pending = path;
        pending += L".s2ne-tmp";
        HANDLE file = CreateFileW(pending.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return Result<bool>::failure(make_error("Cannot create exclusive original-save temporary file"));
        }
        struct PendingFile
        {
            HANDLE& file;
            const std::filesystem::path& path;
            ~PendingFile()
            {
                if (file != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(file);
                }
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
        } cleanup{ file, pending };
        size_t offset = 0;
        while (offset < bytes.size())
        {
            DWORD written = 0, want = DWORD(std::min<size_t>(bytes.size() - offset, 0x40000000));
            if (!WriteFile(file, bytes.data() + offset, want, &written, nullptr) || !written)
            {
                return Result<bool>::failure(make_error("Original-save write failed"));
            }
            offset += written;
        }
        if (!FlushFileBuffers(file))
        {
            return Result<bool>::failure(make_error("Original-save flush failed"));
        }
        CloseHandle(file);
        file = INVALID_HANDLE_VALUE;
        std::error_code error;
        bool exists = std::filesystem::exists(path, error);
        if (error)
        {
            return Result<bool>::failure(make_error("Cannot inspect original-save destination: " + error.message()));
        }
        bool success;
        if (exists)
        {
            auto backup = path;
            backup += L".s2ne-backup";
            success = ReplaceFileW(path.c_str(), pending.c_str(), backup.c_str(), 0, nullptr, nullptr) != 0;
        }
        else
        {
            success = MoveFileExW(pending.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
        }
        if (!success)
        {
            return Result<bool>::failure(make_error("Original-save atomic replacement failed"));
        }
        return Result<bool>::success(true);
    }

public:
    // 43619F preserves the overwritten original as slot53 before4362B4 writes.
    static Result<bool> try_write(const std::filesystem::path& root, unsigned slot, const OriginalSave::State& state, size_t items)
    {
        if (slot < 1 || slot > 53)
        {
            return Result<bool>::failure(make_error("Invalid original save slot"));
        }
        auto encoded = OriginalSave::try_encode(state, items);
        if (!encoded)
        {
            return Result<bool>::failure(encoded.error());
        }
        auto path = OriginalSave::slot_path(root, slot);
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error)
        {
            return Result<bool>::failure(make_error("Cannot create save directory: " + error.message()));
        }
        auto pending = path;
        pending += L".s2ne-tmp";
        bool pending_exists = std::filesystem::exists(pending, error);
        if (error || pending_exists)
        {
            return Result<bool>::failure(make_error("Previous pending save exists or cannot be inspected; original slots left untouched"));
        }
        bool exists = std::filesystem::exists(path, error);
        if (error)
        {
            return Result<bool>::failure(make_error("Cannot inspect save slot: " + error.message()));
        }
        if (slot != 53 && exists)
        {
            auto previous = OriginalSave::try_load(path, items);
            if (!previous)
            {
                return Result<bool>::failure(previous.error());
            }
            if (previous.value().description.front() != 0)
            {
                std::ifstream input(path, std::ios::binary);
                std::vector<uint8_t> previous_bytes{ std::istreambuf_iterator<char>(input), { } };
                if (!input || input.bad())
                {
                    return Result<bool>::failure(make_error("Cannot read save backup source"));
                }
                auto backup = replace(OriginalSave::slot_path(root, 53), previous_bytes);
                if (!backup)
                {
                    return backup;
                }
            }
        }
        return replace(path, encoded.value());
    }
    static void write(const std::filesystem::path& root, unsigned slot, const OriginalSave::State& state, size_t items)
    {
        auto result = try_write(root, slot, state, items);
        if (!result)
        {
            fatal(result.error());
        }
    }
};
}    //namespace s2ne

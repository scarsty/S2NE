#include "save_store.h"

namespace s2ne
{
auto SaveStore::try_replace(const std::filesystem::path& path, std::span<const uint8_t> bytes) -> Result<bool>
{
#ifdef _WIN32
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
#else
    const auto failure = [](const char* message)
    {
        const std::error_code error(errno, std::generic_category());
        return Result<bool>::failure(make_error(std::string(message) + ": " + error.message()));
    };
    auto pending = path;
    pending += ".s2ne-tmp";
    int file = ::open(pending.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
    if (file < 0) { return failure("Cannot create exclusive original-save temporary file"); }
    struct PendingFile
    {
        int& file;
        const std::filesystem::path& path;
        ~PendingFile()
        {
            if (file >= 0) { ::close(file); }
            ::unlink(path.c_str());
        }
    } cleanup{ file, pending };
    size_t offset = 0;
    while (offset < bytes.size())
    {
        const auto written = ::write(file, bytes.data() + offset, std::min<size_t>(bytes.size() - offset, 0x40000000));
        if (written < 0 && errno == EINTR) { continue; }
        if (written < 0) { return failure("Original-save write failed"); }
        if (written == 0) { return Result<bool>::failure(make_error("Original-save write made no progress")); }
        offset += size_t(written);
    }
    int flushed;
    do { flushed = ::fsync(file); } while (flushed < 0 && errno == EINTR);
    if (flushed < 0) { return failure("Original-save flush failed"); }
    const int closed = ::close(file);
    file = -1;
    if (closed < 0) { return failure("Original-save close failed"); }
    auto directory = path.parent_path();
    if (directory.empty()) { directory = "."; }
    int directory_file = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (directory_file < 0) { return failure("Cannot open original-save directory for flush"); }
    struct DirectoryFile
    {
        int file;
        ~DirectoryFile() { ::close(file); }
    } directory_cleanup{ directory_file };
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) { return Result<bool>::failure(make_error("Cannot inspect original-save destination: " + error.message())); }
    if (exists)
    {
        auto backup = path;
        backup += ".s2ne-backup";
        auto backup_pending = backup;
        backup_pending += ".s2ne-tmp";
        if (!std::filesystem::copy_file(path, backup_pending, std::filesystem::copy_options::none, error))
        {
            return Result<bool>::failure(make_error("Cannot create exclusive original-save backup: " + error.message()));
        }
        int unused = -1;
        PendingFile backup_cleanup{ unused, backup_pending };
        const int backup_file = ::open(backup_pending.c_str(), O_RDONLY | O_CLOEXEC);
        if (backup_file < 0) { return failure("Cannot open original-save backup for flush"); }
        do { flushed = ::fsync(backup_file); } while (flushed < 0 && errno == EINTR);
        const int flush_error = errno;
        ::close(backup_file);
        if (flushed < 0)
        {
            errno = flush_error;
            return failure("Original-save backup flush failed");
        }
        if (::rename(backup_pending.c_str(), backup.c_str()) < 0) { return failure("Original-save backup replacement failed"); }
        do { flushed = ::fsync(directory_file); } while (flushed < 0 && errno == EINTR);
        if (flushed < 0) { return failure("Original-save backup directory flush failed; original save left untouched"); }
    }
    if (::rename(pending.c_str(), path.c_str()) < 0) { return failure("Original-save atomic replacement failed"); }
    do { flushed = ::fsync(directory_file); } while (flushed < 0 && errno == EINTR);
    if (flushed < 0) { return failure("Original-save directory flush failed after replacement; new save may already be visible"); }
    return Result<bool>::success(true);
#endif
}

auto SaveStore::try_write(const std::filesystem::path& root, unsigned slot, const Save::State& state, size_t items) -> Result<bool>
{
    if (slot < 1 || slot > 53)
    {
        return Result<bool>::failure(make_error("Invalid original save slot"));
    }
    auto encoded = Save::try_encode(state, items);
    if (!encoded)
    {
        return Result<bool>::failure(encoded.error());
    }
    auto path = Save::slot_path(root, slot);
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
        auto previous = Save::try_load(path, items);
        if (!previous)
        {
            return Result<bool>::failure(previous.error());
        }
        if (previous.value().description.front() != 0)
        {
            std::ifstream input(path, std::ios::binary);
            std::vector<uint8_t> previous_bytes{ std::istreambuf_iterator<char>(input), {} };
            if (!input || input.bad())
            {
                return Result<bool>::failure(make_error("Cannot read save backup source"));
            }
            auto backup = try_replace(Save::slot_path(root, 53), previous_bytes);
            if (!backup)
            {
                return backup;
            }
        }
    }
    return try_replace(path, encoded.value());
}

auto SaveStore::write(const std::filesystem::path& root, unsigned slot, const Save::State& state, size_t items) -> void
{
    auto result = try_write(root, slot, state, items);
    if (!result)
    {
        fatal(result.error());
    }
}
}    //namespace s2ne

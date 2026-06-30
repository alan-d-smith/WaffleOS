#pragma once

// Usage:
//     log_info("DISK",  "read %d sectors", count);
//     log_ok("IDT",     "Initialized IDT");
//     log_warn("FAT",   "filesystem not initialised");
//     log_error("DISK", "read error at LBA %d", lba);

typedef enum {
    LOG_INFO,    // routine status
    LOG_OK,      // success / completion
    LOG_WARN,    // recoverable problem
    LOG_ERROR,   // failure
} LogLevel;

void klog(LogLevel level, const char *tag, const char *fmt, ...);

#define log_info(tag, ...)  klog(LOG_INFO,  (tag), __VA_ARGS__)
#define log_ok(tag, ...)    klog(LOG_OK,    (tag), __VA_ARGS__)
#define log_warn(tag, ...)  klog(LOG_WARN,  (tag), __VA_ARGS__)
#define log_error(tag, ...) klog(LOG_ERROR, (tag), __VA_ARGS__)

/*
 * SPDX-FileCopyrightText: 2026 William Bell
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <limits.h>

    #ifdef __APPLE__
        #include <mach-o/dyld.h>
    #endif
#endif


/*
 * Get the absolute path to the currently running executable.
 *
 * Returns 0 on success, -1 on failure.
 */
static int get_executable_path(char *buffer, size_t size)
{
#ifdef _WIN32

    DWORD len = GetModuleFileNameA(NULL, buffer, (DWORD)size);

    if (len == 0 || len >= size)
        return -1;

    buffer[len] = '\0';
    return 0;

#elif defined(__APPLE__)

    uint32_t len = (uint32_t)size;

    if (_NSGetExecutablePath(buffer, &len) != 0)
        return -1;

    /*
     * _NSGetExecutablePath may return a non-canonical path.
     * realpath() turns it into an absolute path.
     */
    char resolved[PATH_MAX];

    if (realpath(buffer, resolved) == NULL)
        return -1;

    if (strlen(resolved) >= size)
        return -1;

    strcpy(buffer, resolved);
    return 0;

#elif defined(__linux__)

    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);

    if (len < 0 || (size_t)len >= size)
        return -1;

    buffer[len] = '\0';
    return 0;

#else

    return -1;

#endif
}


/*
 * Remove the filename from a path, leaving the directory.
 *
 * "/foo/bar/baz" -> "/foo/bar/"
 * "C:\\foo\\bar\\baz.exe" -> "C:\\foo\\bar\\"
 */
static int get_executable_directory(
    const char *path,
    char *directory,
    size_t size
)
{
    const char *last_slash = strrchr(path, '/');

#ifdef _WIN32
    const char *last_backslash = strrchr(path, '\\');

    if (last_backslash &&
        (!last_slash || last_backslash > last_slash))
    {
        last_slash = last_backslash;
    }
#endif

    if (!last_slash)
        return -1;

    size_t length = (size_t)(last_slash - path + 1);

    if (length >= size)
        return -1;

    memcpy(directory, path, length);
    directory[length] = '\0';

    return 0;
}


/*
 * Get the executable's filename without its extension.
 *
 * Windows:
 *     isotope.exe -> isotope
 *
 * Unix:
 *     isotope -> isotope
 */
static int get_executable_name(
    const char *path,
    char *name,
    size_t size
)
{
    const char *filename = strrchr(path, '/');

#ifdef _WIN32
    const char *backslash = strrchr(path, '\\');

    if (backslash &&
        (!filename || backslash > filename))
    {
        filename = backslash;
    }
#endif

    if (filename)
        filename++;
    else
        filename = path;

    size_t length = strlen(filename);

#ifdef _WIN32
    const char *dot = strrchr(filename, '.');

    if (dot && strcmp(dot, ".exe") == 0)
        length = (size_t)(dot - filename);
#endif

    if (length == 0 || length >= size)
        return -1;

    memcpy(name, filename, length);
    name[length] = '\0';

    return 0;
}


/*
 * Read a single package name from the config file.
 *
 * The config is expected to contain something like:
 *
 *     some-package
 *
 * Leading/trailing whitespace is ignored.
 */
static int read_package_name(
    const char *path,
    char *package,
    size_t size
)
{
    FILE *file = fopen(path, "r");

    if (!file)
        return -1;

    if (!fgets(package, (int)size, file)) {
        fclose(file);
        return -1;
    }

    fclose(file);

    /*
     * Remove newline.
     */
    package[strcspn(package, "\r\n")] = '\0';

    /*
     * Trim leading whitespace.
     */
    char *start = package;

    while (*start == ' ' || *start == '\t')
        start++;

    if (start != package)
        memmove(package, start, strlen(start) + 1);

    /*
     * Trim trailing whitespace.
     */
    size_t length = strlen(package);

    while (length > 0 &&
           (package[length - 1] == ' ' ||
            package[length - 1] == '\t'))
    {
        package[--length] = '\0';
    }

    if (package[0] == '\0')
        return -1;

    return 0;
}


#ifdef _WIN32

/*
 * Ctrl+C is delivered to every process attached to the console. Ignore it in
 * the shim so only argon handles it; otherwise the shim would exit first and
 * hand the prompt back while argon is still running.
 */
static BOOL WINAPI ignore_ctrl_c(DWORD type)
{
    return type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT;
}

/*
 * Append one argument to a command line, quoted so that the child's standard
 * CRT parsing gives back exactly the same string (handles spaces, quotes and
 * backslashes before quotes).
 *
 * Returns the new length, or 0 if the buffer is too small.
 */
static size_t append_quoted_argument(
    char *cmdline,
    size_t length,
    size_t capacity,
    const char *arg
)
{
    size_t needed = length + strlen(arg) * 2 + 4;

    if (needed >= capacity)
        return 0;

    char *out = cmdline + length;

    if (length > 0)
        *out++ = ' ';

    *out++ = '"';

    for (const char *p = arg;; p++)
    {
        size_t backslashes = 0;

        while (*p == '\\')
        {
            backslashes++;
            p++;
        }

        if (*p == '\0')
        {
            /* Backslashes before the closing quote must be doubled. */
            for (size_t i = 0; i < backslashes * 2; i++)
                *out++ = '\\';
            break;
        }

        if (*p == '"')
        {
            /* Double the backslashes and escape the quote itself. */
            for (size_t i = 0; i < backslashes * 2 + 1; i++)
                *out++ = '\\';
            *out++ = '"';
        }
        else
        {
            for (size_t i = 0; i < backslashes; i++)
                *out++ = '\\';
            *out++ = *p;
        }
    }

    *out++ = '"';
    *out = '\0';

    return (size_t)(out - cmdline);
}

/*
 * Run argon, wait for it, and return its exit code.
 *
 * _execv() is not used on Windows: it does not replace the process, so the
 * parent returns to the shell immediately (losing the exit code), and it does
 * not quote arguments, which breaks paths like "C:\Program Files\Argon".
 */
static int run_argon(const char *argon_path, char **args, int arg_count)
{
    /* Windows limits a command line to 32767 characters. */
    enum { CMDLINE_CAPACITY = 32768 };

    char *cmdline = malloc(CMDLINE_CAPACITY);

    if (!cmdline) {
        perror("malloc");
        return 1;
    }

    cmdline[0] = '\0';

    size_t length = append_quoted_argument(
        cmdline, 0, CMDLINE_CAPACITY, argon_path
    );

    for (int i = 0; length != 0 && i < arg_count; i++)
    {
        length = append_quoted_argument(
            cmdline, length, CMDLINE_CAPACITY, args[i]
        );
    }

    if (length == 0) {
        fprintf(stderr, "Command line too long\n");
        free(cmdline);
        return 1;
    }

    STARTUPINFOA startup_info;
    PROCESS_INFORMATION process_info;

    memset(&startup_info, 0, sizeof(startup_info));
    memset(&process_info, 0, sizeof(process_info));
    startup_info.cb = sizeof(startup_info);

    SetConsoleCtrlHandler(ignore_ctrl_c, TRUE);

    if (!CreateProcessA(
            argon_path,
            cmdline,
            NULL,
            NULL,
            TRUE, /* inherit stdin/stdout/stderr handles */
            0,
            NULL,
            NULL,
            &startup_info,
            &process_info
        ))
    {
        fprintf(
            stderr,
            "Failed to execute '%s' (error %lu)\n",
            argon_path,
            (unsigned long)GetLastError()
        );
        free(cmdline);
        return 1;
    }

    free(cmdline);

    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code = 1;
    GetExitCodeProcess(process_info.hProcess, &exit_code);

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return (int)exit_code;
}

#endif

int main(int argc, char **argv)
{
    char executable_path[4096];
    char executable_directory[4096];
    char executable_name[256];

    if (get_executable_path(
            executable_path,
            sizeof(executable_path)
        ) != 0)
    {
        fprintf(
            stderr,
            "Could not determine executable path\n"
        );
        return 1;
    }

    if (get_executable_directory(
            executable_path,
            executable_directory,
            sizeof(executable_directory)
        ) != 0)
    {
        fprintf(
            stderr,
            "Could not determine executable directory\n"
        );
        return 1;
    }

    if (get_executable_name(
            executable_path,
            executable_name,
            sizeof(executable_name)
        ) != 0)
    {
        fprintf(
            stderr,
            "Could not determine executable name\n"
        );
        return 1;
    }


    /*
     * Build:
     *
     *     ../tools-config/<name>.conf
     */
    char config_path[4096];

    if (snprintf(
            config_path,
            sizeof(config_path),
            "%s../tools-config/%s.conf",
            executable_directory,
            executable_name
        ) >= (int)sizeof(config_path))
    {
        fprintf(stderr, "Config path too long\n");
        return 1;
    }


    /*
     * Read package name.
     */
    char package_name[1024];

    if (read_package_name(
            config_path,
            package_name,
            sizeof(package_name)
        ) != 0)
    {
        fprintf(
            stderr,
            "Could not read config: %s\n",
            config_path
        );
        return 1;
    }


    /*
     * Build:
     *
     *     ../bin/argon
     *
     * or:
     *
     *     ../bin/argon.exe
     */
    char argon_path[4096];

#ifdef _WIN32

    if (snprintf(
            argon_path,
            sizeof(argon_path),
            "%s../bin/argon.exe",
            executable_directory
        ) >= (int)sizeof(argon_path))
    {
        fprintf(stderr, "Argon path too long\n");
        return 1;
    }

#else

    if (snprintf(
            argon_path,
            sizeof(argon_path),
            "%s../bin/argon",
            executable_directory
        ) >= (int)sizeof(argon_path))
    {
        fprintf(stderr, "Argon path too long\n");
        return 1;
    }

#endif


    /*
     * Construct argv for argon:
     *
     *     argon <package-name> <original arguments...>
     */
    char **new_argv = malloc(
        sizeof(char *) * (argc + 2)
    );

    if (!new_argv) {
        perror("malloc");
        return 1;
    }

    new_argv[0] = argon_path;
    new_argv[1] = package_name;

    for (int i = 1; i < argc; i++)
        new_argv[i + 1] = argv[i];

    new_argv[argc + 1] = NULL;


#ifdef _WIN32

    /*
     * new_argv[1..] are the package name plus the original arguments.
     */
    int exit_code = run_argon(argon_path, new_argv + 1, argc);

    free(new_argv);

    return exit_code;

#else

    /*
     * Unix/macOS.
     *
     * If successful, this does not return.
     */
    execv(argon_path, new_argv);

#endif


    /*
     * Only reached if execution failed.
     */
    fprintf(
        stderr,
        "Failed to execute '%s': %s\n",
        argon_path,
        strerror(errno)
    );

    free(new_argv);

    return 1;
}
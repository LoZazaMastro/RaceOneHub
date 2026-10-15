using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.Json;

namespace ForzaMastro.Launcher;

internal static class PortablePayload
{
    internal const string ResourceName = "RaceOneHub.Support.zip";
    internal sealed record Item(string path, long bytes, string sha256);
    internal sealed record Manifest(int schema, Item[] files);
    private const long MaximumBytes = 512L * 1024 * 1024;

    internal static string? Prepare()
    {
        using var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(ResourceName);
        if (stream is null) return null; // Normal source-development build.
        var hash = Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant();
        stream.Position = 0;
        var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "RaceOneHub", "components");
        using var mutex = new Mutex(false, @"Local\RaceOneHub.Support." + hash);
        bool acquired;
        try { acquired = mutex.WaitOne(TimeSpan.FromSeconds(30)); }
        catch (AbandonedMutexException) { acquired = true; }
        if (!acquired) throw new IOException("Another Race One Hub instance is preparing its components.");
        try { return Extract(stream, directory, hash); }
        finally { mutex.ReleaseMutex(); }
    }

    private static void NoLinks(string path)
    {
        for (DirectoryInfo? item = new(Path.GetFullPath(path)); item is not null; item = item.Parent)
            if (item.Exists && (item.Attributes & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Component directories must not contain links or reparse points.");
    }

    private static string SafePath(string root, string relative)
    {
        var parts = relative.Split('/');
        if (string.IsNullOrEmpty(relative) || relative.Contains('\\') || relative.Contains(':') ||
            relative.Contains('\0') || parts.Any(p => p is "" or "." or ".." || p.EndsWith('.') || p.EndsWith(' ') ||
                System.Text.RegularExpressions.Regex.IsMatch(p.Split('.')[0], "^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$", System.Text.RegularExpressions.RegexOptions.IgnoreCase)))
            throw new InvalidDataException("Invalid embedded component path.");
        var target = Path.GetFullPath(Path.Combine(root, Path.Combine(parts)));
        if (!target.StartsWith(Path.GetFullPath(root) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("Embedded component escapes its directory.");
        return target;
    }

    internal static string Extract(Stream stream, string componentRoot, string hash)
    {
        if (hash.Length != 64 || hash.Any(c => !Uri.IsHexDigit(c))) throw new InvalidDataException("Invalid component identity.");
        componentRoot = Path.GetFullPath(componentRoot);
        NoLinks(componentRoot);
        Directory.CreateDirectory(componentRoot);
        var destination = Path.Combine(componentRoot, hash);
        using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: true);
        var manifestEntry = archive.GetEntry("manifest.json") ?? throw new InvalidDataException("Component manifest is missing.");
        if (manifestEntry.Length > 1024 * 1024) throw new InvalidDataException("Component manifest is too large.");
        Manifest manifest;
        using (var reader = manifestEntry.Open())
            manifest = JsonSerializer.Deserialize<Manifest>(reader) ?? throw new InvalidDataException("Invalid component manifest.");
        if (manifest.schema != 1 || manifest.files is null || manifest.files.Length is < 1 or > 512)
            throw new InvalidDataException("Unsupported component manifest.");
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        long total = 0;
        foreach (var item in manifest.files)
        {
            SafePath(destination, item.path);
            if (!names.Add(item.path) || item.path == "manifest.json" || item.bytes < 0 ||
                item.bytes > MaximumBytes || item.sha256.Length != 64 || item.sha256.Any(c => !Uri.IsHexDigit(c)))
                throw new InvalidDataException("Invalid or duplicate component entry.");
            total = checked(total + item.bytes);
            if (total > MaximumBytes) throw new InvalidDataException("Embedded components are too large.");
            var entry = archive.GetEntry(item.path) ?? throw new InvalidDataException("Embedded component is missing.");
            if (entry.Length != item.bytes || ((entry.ExternalAttributes >> 16) & 0xf000) == 0xa000)
                throw new InvalidDataException("Embedded component metadata differs.");
        }
        if (archive.Entries.Count != manifest.files.Length + 1)
            throw new InvalidDataException("Unexpected files in embedded components.");
        if (Directory.Exists(destination))
        {
            Verify(destination, manifest);
            return destination;
        }
        var staging = Path.Combine(componentRoot, ".staging-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(staging);
        try
        {
            foreach (var item in manifest.files)
            {
                var target = SafePath(staging, item.path);
                Directory.CreateDirectory(Path.GetDirectoryName(target)!);
                using var input = archive.GetEntry(item.path)!.Open();
                using var output = new FileStream(target, FileMode.CreateNew, FileAccess.Write, FileShare.None);
                input.CopyTo(output);
            }
            Verify(staging, manifest);
            Directory.Move(staging, destination);
            return destination;
        }
        finally
        {
            // Only this freshly created direct child is owned by this attempt.
            if (Path.GetDirectoryName(staging) == componentRoot && Path.GetFileName(staging).StartsWith(".staging-", StringComparison.Ordinal)
                && Directory.Exists(staging)) { NoLinks(staging); Directory.Delete(staging, true); }
        }
    }

    private static void Verify(string root, Manifest manifest)
    {
        NoLinks(root);
        foreach (var item in manifest.files)
        {
            var target = SafePath(root, item.path);
            NoLinks(Path.GetDirectoryName(target)!);
            var file = new FileInfo(target);
            if (!file.Exists || (file.Attributes & FileAttributes.ReparsePoint) != 0 || file.Length != item.bytes)
                throw new InvalidDataException("A bundled component is missing or was changed: " + item.path);
            using var input = file.OpenRead();
            if (!Convert.ToHexString(SHA256.HashData(input)).Equals(item.sha256, StringComparison.OrdinalIgnoreCase))
                throw new InvalidDataException("A bundled component was changed: " + item.path);
        }
    }
}

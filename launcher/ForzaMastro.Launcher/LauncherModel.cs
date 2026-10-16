using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text.Json;

namespace ForzaMastro.Launcher;

public sealed record Choice(string Id, string Label)
{
    public override string ToString() => Label;
}

public sealed record BuildProgress(string Phase, long BytesCopied, long TotalBytes)
{
    public double Percent => TotalBytes > 0 ? 100.0 * BytesCopied / TotalBytes : 0;
    public string Label => Phase switch {
        "preparing_scene_overrides" => $"Preparing scene settings · {Percent:0}%",
        _ => "Building EXE…"
    };
    public static BuildProgress Parse(string json)
    {
        using var document = JsonDocument.Parse(json);
        var root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object || !root.TryGetProperty("phase", out var phase) || phase.ValueKind != JsonValueKind.String ||
            !root.TryGetProperty("bytes_copied", out var copied) || copied.ValueKind != JsonValueKind.Number || !copied.TryGetInt64(out var bytes) ||
            !root.TryGetProperty("total_bytes", out var total) || total.ValueKind != JsonValueKind.Number || !total.TryGetInt64(out var maximum) ||
            bytes < 0 || maximum <= 0 || bytes > maximum)
            throw new InvalidDataException("Invalid build progress.");
        return new(phase.GetString()!, bytes, maximum);
    }
}

public sealed class Option : INotifyPropertyChanged
{
    public string Key { get; }
    public string Label { get; }
    public string Description { get; }
    public Choice[] Choices { get; private set; }
    private Choice[] knownChoices;
    private Choice selected;
    public Choice Selected { get => selected; set { selected = value; Changed(); } }
    public bool Enabled { get; set; }
    private string availability = "";
    public string Availability { get => availability; set { availability = value; Changed(); } }
    public void SetCatalog(Choice[] choices)
    {
        if (choices.Length == 0 || choices.Any(c => string.IsNullOrWhiteSpace(c.Id) || string.IsNullOrWhiteSpace(c.Label)) || choices.Select(c => c.Id).Distinct(StringComparer.Ordinal).Count() != choices.Length)
            throw new InvalidDataException("Invalid language catalog.");
        knownChoices = choices;
        Choices = choices;
        if (choices.FirstOrDefault(c => c.Id == Selected.Id) is { } preserved) Selected = preserved;
    }
    public void LimitTo(string[] values) { Choices = knownChoices.Where(c => values.Contains(c.Id, StringComparer.Ordinal)).ToArray(); Enabled = Choices.Length > 0; if (Enabled && !Choices.Contains(Selected)) Selected = Choices[0]; }
    public Option(string key, string label, string description, params Choice[] choices)
    { Key = key; Label = label; Description = description; Choices = choices; knownChoices = choices; selected = choices[0]; }
    public event PropertyChangedEventHandler? PropertyChanged;
    private void Changed([CallerMemberName] string? name = null) => PropertyChanged?.Invoke(this, new(name));
}

public sealed class BackendCapabilities
{
    public int schema_version { get; set; }
    public string game_id { get; set; } = "";
    public bool ready { get; set; }
    public bool running { get; set; }
    public bool build_ready { get; set; }
    public string output_filename { get; set; } = "";
    public Dictionary<string, JsonElement> current { get; set; } = [];
    public Dictionary<string, JsonElement[]> supported { get; set; } = [];
    public Dictionary<string, JsonElement> preset_status { get; set; } = [];
    public Dictionary<string, string> language_labels { get; set; } = [];
    public string[] warnings { get; set; } = [];
}

public sealed class LauncherModel
{
    private sealed class BuilderPreferences
    {
        public string OutputDirectory { get; set; } = "";
        public string GameFolder { get; set; } = "";
        public bool CreateDesktopShortcut { get; set; }
    }
    public static readonly JsonSerializerOptions Json = new() { WriteIndented = true, PropertyNamingPolicy = JsonNamingPolicy.CamelCase, PropertyNameCaseInsensitive = true };
    public string ProjectRoot { get; }
    public const string GeneratedExecutableName = "RaceOne5.exe"; // Legacy FM5 protocol name.
    public string GameId { get; }
    public string GameTitle => "Forza Motorsport 5";
    public string OutputExecutableName => GeneratedExecutableName;
    public string RuntimeDirectory => "pc";
    public string OutputDirectory { get; set; }
    private string gameFolder = "";
    public string GameFolder
    {
        get => gameFolder;
        set
        {
            var normalized = NormalizeGameFolder(value);
            if (string.Equals(gameFolder, normalized, StringComparison.OrdinalIgnoreCase)) return;
            gameFolder = normalized;
            Backend = null; // A different source needs its own language catalog.
            Status = "Checking selected game files...";
        }
    }
    public bool CreateDesktopShortcut { get; set; }
    public bool SkipPreGame
    {
        get => BuilderOptions[0].Selected.Id == "false";
        set => BuilderOptions[0].Selected = BuilderOptions[0].Choices.First(choice => choice.Id == (value ? "false" : "true"));
    }
    public string OutputFile => Path.Combine(OutputDirectory, OutputExecutableName);
    public string DataDirectory => Path.Combine(ProjectRoot, "launcher", "data");
    public string RuntimePath => Path.Combine(ProjectRoot, "runtime", RuntimeDirectory, "ForzaMastro5.exe");
    public string LogsPath => Path.Combine(ProjectRoot, "runtime", RuntimeDirectory, "data", "logs");
    public string PackagedBackend => Path.Combine(ProjectRoot, "launcher", "backend", "ForzaMastro5Backend.exe");
    public string DevelopmentBackend => Path.Combine(ProjectRoot, "tools", "pc_launcher_backend.py");
    public string DevelopmentPython => Path.Combine(ProjectRoot, ".venv", "Scripts", "python.exe");
    public BackendCapabilities? Backend { get; private set; }
    private bool hasSavedSettings;
    private Dictionary<string, string> savedSelections = [];
    private bool initializedFromBackend;
    public string Status { get; private set; } = "PC profile not connected";
    public Option[] Video { get; } = [
        new("resolution", "Resolution", "", new("2160", "4K (3840x2160)"), new("1080", "1080p (1920x1080)"), new("1440", "1440p (2560x1440)"), new("2880", "5K (5120x2880)"), new("3240", "6K (5760x3240)"), new("3780", "7K (6720x3780)"), new("4320", "8K (7680x4320)")),
        new("frame_rate", "Frame rate", "120 FPS is experimental.", new("30", "30 FPS"), new("60", "60 FPS"), new("120", "120 FPS · experimental")),
        new("antialiasing", "MSAA", "", new("off", "1\u00d7"), new("msaax2", "2\u00d7"), new("msaax4", "4\u00d7")),
        new("graphics", "Graphics quality", "", new("default", "Original"), new("low", "Low"), new("high", "High"), new("ultra", "Ultra")),
        new("display", "Display mode", "", new("fullscreen", "Fullscreen"), new("windowed", "Windowed")),
        new("vsync", "VSync", "", new("true", "On"), new("false", "Off"))
    ];
    public Option[] Audio { get; } = [
        new("language", "Language", "", new Choice("it-IT", "Italian")),
        new("audio", "Audio output", "Uses the system speaker configuration.", new("multichannel", "Multichannel"), new("stereo", "Stereo"))
    ];
    public Option[] BuilderOptions { get; } = [
        new("show_pre_game", "Show settings before playing", "", new("true", "On"), new("false", "Off"))
    ];
    public IEnumerable<Option> All => Video.Concat(Audio).Concat(BuilderOptions);
    public bool CanBuild => Backend is { build_ready: true, running: false } && Backend.output_filename == OutputExecutableName &&
                            Directory.Exists(GameFolder) && File.Exists(RuntimePath) && All.All(o => o.Enabled);

    public LauncherModel(string root, string gameId = "fm5")
    {
        if (gameId != "fm5") throw new ArgumentOutOfRangeException(nameof(gameId));
        GameId = gameId;
        ProjectRoot = Path.GetFullPath(root);
        OutputDirectory = Path.Combine(ProjectRoot, "launcher", "output", "RaceOne5");
        Video[1].Selected = Video[1].Choices.Single(choice => choice.Id == "60");
        Video[2].Selected = Video[2].Choices.Single(choice => choice.Id == "msaax2");
        Load();
    }
    public void SelectOutputParent(string selectedParent)
    {
        if (string.IsNullOrWhiteSpace(selectedParent) || !Path.IsPathFullyQualified(selectedParent))
            throw new InvalidDataException("Choose an absolute output folder path.");
        var parent = Path.TrimEndingDirectorySeparator(Path.GetFullPath(selectedParent));
        OutputDirectory = string.Equals(Path.GetFileName(parent), GameTitle, StringComparison.OrdinalIgnoreCase)
            ? parent : Path.Combine(parent, GameTitle);
    }
    private static bool IsSameOrNested(string candidate, string parent)
    {
        var path = Path.TrimEndingDirectorySeparator(Path.GetFullPath(candidate));
        var ancestor = Path.TrimEndingDirectorySeparator(Path.GetFullPath(parent));
        return string.Equals(path, ancestor, StringComparison.OrdinalIgnoreCase) ||
               path.StartsWith(Path.EndsInDirectorySeparator(ancestor) ? ancestor : ancestor + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase);
    }
    public static string NormalizeGameFolder(string path)
    {
        if (string.IsNullOrWhiteSpace(path)) return "";
        if (!Path.IsPathFullyQualified(path)) throw new InvalidDataException("Choose an absolute game folder path.");
        var normalized = Path.TrimEndingDirectorySeparator(Path.GetFullPath(path));
        return string.Equals(Path.GetFileName(normalized), "Mount", StringComparison.OrdinalIgnoreCase)
            ? Path.GetDirectoryName(normalized) ?? normalized : normalized;
    }
    public static string FindProject(string start)
    {
        for (DirectoryInfo? dir = new(start); dir is not null; dir = dir.Parent)
            if (File.Exists(Path.Combine(dir.FullName, "tools", "pc_launcher_backend.py")) && Directory.Exists(Path.Combine(dir.FullName, "launcher"))) return dir.FullName;
        throw new DirectoryNotFoundException("Place Race One Hub beside its required files.");
    }
    public void Load()
    {
        try
        {
            var settings = Path.Combine(DataDirectory, "settings.json");
            var preferences = Path.Combine(DataDirectory, "builder-state.json");
            if (File.Exists(preferences))
            {
                var state = JsonSerializer.Deserialize<BuilderPreferences>(File.ReadAllText(preferences), Json);
                if (state is not null)
                {
                    if (Path.IsPathFullyQualified(state.OutputDirectory) && !string.Equals(Path.GetFullPath(state.OutputDirectory),
                        Path.Combine(ProjectRoot, "launcher", "output", "ForzaMastro5"), StringComparison.OrdinalIgnoreCase)) OutputDirectory = state.OutputDirectory;
                    if (Path.IsPathFullyQualified(state.GameFolder)) GameFolder = state.GameFolder;
                    CreateDesktopShortcut = state.CreateDesktopShortcut;
                }
            }
            if (File.Exists(settings))
            {
                hasSavedSettings = true;
                savedSelections = JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText(settings), Json) ?? [];
                if (savedSelections.GetValueOrDefault("antialiasing") == "msaax8") savedSelections["antialiasing"] = "msaax4";
                foreach (var option in All)
                    if (savedSelections.TryGetValue(option.Key, out var id) && option.Choices.FirstOrDefault(c => c.Id == id) is { } choice) option.Selected = choice;
            }
        }
        catch (Exception ex) when (ex is IOException or JsonException or InvalidDataException or UnauthorizedAccessException)
        { Backend = null; Status = "Settings unavailable: " + ex.Message; }
    }
    public void ApplyCapabilities(string json)
    {
        var backend = JsonSerializer.Deserialize<BackendCapabilities>(json, Json) ?? throw new InvalidDataException("Empty backend response.");
        if (backend.schema_version != 1 || backend.game_id != GameId) throw new InvalidDataException("Backend is not compatible with " + GameTitle + ".");
        if (backend.build_ready && backend.output_filename != OutputExecutableName) throw new InvalidDataException("Unexpected generated executable name.");
        Backend = backend;
        ApplyLanguageCatalog(Audio[0], backend.language_labels);
        if (!initializedFromBackend)
        {
            foreach (var option in All)
                if (hasSavedSettings && savedSelections.TryGetValue(option.Key, out var saved))
                {
                    if (option.Choices.FirstOrDefault(choice => choice.Id == saved) is { } preserved) option.Selected = preserved;
                }
                else if (backend.current.TryGetValue(option.Key, out var value))
                {
                    var id = value.ValueKind is JsonValueKind.True or JsonValueKind.False ? value.GetBoolean().ToString().ToLowerInvariant() : value.ToString();
                    if (option.Key == "antialiasing" && id == "msaax8") id = "msaax4";
                    if (option.Choices.FirstOrDefault(choice => choice.Id == id) is { } current) option.Selected = current;
                }
        }
        initializedFromBackend = true;
        foreach (var option in All)
        {
            option.LimitTo(backend.supported.TryGetValue(option.Key, out var values) ? values.Select(v => v.ToString().ToLowerInvariant() == "true" ? "true" : v.ToString().ToLowerInvariant() == "false" ? "false" : v.ToString()).ToArray() : []);
            option.Availability = option.Enabled ? "" : "Unavailable in this PC profile";
        }
        Status =
                 !All.All(option => option.Enabled) ? "Some settings are unavailable in this build." :
                 string.IsNullOrEmpty(GameFolder) ? "Choose your extracted " + GameTitle + " folder." :
                 !Directory.Exists(GameFolder) ? "The selected game folder is unavailable. Choose it again." :
                 backend.running ? "Close " + Path.GetFileNameWithoutExtension(OutputExecutableName) + " before building." :
                 backend.build_ready ? "Ready to build." : "Required runtime components are unavailable.";
        if (backend.warnings.Length > 0) Status += " " + string.Join(" ", backend.warnings);
        UpdateNotes();
    }
    private static void ApplyLanguageCatalog(Option option, Dictionary<string, string> labels)
    {
        if (labels.Count == 0) return;
        var choices = labels.Where(pair => pair.Key != "system").Select(pair => new Choice(pair.Key, pair.Value)).OrderBy(choice => choice.Label, StringComparer.OrdinalIgnoreCase).ToArray();
        if (choices.Length > 0) option.SetCatalog(choices);
    }
    public void UpdateNotes()
    {
        var graphics = Video.First(o => o.Key == "graphics");
        if (Backend?.preset_status.TryGetValue(graphics.Selected.Id, out var note) == true && note.ValueKind == JsonValueKind.String)
            graphics.Availability = note.GetString() ?? "";
    }
    public ProcessStartInfo BackendCommand(params string[] arguments)
    {
        ProcessStartInfo info;
        if (File.Exists(PackagedBackend)) info = new(PackagedBackend);
        else if (File.Exists(DevelopmentBackend))
        {
            // A source checkout's own virtualenv has the backend's packages; PATH python may not.
            info = new(File.Exists(DevelopmentPython) ? DevelopmentPython : "python");
            info.ArgumentList.Add(DevelopmentBackend);
        }
        else throw new FileNotFoundException("PC backend is not available yet.");
        info.UseShellExecute = false; info.WorkingDirectory = ProjectRoot;
        info.RedirectStandardOutput = true; info.RedirectStandardError = true; info.CreateNoWindow = true;
        info.ArgumentList.Add("--game"); info.ArgumentList.Add(GameId);
        foreach (var argument in arguments) info.ArgumentList.Add(argument);
        return info;
    }
    public async Task RefreshAsync()
    {
        var selectedSource = GameFolder;
        using var process = Process.Start(CapabilitiesCommand()) ?? throw new IOException("Could not start the PC backend.");
        var output = process.StandardOutput.ReadToEndAsync(); var error = process.StandardError.ReadToEndAsync();
        await process.WaitForExitAsync();
        var stdout = await output; var stderr = await error;
        if (process.ExitCode != 0) throw new IOException(DescribeBackendError(stdout, stderr, process.ExitCode));
        if (!string.Equals(selectedSource, GameFolder, StringComparison.OrdinalIgnoreCase)) return;
        ApplyCapabilities(stdout);
    }
    public ProcessStartInfo CapabilitiesCommand() => string.IsNullOrEmpty(GameFolder)
        ? BackendCommand("--capabilities") : BackendCommand("--capabilities", "--game-folder", GameFolder);
    public static string DescribeBackendError(string stdout, string stderr, int exitCode)
    {
        try
        {
            using var document = JsonDocument.Parse(stdout);
            if (document.RootElement.ValueKind == JsonValueKind.Object && document.RootElement.TryGetProperty("error", out var error) && error.ValueKind == JsonValueKind.String && !string.IsNullOrWhiteSpace(error.GetString()))
                return error.GetString()!;
        }
        catch (JsonException) { }
        return !string.IsNullOrWhiteSpace(stderr) ? stderr.Trim() : $"PC backend unavailable (code {exitCode}).";
    }
    public Dictionary<string, string> Values() => All.ToDictionary(o => o.Key, o => o.Selected.Id);
    public void Save()
    {
        Directory.CreateDirectory(DataDirectory);
        var path = Path.Combine(DataDirectory, "settings.json");
        File.WriteAllText(path + ".tmp", JsonSerializer.Serialize(Values(), Json));
        File.Move(path + ".tmp", path, true);
        hasSavedSettings = true;
        var preferences = Path.Combine(DataDirectory, "builder-state.json");
        File.WriteAllText(preferences + ".tmp", JsonSerializer.Serialize(new BuilderPreferences {
            OutputDirectory = OutputDirectory, GameFolder = GameFolder, CreateDesktopShortcut = CreateDesktopShortcut }, Json));
        File.Move(preferences + ".tmp", preferences, true);
    }
    public ProcessStartInfo PrepareBuild(string requestPath, string outputPath)
    {
        if (string.IsNullOrEmpty(GameFolder) || !Directory.Exists(GameFolder)) throw new InvalidOperationException("Choose your extracted " + GameTitle + " folder.");
        if (!CanBuild || Backend is null) throw new InvalidOperationException("The EXE builder is not ready.");
        if (!Path.IsPathFullyQualified(outputPath) || !string.Equals(Path.GetFileName(outputPath), OutputExecutableName, StringComparison.Ordinal)) throw new InvalidDataException("Choose an output folder for " + OutputExecutableName + ".");
        var destination = Path.GetDirectoryName(outputPath)!;
        if (IsSameOrNested(destination, GameFolder) || IsSameOrNested(GameFolder, destination))
            throw new InvalidDataException("Choose an output folder separate from the game files.");
        foreach (var option in All)
            if (!Backend.supported.TryGetValue(option.Key, out var allowed) || !allowed.Any(v => string.Equals(v.ToString(), option.Selected.Id, StringComparison.OrdinalIgnoreCase)))
                throw new InvalidDataException("Unsupported option: " + option.Label);
        Save();
        var values = Values().ToDictionary(kv => kv.Key, kv => kv.Key is "resolution" or "frame_rate" ? (object)int.Parse(kv.Value, System.Globalization.CultureInfo.InvariantCulture) : kv.Key is "vsync" or "show_pre_game" ? bool.Parse(kv.Value) : kv.Value);
        File.WriteAllText(requestPath, JsonSerializer.Serialize(values, Json));
        var arguments = new List<string> { "--settings", requestPath, "--build", "--output", outputPath, "--game-folder", GameFolder,
                                           "--progress-file", requestPath + ".progress.json" };
        if (CreateDesktopShortcut) arguments.Add("--desktop-shortcut");
        return BackendCommand(arguments.ToArray());
    }
}

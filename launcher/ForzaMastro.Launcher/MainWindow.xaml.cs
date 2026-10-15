using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;
using System.Windows.Media;

namespace ForzaMastro.Launcher;

public partial class MainWindow : Window
{
    private LauncherModel? model;
    private readonly Dictionary<string, LauncherModel> games = [];
    private bool refreshing;
    private bool refreshQueued;
    private bool launching;
    public MainWindow(bool previewOnly = false, string? projectRoot = null)
    {
        InitializeComponent();
        try
        {
            var root = projectRoot ?? LauncherModel.FindProject(AppContext.BaseDirectory);
            games["fm5"] = new LauncherModel(root, "fm5");
            foreach (var game in games.Values)
                foreach (var option in game.All) option.PropertyChanged += (_, e) => { if (ReferenceEquals(model, game) && e.PropertyName == nameof(Option.Selected)) { UpdateSummary(); game.UpdateNotes(); } };
            model = games["fm5"];
            BindGame();
            if (!previewOnly)
            {
                Loaded += async (_, _) => await RefreshBackend();
                Activated += async (_, _) => { if (IsLoaded && !launching) await RefreshBackend(); };
            }
            if (!previewOnly)
                Closing += (_, _) => { try { foreach (var game in games.Values) game.Save(); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { StatusLabel.Text = ex.Message; } };
        }
        catch (Exception ex) { StatusLabel.Text = ex.Message; BuildButton.IsEnabled = false; }
    }
    private async Task RefreshBackend(bool updateStatus = true)
    {
        if (model is null) return;
        if (refreshing) { refreshQueued = true; return; }
        var selected = model;
        refreshing = true; BuildButton.IsEnabled = false;
        try
        {
            await selected.RefreshAsync();
            if (!ReferenceEquals(model, selected)) { refreshQueued = true; return; }
            VideoOptions.Items.Refresh(); AudioOptions.Items.Refresh();
            var preGame = model.BuilderOptions[0];
            PreGameSettingsBox.IsEnabled = preGame.Enabled && preGame.Choices.Length == 2;
            PreGameSettingsBox.IsChecked = model.SkipPreGame;
            if (updateStatus) StatusLabel.Text = model.Status;
            BuildButton.IsEnabled = model.CanBuild && !launching; UpdateSummary();
        }
        catch (Exception ex) { if (ReferenceEquals(model, selected)) { if (updateStatus) StatusLabel.Text = ex.Message; BuildButton.IsEnabled = false; } }
        finally
        {
            refreshing = false;
            if (refreshQueued) { refreshQueued = false; await RefreshBackend(updateStatus); }
        }
    }
    private void BindGame()
    {
        if (model is null) return;
        VideoOptions.ItemsSource = model.Video; AudioOptions.ItemsSource = model.Audio;
        OutputPath.Text = model.OutputDirectory; GamePath.Text = model.GameFolder;
        DesktopShortcutBox.IsChecked = model.CreateDesktopShortcut;
        PreGameSettingsBox.IsChecked = model.SkipPreGame;
        PreGameSettingsBox.IsEnabled = model.BuilderOptions[0].Enabled && model.BuilderOptions[0].Choices.Length == 2;
        PreGameWarning.Visibility = model.SkipPreGame ? Visibility.Visible : Visibility.Collapsed;
        StatusLabel.Text = model.Status; BuildButton.IsEnabled = model.CanBuild && !launching;
        GameTitle.Text = model.GameTitle; Eyebrow.Text = model.GameTitle.ToUpperInvariant();
        GameNumber.Text = "5";
        var color = (Color)ColorConverter.ConvertFromString("#FF8A44");
        Resources["Accent"] = new SolidColorBrush(color);
        Resources["AccentHover"] = new SolidColorBrush(color) { Opacity = .85 };
        Resources["AccentPressed"] = new SolidColorBrush(color) { Opacity = .7 };
        Resources["AccentText"] = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#17191B"));
        GameHero.BorderBrush = new SolidColorBrush(Color.FromArgb(0x66, color.R, color.G, color.B));
        HeroGradient.Color = Color.FromArgb(0x26, color.R, color.G, color.B);
        GameNumber.Foreground = new SolidColorBrush(Color.FromArgb(0x55, color.R, color.G, color.B));
        UpdateSummary();
    }
    internal LauncherModel SelectedGameModel => model ?? throw new InvalidOperationException("No game selected.");
    internal void SelectGame(string gameId, bool persist = true)
    {
        if (launching || model is null || model.GameId == gameId) return;
        if (!games.TryGetValue(gameId, out var next)) throw new ArgumentOutOfRangeException(nameof(gameId));
        if (persist) model.Save();
        model = next;
        BindGame();
        Fm5Tab.IsChecked = gameId == "fm5";
    }
    private async void GameTab_Checked(object sender, RoutedEventArgs e)
    {
        if (model is null || sender is not RadioButton tab || tab.Tag is not string gameId || model.GameId == gameId) return;
        try { SelectGame(gameId); await RefreshBackend(); }
        catch (Exception ex) { StatusLabel.Text = ex.Message; }
    }
    internal async Task PreparePreview(int page)
    {
        await RefreshBackend();
        // Example paths belong only to the screenshot, never to saved settings.
        GamePath.Text = @"D:\Xbox One\Forza Motorsport 5";
        OutputPath.Text = @"C:\Games\Forza Motorsport 5";
        ShowPage(page);
    }
    private void UpdateSummary()
    {
        if (model is null) return;
        var settings = model.Values();
        var resolution = ResolutionSummary(settings["resolution"]);
        Summary.Text = $"{resolution}  /  {settings["frame_rate"]} FPS  /  MSAA {(settings["antialiasing"] == "off" ? "1" : settings["antialiasing"].Replace("msaax", ""))}×\n{model.Video[3].Selected.Label}  /  {model.Audio[0].Selected.Label}";
    }
    internal static string ResolutionSummary(string id) => id switch
    {
        "2160" => "4K", "2880" => "5K", "3240" => "6K", "3780" => "7K",
        "4320" => "8K", "1440" => "1440p", _ => "1080p"
    };
    private void Navigation_Checked(object sender, RoutedEventArgs e)
    {
        if (OverviewPage is null || sender is not RadioButton button || !int.TryParse(button.Tag?.ToString(), out var page)) return;
        ShowPage(page);
    }
    private void ShowPage(int page)
    {
        new[] { OverviewNav, VideoNav, AudioNav, CreditsNav }[page].IsChecked = true;
        OverviewPage.Visibility = page == 0 ? Visibility.Visible : Visibility.Collapsed;
        VideoPage.Visibility = page == 1 ? Visibility.Visible : Visibility.Collapsed;
        AudioPage.Visibility = page == 2 ? Visibility.Visible : Visibility.Collapsed;
        CreditsPage.Visibility = page == 3 ? Visibility.Visible : Visibility.Collapsed;
        Heading.Text = new[] { "Overview", "Video", "Audio & language", "Credits" }[page];
        Subtitle.Text = new[] { "", "", "", "" }[page];
    }
    private void Video_Click(object sender, RoutedEventArgs e) => ShowPage(1);
    private void PreGameSettings_Changed(object sender, RoutedEventArgs e)
    {
        if (model is null) return;
        var id = PreGameSettingsBox.IsChecked == true ? "false" : "true";
        if (model.BuilderOptions[0].Choices.FirstOrDefault(choice => choice.Id == id) is { } selection)
            model.BuilderOptions[0].Selected = selection;
        PreGameWarning.Visibility = id == "false" ? Visibility.Visible : Visibility.Collapsed;
    }
    private void DesktopShortcut_Changed(object sender, RoutedEventArgs e)
    {
        if (model is not null) model.CreateDesktopShortcut = DesktopShortcutBox.IsChecked == true;
    }
    private async void Build_Click(object sender, RoutedEventArgs e)
    {
        if (model is null) return;
        launching = true;
        GameBrowseButton.IsEnabled = OutputBrowseButton.IsEnabled = DesktopShortcutBox.IsEnabled = Fm5Tab.IsEnabled = false;
        BuildButton.IsEnabled = false; BusyBar.Visibility = Visibility.Visible;
        StatusLabel.Text = "Building EXE…";
        DispatcherTimer? progressTimer = null;
        try
        {
            Directory.CreateDirectory(model.DataDirectory);
            var id = DateTime.UtcNow.ToString("yyyyMMddTHHmmssfff") + "-" + Guid.NewGuid().ToString("N");
            var request = Path.Combine(model.DataDirectory, id + "-request.json");
            using var process = Process.Start(model.PrepareBuild(request, model.OutputFile)) ?? throw new IOException("Could not start the PC backend.");
            var progressPath = request + ".progress.json";
            progressTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(250) };
            progressTimer.Tick += (_, _) => {
                try
                {
                    if (!File.Exists(progressPath) || new FileInfo(progressPath).Length > 65536) return;
                    var progress = BuildProgress.Parse(File.ReadAllText(progressPath));
                    StatusLabel.Text = progress.Label;
                    BusyBar.IsIndeterminate = false;
                    BusyBar.Value = progress.Percent;
                }
                catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException or InvalidOperationException) { }
            };
            progressTimer.Start();
            var stdout = process.StandardOutput.ReadToEndAsync(); var stderr = process.StandardError.ReadToEndAsync();
            await process.WaitForExitAsync();
            var output = await stdout; var error = await stderr;
            progressTimer.Stop();
            await File.WriteAllTextAsync(Path.Combine(model.DataDirectory, id + "-backend.log"), output + "\n" + error);
            if (process.ExitCode != 0) throw new IOException(LauncherModel.DescribeBackendError(output, error, process.ExitCode));
            using var document = JsonDocument.Parse(output);
            var root = document.RootElement;
            if (!root.TryGetProperty("ok", out var ok) || ok.ValueKind != JsonValueKind.True || !root.TryGetProperty("output", out var built) || built.GetString() is not { } builtPath || !string.Equals(Path.GetFullPath(builtPath), Path.GetFullPath(model.OutputFile), StringComparison.OrdinalIgnoreCase) || !File.Exists(builtPath))
                throw new IOException("The backend did not confirm the EXE build.");
            StatusLabel.Text = root.TryGetProperty("warnings", out var warnings) && warnings.ValueKind == JsonValueKind.Array && warnings.GetArrayLength() > 0
                ? "Build completed. " + string.Join(" ", warnings.EnumerateArray().Select(warning => warning.GetString()))
                : "Build completed.";
        }
        catch (Exception ex) { StatusLabel.Text = ex.Message; }
        finally
        {
            launching = false; BusyBar.Visibility = Visibility.Collapsed;
            progressTimer?.Stop(); BusyBar.IsIndeterminate = true; BusyBar.Value = 0;
            GameBrowseButton.IsEnabled = OutputBrowseButton.IsEnabled = DesktopShortcutBox.IsEnabled = Fm5Tab.IsEnabled = true;
            await RefreshBackend(false);
        }
    }
    private async void BrowseGame_Click(object sender, RoutedEventArgs e)
    {
        if (model is null || launching) return;
        var picker = new Microsoft.Win32.OpenFolderDialog { Title = "Choose your extracted " + model.GameTitle + " folder", Multiselect = false };
        if (Directory.Exists(model.GameFolder)) picker.InitialDirectory = model.GameFolder;
        if (picker.ShowDialog(this) != true) return;
        model.GameFolder = picker.FolderName;
        GamePath.Text = model.GameFolder;
        BuildButton.IsEnabled = false;
        StatusLabel.Text = "Checking selected game files…";
        await RefreshBackend();
    }
    private void BrowseOutput_Click(object sender, RoutedEventArgs e)
    {
        if (model is null || launching) return;
        var picker = new Microsoft.Win32.OpenFolderDialog { Title = "Choose a parent folder for " + model.GameTitle, Multiselect = false };
        if (Directory.Exists(model.OutputDirectory)) picker.InitialDirectory = model.OutputDirectory;
        if (picker.ShowDialog(this) == true) { model.SelectOutputParent(picker.FolderName); OutputPath.Text = model.OutputDirectory; }
    }
    private void OpenLogs_Click(object sender, RoutedEventArgs e)
    {
        if (model is null) return;
        var path = model.DataDirectory;
        Directory.CreateDirectory(path); Open(path);
    }
    private void License_Click(object sender, RoutedEventArgs e) { if (model is not null) Open(Path.Combine(model.ProjectRoot, "launcher", "THIRD-PARTY-NOTICES.txt")); }
    private void Donate_Click(object sender, RoutedEventArgs e)
    {
        try { Process.Start(ProjectLinks.DonationRequest()); }
        catch (Exception ex) { StatusLabel.Text = "Could not open the donation page: " + ex.Message; }
    }
    private void Reference_Navigate(object sender, System.Windows.Navigation.RequestNavigateEventArgs e) { Open(e.Uri.AbsoluteUri); e.Handled = true; }
    private void Open(string path)
    { try { Process.Start(new ProcessStartInfo(path) { UseShellExecute = true }); } catch (Exception ex) { StatusLabel.Text = ex.Message; } }
    private void Minimize_Click(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void Maximize_Click(object sender, RoutedEventArgs e) => WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
    private void Close_Click(object sender, RoutedEventArgs e) { try { foreach (var game in games.Values) game.Save(); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { } Close(); }
}

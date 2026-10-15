using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.IO;

namespace ForzaMastro.Launcher;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        if (e.Args.Length == 2 && e.Args[0] == "--self-test")
        {
            try { SelfTests.Run(e.Args[1]); Shutdown(0); }
            catch (Exception ex) { File.WriteAllText(e.Args[1], ex.ToString()); Shutdown(1); }
            return;
        }
        if (e.Args.Length == 2 && e.Args[0] == "--check-backend")
        {
            try
            {
                Task.Run(async () =>
                {
                    var model = new LauncherModel(LauncherModel.FindProject(AppContext.BaseDirectory));
                    await model.RefreshAsync();
                    File.WriteAllText(e.Args[1], System.Text.Json.JsonSerializer.Serialize(new { passed = true, capabilities = model.Backend, canBuild = model.CanBuild, options = model.Values(), optionCounts = model.All.ToDictionary(o => o.Key, o => o.Choices.Length), scope = "Read-only capabilities; no settings applied or game launched." }, LauncherModel.Json));
                }).GetAwaiter().GetResult();
                Shutdown(0);
            }
            catch (Exception ex) { File.WriteAllText(e.Args[1], ex.ToString()); Shutdown(1); }
            return;
        }
        var preview = e.Args.Length == 2 && (e.Args[0] == "--render-preview" || e.Args[0] == "--render-video" || e.Args[0] == "--render-audio" || e.Args[0] == "--render-credits");
        var window = new MainWindow(preview);
        MainWindow = window;
        if (preview)
        {
            Dispatcher.InvokeAsync(async () =>
            {
                await window.PreparePreview(e.Args[0] == "--render-video" ? 1 : e.Args[0] == "--render-audio" ? 2 : e.Args[0] == "--render-credits" ? 3 : 0);
                var visual = (FrameworkElement)window.Content;
                visual.Measure(new Size(window.Width, window.Height));
                visual.Arrange(new Rect(0, 0, window.Width, window.Height));
                visual.UpdateLayout();
                var bitmap = new RenderTargetBitmap((int)window.Width, (int)window.Height, 96, 96, PixelFormats.Pbgra32);
                bitmap.Render(visual);
                var encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create(bitmap));
                using var stream = File.Create(e.Args[1]);
                encoder.Save(stream);
                Shutdown();
            }, System.Windows.Threading.DispatcherPriority.ApplicationIdle);
        }
        else window.Show();
    }
}

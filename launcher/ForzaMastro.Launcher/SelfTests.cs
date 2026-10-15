using System.IO;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
namespace ForzaMastro.Launcher;
internal static class SelfTests
{
 public static void Run(string report)
 {
  int checks=0;
  void Check(bool ok,string name) { if(!ok) throw new Exception(name); checks++; }
  var temp=Path.Combine(Path.GetTempPath(),"Race One Hub Tests "+Guid.NewGuid().ToString("N"));
  Directory.CreateDirectory(Path.Combine(temp,"launcher","backend"));Directory.CreateDirectory(Path.Combine(temp,"runtime","pc"));
  Directory.CreateDirectory(Path.Combine(temp,"tools"));File.WriteAllText(Path.Combine(temp,"tools","pc_launcher_backend.py"),"# Fixture root marker; never executed");
  try {
   var model=new LauncherModel(temp);
   Check(!model.CanBuild,"Missing backend cannot launch");
   Check(model.Video.Length+model.Audio.Length==8 && model.All.Count()==9,"Eight game options plus independent builder option");
   Check(model.Values()["graphics"]=="default","Original preset default");
   Check(model.Values()["vsync"]=="true","VSync on");
   Check(model.Values()["audio"]=="multichannel","Multichannel audio");
   Check(model.Values()["language"]=="it-IT","Italian");
   Check(model.GameFolder=="" && !model.CreateDesktopShortcut,"Fresh builder never selects runtime game or creates shortcut by default");
   Check(model.Video[1].Choices.Select(c=>c.Id).SequenceEqual(new[]{"30","60","120"}),"FPS targets in ascending order");
   Check(model.Video[1].Selected.Id=="60","FPS display order preserves existing default");
   Check(model.Video[2].Choices.Select(c=>c.Id).SequenceEqual(new[]{"off","msaax2","msaax4"}),"Only 1x 2x 4x MSAA choices");
   Check(model.Video[0].Choices.Select(c=>c.Id).Order().SequenceEqual(new[]{"1080","1440","2160","2880","3240","3780","4320"}),"Full HD through8K height-valued resolution catalog");
   Check(!model.SkipPreGame && model.Values()["show_pre_game"]=="true","Unchecked skip keeps native pre-game enabled");
   Check(LauncherModel.FindProject(Path.Combine(temp,"runtime","pc"))==temp,"Root discovery");
   model.Save();Check(File.Exists(Path.Combine(temp,"launcher","data","settings.json")),"Settings local");
   var supported=model.All.ToDictionary(o=>o.Key,o=>o.Choices.Select(c=>c.Id).ToArray());
   string Capabilities(bool ready,bool running,string game="fm5")=>JsonSerializer.Serialize(new{schema_version=1,game_id=game,ready,running,build_ready=ready,output_filename="RaceOne5.exe",current=new{},supported,warnings=new[]{"120 FPS experimental"}});
   model.ApplyCapabilities(Capabilities(true,false));Check(!model.CanBuild,"Missing runtime rejected");
   File.WriteAllBytes(model.RuntimePath,[]);File.WriteAllBytes(model.PackagedBackend,[]);
   Check(!model.CanBuild && model.GameFolder=="","Backend readiness never auto-selects existing game data");
   var gameFolder=Path.Combine(temp,"User game files");Directory.CreateDirectory(Path.Combine(gameFolder,"Mount"));
   model.GameFolder=Path.Combine(gameFolder,"Mount");
   Check(model.GameFolder==gameFolder && model.Backend is null,"Selecting Mount normalizes parent and invalidates old capabilities");
   model.ApplyCapabilities(Capabilities(true,false));
   Check(model.CanBuild,"Ready supported runtime and chosen source enable build");
   var capCommand=model.CapabilitiesCommand();
   Check(capCommand.ArgumentList.SequenceEqual(new[]{"--game","fm5","--capabilities","--game-folder",gameFolder}),"Capabilities refresh uses chosen game folder as one argument");
   var request=Path.Combine(temp,"request with spaces.json");var launch=model.PrepareBuild(request,model.OutputFile);
   Check(!launch.UseShellExecute,"No shell expansion");Check(launch.ArgumentList[3]==request,"Spaces one argument");Check(launch.WorkingDirectory==temp,"Working directory exact");
   using var payload=JsonDocument.Parse(File.ReadAllText(request));
   Check(payload.RootElement.GetProperty("resolution").GetInt32()==2160,"Resolution numeric");Check(payload.RootElement.GetProperty("frame_rate").GetInt32()==60,"FPS numeric");
   Check(payload.RootElement.GetProperty("vsync").GetBoolean(),"VSync boolean");Check(payload.RootElement.GetProperty("antialiasing").GetString()=="msaax2","Exact MSAA ID");Check(payload.RootElement.GetProperty("audio").GetString()=="multichannel","Exact audio ID");
   foreach(var aa in new[]{"off","msaax2","msaax4"})
   {
    model.Video[2].Selected=model.Video[2].Choices.First(c=>c.Id==aa);
    model.PrepareBuild(request,model.OutputFile);
    using var aaRequest=JsonDocument.Parse(File.ReadAllText(request));
    Check(aaRequest.RootElement.GetProperty("antialiasing").GetString()==aa,"Supported sample request exact ID "+aa);
    Check(aaRequest.RootElement.EnumerateObject().Count()==9,"MSAA choice preserves nine-setting request "+aa);
   }
   model.Video[2].Selected=model.Video[2].Choices.First(c=>c.Id=="msaax2");
   var highResolutions=new[]{("3240","6K (5760x3240)","6K"),("3780","7K (6720x3780)","7K"),("4320","8K (7680x4320)","8K")};
   foreach(var (height,label,summary) in highResolutions)
   {
    var option=model.Video[0].Choices.Single(c=>c.Id==height);
    Check(option.Label==label,"Exact16:9 resolution label "+summary);
    model.Video[0].Selected=option;
    model.PrepareBuild(request,model.OutputFile);
    using var highRequest=JsonDocument.Parse(File.ReadAllText(request));
    Check(highRequest.RootElement.GetProperty("resolution").GetInt32()==int.Parse(height),"Numeric resolution request "+summary);
    Check(highRequest.RootElement.EnumerateObject().Count()==9,"High-resolution request retains exact protocol shape "+summary);
    Check(MainWindow.ResolutionSummary(height)==summary,"Overview summary "+summary);
    Check(new LauncherModel(temp).Values()["resolution"]==height,"Resolution survives settings reload "+summary);
   }
   // Instantiate the actual option template without showing a window or calling
   // a backend. Verify bindings and selection for every catalog item.
   var templateWindow=new MainWindow(previewOnly:true,projectRoot:temp);
   var template=(DataTemplate)templateWindow.Resources["OptionTemplate"];
   var optionVisual=(FrameworkElement)template.LoadContent();
   optionVisual.DataContext=model.Video[0];
   optionVisual.Measure(new Size(780,160));optionVisual.Arrange(new Rect(0,0,780,160));optionVisual.UpdateLayout();
   T? Find<T>(DependencyObject parent) where T:DependencyObject
   {
    if(parent is T found)return found;
    for(var i=0;i<VisualTreeHelper.GetChildrenCount(parent);i++)if(Find<T>(VisualTreeHelper.GetChild(parent,i)) is {} match)return match;
    return null;
   }
   var resolutionCombo=Find<ComboBox>(optionVisual)??throw new Exception("Resolution template ComboBox missing");
   Check(resolutionCombo.Items.Count==7,"Actual WPF selector contains all7 resolution options");
   foreach(var choice in model.Video[0].Choices)
   {
    resolutionCombo.SelectedItem=choice;
    optionVisual.UpdateLayout();
    Check(model.Video[0].Selected.Id==choice.Id,"WPF selection binding "+choice.Id);
   }
   var aaVisual=(FrameworkElement)template.LoadContent();aaVisual.DataContext=model.Video[2];
   aaVisual.Measure(new Size(780,160));aaVisual.Arrange(new Rect(0,0,780,160));aaVisual.UpdateLayout();
   var aaCombo=Find<ComboBox>(aaVisual)??throw new Exception("MSAA template ComboBox missing");
   Check(aaCombo.Items.Count==3,"Actual WPF MSAA selector contains exactly three supported choices");
   foreach(var choice in model.Video[2].Choices){aaCombo.SelectedItem=choice;aaVisual.UpdateLayout();Check(model.Video[2].Selected.Id==choice.Id,"MSAA WPF selection binding "+choice.Id);}
   templateWindow.SelectedGameModel.Video[2].Selected=templateWindow.SelectedGameModel.Video[2].Choices.Single(c=>c.Id=="off");
   Check(templateWindow.Summary.Text.Contains("MSAA 1\u00d7",StringComparison.Ordinal),"Disabled MSAA overview shows1x, notoffx");
   model.Video[2].Selected=model.Video[2].Choices.Single(c=>c.Id=="msaax2");
   var resolutionBitmap=new RenderTargetBitmap(780,160,96,96,PixelFormats.Pbgra32);
   resolutionBitmap.Render(optionVisual);
   var resolutionEncoder=new PngBitmapEncoder();resolutionEncoder.Frames.Add(BitmapFrame.Create(resolutionBitmap));
   using(var preview=File.Create(Path.ChangeExtension(report,".resolution-options.png")))resolutionEncoder.Save(preview);
   model.Video[0].Selected=model.Video[0].Choices.First(c=>c.Id=="2160");
   model.ApplyCapabilities(Capabilities(true,true));Check(!model.CanBuild,"Running game blocks build consistently with backend");model.ApplyCapabilities(Capabilities(false,false));Check(!model.CanBuild,"Unready rejected");
   try{model.ApplyCapabilities(Capabilities(true,false,"fh2"));throw new Exception("Wrong game accepted");}catch(InvalidDataException){checks++;}
   model=new LauncherModel(temp);supported["frame_rate"]=["60"];model.ApplyCapabilities(Capabilities(true,false));Check(model.Video[1].Choices.Length==1,"Unsupported filtered");
   supported["frame_rate"]=["60","120"]; model.ApplyCapabilities(Capabilities(true,false)); Check(model.Video[1].Choices.Length==2,"Restored capabilities restore choices");
   supported["frame_rate"]=["60"]; model.ApplyCapabilities(Capabilities(true,false));
   model.Video[1].Selected=new("120","120 FPS");try{model.PrepareBuild(request,model.OutputFile);throw new Exception("Unsupported accepted");}catch(InvalidDataException){checks++;}
   Check(!Directory.EnumerateFiles(Path.Combine(temp,"runtime","pc")).Any(p=>p.EndsWith(".json",StringComparison.Ordinal)),"No runtime settings writes");
   var freshRoot=Path.Combine(temp,"fresh");
   var fresh=new LauncherModel(freshRoot);
   var current=new Dictionary<string,object>{{"frame_rate",120},{"graphics","ultra"},{"resolution",2880},{"vsync",false}};
   string FreshCapabilities()=>JsonSerializer.Serialize(new{schema_version=1,game_id="fm5",ready=true,running=false,build_ready=true,output_filename="RaceOne5.exe",current,supported,preset_status=new{ultra=""}});
   supported["frame_rate"]=["60","120"];
   fresh.ApplyCapabilities(FreshCapabilities());
   Check(fresh.Values()["frame_rate"]=="120","First startup reflects backend FPS");
   Check(fresh.Values()["graphics"]=="ultra","First startup reflects backend preset");
   Check(fresh.Video[3].Availability=="","Ultra has no extra status text");
   Check(fresh.Values()["resolution"]=="2880" && fresh.Values()["vsync"]=="false","Initial numeric and boolean current values");
   fresh.Video[1].Selected=fresh.Video[1].Choices.First(c=>c.Id=="60");
   fresh.ApplyCapabilities(FreshCapabilities());
   Check(fresh.Values()["frame_rate"]=="60","Refresh preserves unsaved user edit");
   fresh.OutputDirectory=Path.Combine(temp,"Custom output folder");fresh.GameFolder=gameFolder;fresh.CreateDesktopShortcut=true;fresh.Save();
   var reload=new LauncherModel(freshRoot);reload.ApplyCapabilities(FreshCapabilities());
   Check(reload.Values()["frame_rate"]=="60","Saved user setting overrides backend current");
   Check(reload.OutputDirectory==fresh.OutputDirectory,"Output folder preference persists");
   Check(reload.GameFolder==gameFolder && reload.CreateDesktopShortcut,"Source and optional shortcut preferences persist");
   Check(LauncherModel.DescribeBackendError("{\"ok\":false,\"error\":\"Exact profile error\"}","fallback",2)=="Exact profile error","JSON error preferred");
   Check(LauncherModel.DescribeBackendError("not JSON","fallback",2)=="fallback","Stderr fallback");
   Check(LauncherModel.DescribeBackendError("[]","",7).Contains("7"),"Malformed error shape fallback");
   var donation=ProjectLinks.DonationRequest();
   Check(donation.FileName=="https://ko-fi.com/lozazamastro","Original builder donation destination");
   Check(donation.UseShellExecute,"Donation uses default browser");
   Check(donation.ArgumentList.Count==0 && donation.Arguments=="","Donation has no extra arguments");
   Check(launch.ArgumentList.Contains("--build") && !launch.ArgumentList.Contains("--launch"),"Build command never launches");
   string Argument(System.Diagnostics.ProcessStartInfo command,string key)=>command.ArgumentList[command.ArgumentList.IndexOf(key)+1];
   Check(Argument(launch,"--output")==model.OutputFile,"Fixed output path passed separately");
   Check(Argument(launch,"--game-folder")==gameFolder,"Game folder mandatory in build command");
   Check(Argument(launch,"--progress-file")==request+".progress.json","Real byte progress file passed separately");
   Check(!launch.ArgumentList.Contains("--desktop-shortcut"),"Unchecked Desktop option omits flag");
   Check(LauncherModel.GeneratedExecutableName=="RaceOne5.exe","Exact generated executable name");
   Check(model.OutputFile.EndsWith("RaceOne5.exe",StringComparison.Ordinal),"Race One game output branding");
   model.Video[1].Selected=model.Video[1].Choices.First(c=>c.Id=="60");
   try{model.PrepareBuild(request,Path.Combine(temp,"WrongName.exe"));throw new Exception("Wrong output accepted");}catch(InvalidDataException){checks++;}
   var pending=new LauncherModel(temp);
   pending.ApplyCapabilities(JsonSerializer.Serialize(new{schema_version=1,game_id="fm5",ready=true,running=false,supported}));
   Check(!pending.CanBuild,"Launch-only backend never counts as a builder");
   try{pending.ApplyCapabilities(JsonSerializer.Serialize(new{schema_version=1,game_id="fm5",build_ready=true,output_filename="WrongTitle.exe",supported}));throw new Exception("Wrong backend output accepted");}catch(InvalidDataException){checks++;}
   var languageRoot=Path.Combine(temp,"languages");
   Directory.CreateDirectory(Path.Combine(languageRoot,"runtime","pc"));Directory.CreateDirectory(Path.Combine(languageRoot,"launcher","backend"));
   var languages=new LauncherModel(languageRoot);File.WriteAllBytes(languages.RuntimePath,[]);File.WriteAllBytes(languages.PackagedBackend,[]);
   languages.GameFolder=gameFolder;
   var languageSupport=languages.All.ToDictionary(o=>o.Key,o=>o.Choices.Select(c=>c.Id).ToArray());
   languageSupport["language"]=["fr-FR","de-DE"];
   var labels=new Dictionary<string,string>{{"fr-FR","French"},{"de-DE","German"}};
   string LanguageCaps()=>JsonSerializer.Serialize(new{schema_version=1,game_id="fm5",build_ready=true,output_filename="RaceOne5.exe",current=new{language="fr-FR"},supported=languageSupport,language_labels=labels});
   languages.ApplyCapabilities(LanguageCaps());
   Check(languages.Audio.Length==2 && languages.Audio[0].Label=="Language" && languages.Audio[1].Key=="audio","Single game-language control");
   Check(languages.Values()["language"]=="fr-FR" && !languages.Values().ContainsKey("speech"),"Single language payload");
   Check(languages.Audio[0].Choices.Length==2 && languages.Audio[0].Choices.All(c=>c.Id!="system"),"Only explicit supported languages");
   languages.Audio[0].Selected=languages.Audio[0].Choices.First(c=>c.Id=="de-DE");
   languages.ApplyCapabilities(LanguageCaps());
   Check(languages.Values()["language"]=="de-DE","Refresh preserves game-language selection");
   languages.Save();var languageReload=new LauncherModel(languageRoot);languageReload.ApplyCapabilities(LanguageCaps());
   Check(languageReload.Values()["language"]=="de-DE","Language survives saved-settings reload");
   var languageRequest=Path.Combine(temp,"language-request.json");languageReload.PrepareBuild(languageRequest,languageReload.OutputFile);
   using(var parsed=JsonDocument.Parse(File.ReadAllText(languageRequest)))
   {
    Check(parsed.RootElement.EnumerateObject().Count()==9,"Nine-field build request");
    Check(parsed.RootElement.GetProperty("language").GetString()=="de-DE" && !parsed.RootElement.TryGetProperty("speech",out _),"No independent speech override");
   }
   var oldSettings=languages.Values();oldSettings["speech"]="system";oldSettings["language"]="system";
   File.WriteAllText(Path.Combine(languages.DataDirectory,"settings.json"),JsonSerializer.Serialize(oldSettings));
   var legacySettings=new LauncherModel(languageRoot);legacySettings.ApplyCapabilities(LanguageCaps());
   Check(legacySettings.Audio[0].Choices.Any(c=>c.Id==legacySettings.Values()["language"]),"Old System locale migrates to explicit available language");
   Check(!legacySettings.Values().ContainsKey("speech"),"Retired voice setting omitted");
   Check(new LauncherModel(Path.Combine(temp,"pre-game-default")).Values()["show_pre_game"]=="true","Pre-game settings default on");
   languageReload.SkipPreGame=true;
   languageReload.CreateDesktopShortcut=true;
   var shortcutBuild=languageReload.PrepareBuild(languageRequest,languageReload.OutputFile);
   Check(shortcutBuild.ArgumentList.Count(arg=>arg=="--desktop-shortcut")==1,"Checked Desktop option passes exactly one flag");
   using(var parsedPreGame=JsonDocument.Parse(File.ReadAllText(languageRequest)))
    Check(parsedPreGame.RootElement.GetProperty("show_pre_game").ValueKind==JsonValueKind.False,"Pre-game toggle serialized as boolean");
   var reloadedPreGame=new LauncherModel(languageRoot);reloadedPreGame.ApplyCapabilities(LanguageCaps());
   Check(reloadedPreGame.Values()["show_pre_game"]=="false","Pre-game preference persists independently");
   Check(reloadedPreGame.SkipPreGame,"Saved internal false reflects checked Skip control");
   reloadedPreGame.SkipPreGame=false;
   Check(reloadedPreGame.Values()["show_pre_game"]=="true","Unchecking Skip re-enables pre-game");
   languageSupport.Remove("show_pre_game");var missingPreGame=new LauncherModel(languageRoot);missingPreGame.ApplyCapabilities(LanguageCaps());
   Check(!missingPreGame.CanBuild,"Missing pre-game capability blocks unsupported generation");
   Check(missingPreGame.Status.Contains("prepared",StringComparison.Ordinal),"Missing setting capability is visibly pending");
   var missingSource=new LauncherModel(Path.Combine(temp,"missing-source"));
   missingSource.ApplyCapabilities(Capabilities(true,false));
   try{missingSource.PrepareBuild(Path.Combine(temp,"must-not-exist.json"),missingSource.OutputFile);throw new Exception("Missing source accepted");}catch(InvalidOperationException){checks++;}
   Check(!File.Exists(Path.Combine(temp,"must-not-exist.json")),"Missing source rejected before request writes");
   var otherSource=Path.Combine(temp,"Other game files");Directory.CreateDirectory(otherSource);
   languageSupport["show_pre_game"]=["true","false"];
   languageSupport["language"]=["cs-CZ","hu-HU","pl-PL"];
   languages.GameFolder=otherSource;
   Check(languages.Backend is null && !languages.CanBuild,"New source cannot build with stale language catalog");
   languages.ApplyCapabilities(JsonSerializer.Serialize(new{schema_version=1,game_id="fm5",ready=true,build_ready=true,output_filename="RaceOne5.exe",supported=languageSupport,
    language_labels=new Dictionary<string,string>{{"cs-CZ","Czech"},{"hu-HU","Hungarian"},{"pl-PL","Polish"}}}));
   Check(languages.Audio[0].Choices.Select(c=>c.Id).Order().SequenceEqual(languageSupport["language"].Order()),"Selected-source catalog exposes Czech Hungarian Polish without old languages");
   Check(languages.Values()["language"]=="cs-CZ","Missing old selection falls back to explicit available language");
   foreach(var fps in new[]{"30","60","120"})
   {
    languages.Video[1].Selected=languages.Video[1].Choices.First(choice=>choice.Id==fps);
    languages.PrepareBuild(languageRequest,languages.OutputFile);
    using var fpsRequest=JsonDocument.Parse(File.ReadAllText(languageRequest));
    Check(fpsRequest.RootElement.GetProperty("frame_rate").GetInt32()==int.Parse(fps),"Typed FPS request "+fps);
    Check(fpsRequest.RootElement.EnumerateObject().Count()==9,"Source and shortcut preferences stay outside nine game settings");
   }
   var progress=BuildProgress.Parse("{\"phase\":\"preparing_scene_overrides\",\"bytes_copied\":21474836480,\"total_bytes\":42949672960}");
   Check(progress.Percent==50 && progress.Label.Contains("50%",StringComparison.Ordinal),"Scene preparation progress uses 64-bit byte counts");
   foreach(var bad in new[]{"[]","{}","{\"phase\":\"preparing_scene_overrides\",\"bytes_copied\":-1,\"total_bytes\":10}","{\"phase\":\"preparing_scene_overrides\",\"bytes_copied\":11,\"total_bytes\":10}","{\"phase\":\"preparing_scene_overrides\",\"bytes_copied\":0,\"total_bytes\":0}"})
    try{BuildProgress.Parse(bad);throw new Exception("Bad progress accepted");}catch(InvalidDataException){checks++;}
   var migrationRoot=Path.Combine(temp,"brand-migration");
   Directory.CreateDirectory(Path.Combine(migrationRoot,"launcher","data"));
   File.WriteAllText(Path.Combine(migrationRoot,"launcher","data","builder-state.json"),JsonSerializer.Serialize(new{outputDirectory=Path.Combine(migrationRoot,"launcher","output","ForzaMastro5")}));
   var migrated=new LauncherModel(migrationRoot);
   Check(migrated.OutputDirectory==Path.Combine(migrationRoot,"launcher","output","RaceOne5"),"Previous default output directory migrates to new product name");
   foreach(var gameId in new[]{"fm5"})
   {
    var legacyRoot=Path.Combine(temp,"legacy-aa-"+gameId);var legacyModel=new LauncherModel(legacyRoot,gameId);
    Directory.CreateDirectory(legacyModel.DataDirectory);
    var legacyValues=legacyModel.Values();legacyValues["antialiasing"]="msaax8";
    File.WriteAllText(Path.Combine(legacyModel.DataDirectory,"settings.json"),JsonSerializer.Serialize(legacyValues));
    var migratedAa=new LauncherModel(legacyRoot,gameId);
    Check(migratedAa.Values()["antialiasing"]=="msaax4","Saved8x migrates to4x "+gameId);
    migratedAa.Save();
    var persisted=JsonSerializer.Deserialize<Dictionary<string,string>>(File.ReadAllText(Path.Combine(migratedAa.DataDirectory,"settings.json")))!;
    Check(persisted["antialiasing"]=="msaax4" && persisted.Count==9,"Migrated nine values persist "+gameId);
    Check(!migratedAa.Video[2].Choices.Any(c=>c.Id=="msaax8"),"Eight samples absent "+gameId);
    var aaSupport=migratedAa.All.ToDictionary(o=>o.Key,o=>o.Choices.Select(c=>c.Id).ToArray());
    var currentAa=new LauncherModel(Path.Combine(temp,"current-aa-"+gameId),gameId);
    currentAa.ApplyCapabilities(JsonSerializer.Serialize(new{schema_version=1,game_id=gameId,ready=true,build_ready=true,output_filename=currentAa.OutputExecutableName,current=new{antialiasing="msaax8"},supported=aaSupport}));
    Check(currentAa.Values()["antialiasing"]=="msaax4","Backend old8x initializes4x "+gameId);
    foreach(var aa in new[]{"off","msaax2","msaax4"})
    {
     migratedAa.Video[2].Selected=migratedAa.Video[2].Choices.Single(c=>c.Id==aa);var values=migratedAa.Values();migratedAa.Save();
     var restored=new LauncherModel(legacyRoot,gameId);
     Check(restored.Values().OrderBy(p=>p.Key).SequenceEqual(values.OrderBy(p=>p.Key)) && restored.Values().Count==9,"Every sample choice roundtrips all nine preferences "+gameId+" "+aa);
    }
   }
   var tabWindow=new MainWindow(previewOnly:true,projectRoot:temp);
   Check(tabWindow.SelectedGameModel.GameId=="fm5" && tabWindow.Fm5Tab.IsChecked==true,"Only supported game is selected");
   Check(tabWindow.GameTitle.Text=="Forza Motorsport 5" && tabWindow.GameNumber.Text=="5","Exact game branding");
   Check(((SolidColorBrush)tabWindow.Resources["Accent"]).Color==(Color)ColorConverter.ConvertFromString("#FF8A44"),"Exact FM5 accent");
   Check(ReferenceEquals(tabWindow.VideoOptions.ItemsSource,tabWindow.SelectedGameModel.Video),"Selected video model binding");
   var visual=(FrameworkElement)tabWindow.Content;
   visual.Measure(new Size(tabWindow.Width,tabWindow.Height));visual.Arrange(new Rect(0,0,tabWindow.Width,tabWindow.Height));visual.UpdateLayout();
   Check(tabWindow.Fm5Tab.ActualWidth>100 && tabWindow.GameTitle.ActualWidth>200,"Single game tab and title fit");
   Check(!tabWindow.BuildButton.IsEnabled,"No prepared runtime never presents enabled Build");
   var bitmap=new RenderTargetBitmap((int)tabWindow.Width,(int)tabWindow.Height,96,96,PixelFormats.Pbgra32);bitmap.Render(visual);
   var encoder=new PngBitmapEncoder();encoder.Frames.Add(BitmapFrame.Create(bitmap));
   using(var image=File.Create(Path.ChangeExtension(report,".overview.png")))encoder.Save(image);
   tabWindow.Close();
   File.WriteAllText(report,JsonSerializer.Serialize(new{passed=true,checks,scope="Builder model, JSON protocol and offscreen WPF option-template binding; no executable generated and no game or hardware launched."},LauncherModel.Json));
  }finally{Directory.Delete(temp,true);}
 }
}

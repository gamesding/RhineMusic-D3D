using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;

namespace Rhine.Configuration {
    internal static class JsonData {
        internal static readonly JavaScriptSerializer Serializer = new JavaScriptSerializer { MaxJsonLength = 1024 * 1024, RecursionLimit = 48 };
        internal static Dictionary<string, object> Object(object value) { return value as Dictionary<string, object> ?? new Dictionary<string, object>(); }
        internal static object Value(Dictionary<string, object> value, string key, object fallback) { object result; return value.TryGetValue(key, out result) ? result : fallback; }
        internal static string Text(Dictionary<string, object> value, string key, string fallback = "") { return Value(value, key, fallback) as string ?? fallback; }
        internal static bool Flag(Dictionary<string, object> value, string key) { object result; return value.TryGetValue(key, out result) && result is bool && (bool)result; }
        internal static double Number(Dictionary<string, object> value, string key, double fallback = 0) { object result; if (!value.TryGetValue(key, out result) || result is string || result is bool || result == null) return fallback; try { double n = Convert.ToDouble(result, CultureInfo.InvariantCulture); return Double.IsNaN(n) || Double.IsInfinity(n) ? fallback : n; } catch { return fallback; } }
        internal static Dictionary<string, object> Read(string file) { if (!File.Exists(file)) return new Dictionary<string, object>(); if (new FileInfo(file).Length > 1024 * 1024) throw new InvalidDataException("配置文件过大，未作修改。"); var text = File.ReadAllText(file, new UTF8Encoding(false, true)); var result = Serializer.DeserializeObject(text) as Dictionary<string, object>; if (result == null) throw new InvalidDataException("配置文件内容损坏，请先备份并检查 preferences.json。"); return result; }
        internal static void WriteAtomic(string file, Dictionary<string, object> value) {
            string directory = Path.GetDirectoryName(Path.GetFullPath(file)); Directory.CreateDirectory(directory);
            string temporary = Path.Combine(directory, ".configuration-" + Guid.NewGuid().ToString("N") + ".tmp");
            try {
                byte[] bytes = new UTF8Encoding(false).GetBytes(Serializer.Serialize(value));
                using (var output = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096, FileOptions.WriteThrough)) { output.Write(bytes, 0, bytes.Length); output.Flush(true); }
                if (File.Exists(file)) File.Replace(temporary, file, null); else File.Move(temporary, file);
            } finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }
    }
    internal sealed class Adapter {
        internal string Name, Identity;
        internal uint Vendor;
        internal double VideoMemory;
        internal bool Dx11, Dx12, Software, HardwareRays, Spectral, Dlss;
        internal static Adapter From(Dictionary<string, object> v) {
            var result = new Adapter { Name = JsonData.Text(v, "name", "未命名显卡"), Identity = JsonData.Text(v, "luid"), Vendor = (uint)JsonData.Number(v, "vendorId"), VideoMemory = JsonData.Number(v, "dedicatedBytes"), Dx11 = JsonData.Flag(v, "d3d11"), Dx12 = JsonData.Flag(v, "d3d12"), Software = JsonData.Flag(v, "software"), HardwareRays = JsonData.Number(v, "raytracingTier") > 0 };
            result.Spectral = !result.Software && result.Vendor == 0x10de && result.Dx12 && JsonData.Flag(v, "inlineRaytracing") && JsonData.Flag(v, "spectralCausticsEligible");
            result.Dlss = !result.Software && result.Vendor == 0x10de && result.Dx11 && JsonData.Flag(v, "dlssEligible"); return result;
        }
        internal double Score { get { return !Dx11 ? -1000 : Software ? -100 : 100 + Math.Min(VideoMemory / 1073741824.0, 32) * 10 + (Dx12 ? 12 : 0) + (Spectral ? 8 : 0); } }
        public override string ToString() { return Name; }
    }
    internal sealed class Machine {
        internal List<Adapter> Adapters = new List<Adapter>();
        internal double Memory;
        internal int Processors;
        internal bool Battery, Detected;
        internal string Error = "";
        internal Adapter Best { get { return Adapters.OrderByDescending(x => x.Score).First(); } }
        internal static Machine From(Dictionary<string, object> value) {
            var result = new Machine { Memory = JsonData.Number(value, "memoryBytes"), Processors = (int)JsonData.Number(value, "logicalProcessors", 2), Battery = JsonData.Flag(value, "onBattery"), Detected = JsonData.Flag(value, "ok") };
            var adapters = JsonData.Value(value, "adapters", new object[0]) as IEnumerable;
            if (adapters != null) foreach (object item in adapters) { var a = Adapter.From(JsonData.Object(item)); if (a.Dx11) result.Adapters.Add(a); }
            if (result.Adapters.Count == 0) return Fallback("未检测到可用的硬件渲染设备，已提供软件兼容模式。"); return result;
        }
        internal static Machine Fallback(string error) { return new Machine { Memory = 4 * 1073741824.0, Processors = 2, Detected = false, Error = error, Adapters = new List<Adapter> { new Adapter { Name = "DX11 软件兼容模式", Identity = "", Dx11 = true, Software = true } } }; }
    }
    internal sealed class Profile {
        internal string Mode = "automatic", Api = "dx11", Upscaler = "native", UpscaleQuality = "quality", Reflections = "off", ReflectionQuality = "balanced", Preset = "original";
        internal Adapter Adapter;
        internal Dictionary<string, object> Quality = QualityPreset("original");
        internal string Reason = "";
        internal double AutomaticNativeScale = 100;
        internal static Dictionary<string, object> QualityPreset(string preset) {
            bool low = preset == "performance", high = preset == "high", ultra = preset == "ultra";
            return new Dictionary<string, object> { { "scale", low ? 80 : high ? 125 : ultra ? 150 : 100 }, { "pixelRatio", low ? 1.0 : high || ultra ? 2.0 : 1.5 }, { "antialias", high || ultra ? "smaa" : "off" }, { "shadows", low ? 1024 : high || ultra ? 4096 : 2048 }, { "aoSamples", low ? 0 : ultra ? 64 : 32 }, { "aoResolution", low ? .5 : 1 }, { "depthOfField", low ? 0 : 100 }, { "transmission", low ? .5 : 1 }, { "anisotropy", low ? 4 : 16 } };
        }
        internal static Profile Recommend(Machine machine) {
            Adapter a = machine.Best; double gb = a.VideoMemory / 1073741824.0, ram = machine.Memory / 1073741824.0;
            bool low = a.Software || gb < 2 || ram < 6 || machine.Processors < 4 || machine.Battery;
            bool strong = !low && gb >= 6 && ram >= 8;
            var p = new Profile { Adapter = a, Api = !low && a.Dx12 ? "dx12" : "dx11", Preset = low ? "performance" : "original", Upscaler = a.Software ? "native" : a.Dlss ? "dlss" : "fsr", UpscaleQuality = low ? "balanced" : "quality", Reflections = !low ? strong && a.Spectral ? "rtx" : "ssr" : "off", ReflectionQuality = strong ? "balanced" : "performance" };
            p.Quality = QualityPreset(p.Preset); p.Quality["pixelRatio"] = 1.0; p.Quality["scale"] = 100;
            p.AutomaticNativeScale = a.Software ? 50 : low ? 80 : 100;
            if (a.Software) { p.Quality["scale"] = 50; p.Quality["shadows"] = 0; p.Quality["transmission"] = .25; }
            else if (low) p.Quality["transmission"] = .5;
            p.Reason = a.Software ? "优先保证窗口可以显示：DX11 软件渲染、较低分辨率，关闭反射。软件模式主要用于兼容排查。" : low ? "根据显存、内存与供电状态优先推荐稳定配置：DX11 与适度画质，减少渲染负担。" : strong && a.Spectral ? "已检测到可用的 DX12、硬件光追与足够显存，推荐保留玻璃光追反射并配合超分。" : "已验证图形设备能力，推荐平衡画质；使用 SSR 保留玻璃反射。";
            return p;
        }
        internal static Profile Existing(Dictionary<string, object> saved, Machine machine) {
            Profile p = Recommend(machine); p.Mode = JsonData.Text(saved, "configurationMode", "automatic") == "custom" ? "custom" : "automatic";
            bool software = JsonData.Flag(saved, "graphicsSoftware"); string identity = JsonData.Text(saved, "graphicsAdapter");
            Adapter a = machine.Adapters.FirstOrDefault(x => software ? x.Software : !x.Software && !String.IsNullOrEmpty(identity) && x.Identity == identity);
            if (a != null && p.Mode == "custom") p.Adapter = a;
            if (p.Mode == "custom") {
                p.Api = JsonData.Text(saved, "graphicsApi", p.Api); p.Reflections = JsonData.Text(saved, "reflections", p.Reflections); p.ReflectionQuality = JsonData.Text(saved, "reflectionQuality", p.ReflectionQuality); p.Preset = JsonData.Text(saved, "quality", p.Preset);
                var quality = JsonData.Object(JsonData.Value(saved, "renderQuality", null)); if (quality.Count > 0) foreach (var pair in quality) if (p.Quality.ContainsKey(pair.Key)) p.Quality[pair.Key] = pair.Value;
            }
            p.Upscaler = JsonData.Text(saved, "upscaler", p.Upscaler); p.UpscaleQuality = JsonData.Text(saved, "upscaleQuality", p.UpscaleQuality); p.Normalize(); return p;
        }
        internal void Normalize() {
            if (Adapter == null || !Adapter.Dx11) throw new InvalidOperationException("没有可用的渲染设备。");
            if (Adapter.Software) { Api = "dx11"; Upscaler = "native"; Reflections = "off"; }
            if (Api != "dx12" || !Adapter.Dx12) Api = "dx11";
            if (Upscaler != "native" && Upscaler != "fsr" && Upscaler != "dlss") Upscaler = Adapter.Dlss ? "dlss" : Adapter.Software ? "native" : "fsr";
            if (Upscaler == "dlss" && !Adapter.Dlss) Upscaler = Adapter.Software ? "native" : "fsr";
            if (!new[] { "quality", "balanced", "performance", "native-aa" }.Contains(UpscaleQuality)) UpscaleQuality = "quality";
            if (Api != "dx12" || !Adapter.Spectral) { if (Reflections == "rtx") Reflections = Adapter.Software ? "off" : "ssr"; }
            if (!new[] { "off", "ssr", "rtx" }.Contains(Reflections)) Reflections = "off";
            if (!new[] { "performance", "balanced", "high" }.Contains(ReflectionQuality)) ReflectionQuality = "balanced";
            if (!new[] { "performance", "original", "high", "ultra" }.Contains(Preset)) Preset = "original";
            var defaults = QualityPreset(Preset);
            foreach (string key in defaults.Keys.ToArray()) if (!Quality.ContainsKey(key)) Quality[key] = defaults[key];
            foreach (var pair in new Dictionary<string, double[]> { { "pixelRatio", new[] { 1.0, 1.5, 2.0, 3.0 } }, { "shadows", new[] { 0.0, 1024, 2048, 4096 } }, { "aoSamples", new[] { 0.0, 16, 32, 64 } }, { "aoResolution", new[] { .5, .75, 1 } }, { "transmission", new[] { .25, .5, .75, 1 } }, { "anisotropy", new[] { 1.0, 2, 4, 8, 16 } } }) { double v = JsonData.Number(Quality, pair.Key, Double.NaN); if (!pair.Value.Contains(v)) Quality[pair.Key] = defaults[pair.Key]; }
            Quality["scale"] = Math.Max(50, Math.Min(200, Math.Round(JsonData.Number(Quality, "scale", 100) / 5) * 5));
            Quality["depthOfField"] = Math.Max(0, Math.Min(150, Math.Round(JsonData.Number(Quality, "depthOfField", 100) / 5) * 5));
            if (!new[] { "off", "smaa" }.Contains(JsonData.Text(Quality, "antialias"))) Quality["antialias"] = "off";
            if (Mode == "automatic") Quality["scale"] = Upscaler == "native" ? AutomaticNativeScale : 100;
        }
        internal Dictionary<string, object> Patch() {
            Normalize(); return new Dictionary<string, object> { { "graphicsApi", Api }, { "graphicsAdapter", Adapter.Identity ?? "" }, { "graphicsSoftware", Adapter.Software }, { "upscaler", Upscaler }, { "upscaleQuality", UpscaleQuality }, { "reflections", Reflections }, { "reflectionQuality", ReflectionQuality }, { "quality", Preset }, { "renderQuality", Quality }, { "configurationMode", Mode } };
        }
        internal string Summary { get { return (Adapter.Software ? "DX11 软件渲染" : Api.ToUpperInvariant()) + "  /  " + (Reflections == "rtx" ? "光追反射" : Reflections == "ssr" ? "SSR 反射" : "关闭反射") + "  /  " + (Upscaler == "native" ? "原生分辨率" : Upscaler.ToUpperInvariant() + " 超分") + "  /  渲染 " + JsonData.Number(Quality, "scale") + "%"; } }
    }
    internal static class SettingsStore {
        internal static string InstanceMutexName = "Local\\RhineMusicD3D.UserInstance";
        internal static string DirectoryPath { get { string value = Environment.GetEnvironmentVariable("RHINE_APP_HOME"); return Path.GetFullPath(String.IsNullOrWhiteSpace(value) ? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Rhine Music") : value); } }
        internal static Dictionary<string, object> Load() { return JsonData.Read(Path.Combine(DirectoryPath, "preferences.json")); }
        internal static void Save(Profile profile) {
            bool created;
            using (var playerGate = new Mutex(false, InstanceMutexName, out created)) {
                if (!created) throw new InvalidOperationException("请先关闭播放器，再保存启动配置。");
                string file = Path.Combine(DirectoryPath, "preferences.json"); var existing = JsonData.Read(file);
                foreach (var pair in profile.Patch()) existing[pair.Key] = pair.Value;
                JsonData.WriteAtomic(file, existing);
            }
        }
    }
    internal static class HardwareProbe {
        internal static Machine Detect() {
            string parent = Path.GetFullPath(Path.Combine(Path.GetTempPath(), "RhineMusic-Configuration")); string directory = Path.GetFullPath(Path.Combine(parent, Guid.NewGuid().ToString("N")));
            if (!directory.StartsWith(parent.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Invalid probe directory.");
            Directory.CreateDirectory(directory);
            try {
                string executable = Path.Combine(directory, "RhineConfigurationProbe.exe"), report = Path.Combine(directory, "hardware.json");
                using (var input = Assembly.GetExecutingAssembly().GetManifestResourceStream("Rhine.Configuration.Probe")) { if (input == null) throw new InvalidDataException("配置程序的硬件检测组件缺失。"); using (var output = new FileStream(executable, FileMode.CreateNew, FileAccess.Write, FileShare.None)) input.CopyTo(output); }
                var start = new ProcessStartInfo(executable, "--report \"" + report + "\"") { UseShellExecute = false, CreateNoWindow = true, WindowStyle = ProcessWindowStyle.Hidden, WorkingDirectory = directory };
                using (var process = Process.Start(start)) { if (!process.WaitForExit(20000)) { process.Kill(); process.WaitForExit(3000); return Machine.Fallback("显卡检测超时。可先使用 DX11 软件兼容模式，或点击重新检测。"); } if (process.ExitCode != 0 || !File.Exists(report)) return Machine.Fallback("显卡检测未完成。可先使用 DX11 软件兼容模式，或点击重新检测。"); }
                return Machine.From(JsonData.Read(report));
            } catch { return Machine.Fallback("显卡检测未完成。可先使用 DX11 软件兼容模式，或点击重新检测。"); }
            finally { try { if (Directory.Exists(directory) && Path.GetFullPath(directory).StartsWith(parent.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)) Directory.Delete(directory, true); } catch { } }
        }
    }
}

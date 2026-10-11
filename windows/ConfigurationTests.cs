using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows.Forms;
namespace Rhine.Configuration {
    internal static class ConfigurationTests {
        private static int checks;
        private static void Require(bool value, string text) { ++checks; if (!value) throw new InvalidOperationException(text); }
        private static Dictionary<string, object> AdapterData(string name, uint vendor, double gb, bool dx12, bool rays, bool dlss = false, bool software = false) { return new Dictionary<string, object> { { "name", name }, { "luid", software ? "" : "0000000000001234" }, { "vendorId", vendor }, { "dedicatedBytes", gb * 1073741824.0 }, { "software", software }, { "d3d11", true }, { "d3d12", dx12 }, { "raytracingTier", rays ? 11 : 0 }, { "inlineRaytracing", rays }, { "spectralCausticsEligible", rays }, { "dlssEligible", dlss } }; }
        private static Dictionary<string, object> MachineData(Dictionary<string, object>[] adapters, double ram = 16, bool battery = false) { return new Dictionary<string, object> { { "ok", true }, { "adapters", adapters }, { "memoryBytes", ram * 1073741824.0 }, { "logicalProcessors", 8 }, { "onBattery", battery } }; }
        private static IEnumerable<Control> RoundedControls(Control parent) {
            foreach (Control control in parent.Controls) {
                if (control is ActionButton || control is Surface) yield return control;
                foreach (Control descendant in RoundedControls(control)) yield return descendant;
            }
        }
        private static void VerifyRoundedEdges(Form form) {
            // Render each child independently: a prefilled full-window bitmap can
            // hide missing background painting around a rounded child control.
            foreach (var group in RoundedControls(form).GroupBy(control => control.Parent)) {
                Control parent = group.Key; Point clientOrigin = parent is Form ? parent.PointToScreen(Point.Empty) - new Size(parent.Location) : Point.Empty;
                using (var backdrop = new Bitmap(parent.Width, parent.Height)) {
                    parent.DrawToBitmap(backdrop, new Rectangle(Point.Empty, parent.Size));
                    foreach (Control control in group) using (var bitmap = new Bitmap(control.Width, control.Height)) {
                        control.DrawToBitmap(bitmap, new Rectangle(Point.Empty, control.Size));
                        foreach (bool right in new[] { false, true }) foreach (bool bottom in new[] { false, true }) {
                            // A scrolling panel has a native scrollbar outside its
                            // client area; inspect the painted rounded surface.
                            int x = right ? control.ClientSize.Width - 1 : 0, y = bottom ? control.ClientSize.Height - 1 : 0;
                            int parentX = clientOrigin.X + control.Left + (right ? control.Width + 1 : -2);
                            int parentY = clientOrigin.Y + control.Top + (bottom ? control.Height + 1 : -2);
                            Color actual = bitmap.GetPixel(x, y), expected = backdrop.GetPixel(parentX, parentY);
                            Require(actual.A == 255 && Math.Abs(actual.R - expected.R) <= 6 && Math.Abs(actual.G - expected.G) <= 6 && Math.Abs(actual.B - expected.B) <= 6,
                                "Rounded corner background mismatch: " + control.GetType().Name + " / " + control.Text + " / " + (Colors.Night ? "night" : "day") + " / right=" + right + " / bottom=" + bottom + " / actual " + actual + " / expected " + expected);
                        }
                    }
                }
            }
        }
        internal static int Run(string[] args) {
            string report = null; for (int i = 1; i < args.Length; ++i) { if (args[i] == "--report" && i + 1 < args.Length) report = Path.GetFullPath(args[++i]); else throw new ArgumentException("Invalid self-test arguments"); }
            if (report == null) throw new ArgumentException("Self-test requires --report");
            string directory = Path.GetDirectoryName(report); Directory.CreateDirectory(directory); string testHome = Path.Combine(directory, "isolated-home-" + Guid.NewGuid().ToString("N")); string previous = Environment.GetEnvironmentVariable("RHINE_APP_HOME"), previousMutex = SettingsStore.InstanceMutexName; Environment.SetEnvironmentVariable("RHINE_APP_HOME", testHome); SettingsStore.InstanceMutexName = "Local\\RhineMusicD3D.ConfigurationTests." + Guid.NewGuid().ToString("N");
            try {
                var fixtures = new Dictionary<string, Dictionary<string, object>> {
                    { "nvidia-rtx", MachineData(new[] { AdapterData("NVIDIA RTX · 测试样例", 0x10de, 8, true, true, true) }) },
                    { "nvidia-gtx", MachineData(new[] { AdapterData("NVIDIA GTX · 测试样例", 0x10de, 4, true, false) }) },
                    { "amd", MachineData(new[] { AdapterData("AMD Radeon · 测试样例", 0x1002, 8, true, true, true) }) },
                    { "intel", MachineData(new[] { AdapterData("Intel Arc · 测试样例", 0x8086, 8, true, true, true) }) },
                    { "legacy", MachineData(new[] { AdapterData("DX11 老显卡 · 测试样例", 0x1002, 1, false, false) }, 4) },
                    { "software", MachineData(new[] { AdapterData("Windows 软件渲染 · 测试样例", 0, 0, false, false, false, true) }, 4) }
                };
                var profiles = new Dictionary<string, object>();
                foreach (var pair in fixtures) {
                    JsonData.WriteAtomic(Path.Combine(testHome, "preferences.json"), new Dictionary<string, object>());
                    var machine = Machine.From(pair.Value); var profile = Profile.Recommend(machine); profile.Normalize(); var patch = profile.Patch(); profiles[pair.Key] = patch;
                    Require(profile.Adapter.Dx11, pair.Key + " lacks compatible adapter"); Require(profile.Api != "dx12" || profile.Adapter.Dx12, pair.Key + " selected unsupported DX12"); Require(profile.Upscaler != "dlss" || profile.Adapter.Vendor == 0x10de, pair.Key + " exposed DLSS on another vendor"); Require(profile.Reflections != "rtx" || profile.Adapter.Spectral && profile.Api == "dx12", pair.Key + " selected unsupported ray tracing");
                    profile.Api = "dx12"; profile.Upscaler = "dlss"; profile.Reflections = "rtx"; profile.Normalize(); Require(profile.Upscaler != "dlss" || profile.Adapter.Dlss, pair.Key + " failed to normalize forced DLSS"); Require(profile.Reflections != "rtx" || profile.Adapter.Spectral, pair.Key + " failed to normalize forced ray tracing");
                    JsonData.WriteAtomic(Path.Combine(directory, pair.Key + ".json"), pair.Value);
                    using (var form = new ConfigurationForm(machine)) {
                        form.ShowInTaskbar = false; form.Opacity = 0; form.Show(); Application.DoEvents();
                        form.CapturePreview(Path.Combine(directory, pair.Key + "-automatic.png"), pair.Key != "amd", false); VerifyRoundedEdges(form);
                        form.CapturePreview(Path.Combine(directory, pair.Key + "-custom.png"), pair.Key != "amd", true); VerifyRoundedEdges(form);
                        form.CaptureCompactPreview(Path.Combine(directory, pair.Key + "-compact.png")); VerifyRoundedEdges(form);
                        form.CapturePreview(Path.Combine(directory, pair.Key + "-opposite-theme.png"), pair.Key == "amd", true); VerifyRoundedEdges(form);
                        form.VerifyInterface(Require); form.Close();
                    }
                }
                var duplicate = AdapterData("Same GPU", 0x10de, 8, true, true, true); duplicate["luid"] = "0000000000000002"; var integrated = AdapterData("Same GPU", 0x8086, 0, true, false); integrated["luid"] = "0000000000000001";
                var multi = Machine.From(MachineData(new[] { integrated, duplicate })); Require(multi.Best.Identity == "0000000000000002", "Auto recommendation did not select the capable discrete GPU");
                var automaticSaved = Profile.Existing(new Dictionary<string, object> { { "configurationMode", "automatic" }, { "graphicsAdapter", integrated["luid"] } }, multi); Require(automaticSaved.Adapter == multi.Best, "Saved automatic mode failed to reevaluate the best GPU");
                var customSaved = Profile.Existing(new Dictionary<string, object> { { "configurationMode", "custom" }, { "graphicsAdapter", integrated["luid"] } }, multi); Require(customSaved.Adapter.Identity == "0000000000000001", "Custom mode did not preserve the selected GPU");
                var battery = Profile.Recommend(Machine.From(MachineData(new[] { duplicate }, 16, true))); Require(battery.Api == "dx11" && battery.Reflections == "off", "Battery recommendation ignored power limits");
                var software = Profile.Recommend(Machine.From(fixtures["software"])); Require(software.Api == "dx11" && software.Upscaler == "native" && software.Reflections == "off" && JsonData.Number(software.Quality, "scale") == 50, "Software compatibility is not conservative");
                var malformed = Profile.Existing(new Dictionary<string, object> { { "configurationMode", "custom" }, { "graphicsApi", "invalid" }, { "upscaler", "invalid" }, { "renderQuality", new Dictionary<string, object> { { "scale", "bad" }, { "shadows", -9 }, { "aoSamples", 999 } } } }, multi); Require(malformed.Api == "dx11" && JsonData.Number(malformed.Quality, "scale") == 100 && JsonData.Number(malformed.Quality, "shadows") == 2048, "Malformed saved graphics settings were not normalized");
                string preferences = Path.Combine(testHome, "preferences.json"); var before = new Dictionary<string, object> { { "theme", "night" }, { "volume", .43 }, { "repeatMode", "album" }, { "output", new Dictionary<string, object> { { "backend", "asio" }, { "device", 4 }, { "dsdMode", "native" } } }, { "unrelatedTestValue", "preserved" } }; JsonData.WriteAtomic(preferences, before);
                var desired = Profile.Recommend(Machine.From(fixtures["legacy"])); desired.Mode = "custom"; desired.Quality["scale"] = 65; SettingsStore.Save(desired); var after = JsonData.Read(preferences);
                Require(JsonData.Text(after, "theme") == "night" && JsonData.Number(after, "volume") == .43 && JsonData.Text(after, "repeatMode") == "album", "Saving graphics modified playback or theme preferences"); Require(JsonData.Serializer.Serialize(after["output"]) == JsonData.Serializer.Serialize(before["output"]), "Saving graphics modified audio output"); Require(JsonData.Text(after, "unrelatedTestValue") == "preserved", "Saving graphics removed unrelated preferences"); Require(JsonData.Number(JsonData.Object(after["renderQuality"]), "scale") == 65, "Fine scale value was not saved");
                string unchanged = File.ReadAllText(preferences); bool rejected = false; using (var running = new Mutex(false, SettingsStore.InstanceMutexName)) { try { SettingsStore.Save(desired); } catch (InvalidOperationException) { rejected = true; } } Require(rejected && File.ReadAllText(preferences) == unchanged, "Running-player guard failed to preserve settings");
                File.WriteAllText(preferences, "{invalid JSON"); rejected = false; try { SettingsStore.Save(desired); } catch { rejected = true; } Require(rejected && File.ReadAllText(preferences) == "{invalid JSON", "Malformed preferences were overwritten");
                var actual = HardwareProbe.Detect(); Require(actual.Detected, "Real hardware probe failed"); Require(actual.Adapters.Count > 0 && actual.Memory > 0, "Real hardware report is incomplete");
                JsonData.WriteAtomic(report, new Dictionary<string, object> { { "ok", true }, { "version", "0.1" }, { "checks", checks }, { "syntheticProfiles", profiles }, { "hardware", new Dictionary<string, object> { { "detected", actual.Detected }, { "memoryBytes", actual.Memory }, { "logicalProcessors", actual.Processors }, { "adapters", actual.Adapters.Select(a => new Dictionary<string, object> { { "name", a.Name }, { "luid", a.Identity }, { "dx11", a.Dx11 }, { "dx12", a.Dx12 }, { "hardwareRaytracing", a.HardwareRays }, { "spectralEligible", a.Spectral }, { "dlssEligible", a.Dlss }, { "software", a.Software } }).ToArray() } } } }); return 0;
            } catch (Exception failure) { JsonData.WriteAtomic(report, new Dictionary<string, object> { { "ok", false }, { "version", "0.1" }, { "checks", checks }, { "error", failure.ToString() } }); return 2; }
            finally { Environment.SetEnvironmentVariable("RHINE_APP_HOME", previous); SettingsStore.InstanceMutexName = previousMutex; }
        }
    }
}

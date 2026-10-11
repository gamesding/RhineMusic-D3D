using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

[assembly: AssemblyTitle("Rhine Music · 图形配置")]
[assembly: AssemblyDescription("Rhine Music Direct3D graphics configuration")]
[assembly: AssemblyCompany("GamesDing")]
[assembly: AssemblyProduct("Rhine Music Direct3D")]
[assembly: AssemblyVersion("0.1.0.0")]
[assembly: AssemblyFileVersion("0.1.0.0")]

namespace Rhine.Configuration {
    internal static class Colors {
        internal static bool Night;
        internal static Color Background { get { return Night ? Color.FromArgb(11, 18, 29) : Color.FromArgb(238, 236, 230); } }
        internal static Color Card { get { return Night ? Color.FromArgb(19, 30, 46) : Color.FromArgb(251, 250, 246); } }
        internal static Color Input { get { return Night ? Color.FromArgb(27, 41, 59) : Color.FromArgb(241, 239, 232); } }
        internal static Color Ink { get { return Night ? Color.FromArgb(233, 239, 245) : Color.FromArgb(35, 42, 40); } }
        internal static Color Muted { get { return Night ? Color.FromArgb(156, 174, 193) : Color.FromArgb(101, 111, 106); } }
        internal static Color Border { get { return Night ? Color.FromArgb(54, 71, 91) : Color.FromArgb(207, 209, 198); } }
        internal static Color Accent { get { return Night ? Color.FromArgb(231, 183, 118) : Color.FromArgb(137, 87, 34); } }
        internal static Color Error { get { return Night ? Color.FromArgb(255, 161, 150) : Color.FromArgb(160, 43, 30); } }
        internal static Color Blend(Color a, Color b, float amount) { return Color.FromArgb((int)(a.R + (b.R - a.R) * amount), (int)(a.G + (b.G - a.G) * amount), (int)(a.B + (b.B - a.B) * amount)); }
    }
    internal static class Drawing {
        internal static GraphicsPath Rounded(RectangleF rect, float radius) { var path = new GraphicsPath(); float d = Math.Min(radius * 2, Math.Min(rect.Width, rect.Height)); path.AddArc(rect.X, rect.Y, d, d, 180, 90); path.AddArc(rect.Right - d, rect.Y, d, d, 270, 90); path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90); path.AddArc(rect.X, rect.Bottom - d, d, d, 90, 90); path.CloseFigure(); return path; }
        internal static void Card(Graphics g, RectangleF rect, float radius, Color fill, Color stroke, float weight = 1) { using (var path = Rounded(rect, radius)) { using (var brush = new SolidBrush(fill)) g.FillPath(brush, path); using (var pen = new Pen(stroke, weight)) g.DrawPath(pen, path); } }
        internal static void ParentBackground(Control control, PaintEventArgs e, Action<Control, PaintEventArgs> background, Action<Control, PaintEventArgs> foreground) {
            if (control.Parent == null) { e.Graphics.Clear(Colors.Background); return; }
            GraphicsState state = e.Graphics.Save();
            try {
                e.Graphics.TranslateTransform(-control.Left, -control.Top);
                Rectangle clip = e.ClipRectangle; clip.Offset(control.Left, control.Top);
                using (var parentPaint = new PaintEventArgs(e.Graphics, clip)) {
                    background(control.Parent, parentPaint);
                    foreground(control.Parent, parentPaint);
                }
            } finally { e.Graphics.Restore(state); }
        }
        internal static Font Font(float size, bool bold = false) { return new Font("Microsoft YaHei UI", size, bold ? FontStyle.Bold : FontStyle.Regular, GraphicsUnit.Point); }
        internal static void Text(Graphics g, string text, float size, bool bold, Color color, RectangleF box, bool center = false) { using (var font = Font(size, bold)) using (var brush = new SolidBrush(color)) using (var format = new StringFormat { Alignment = center ? StringAlignment.Center : StringAlignment.Near, LineAlignment = center ? StringAlignment.Center : StringAlignment.Near, Trimming = StringTrimming.EllipsisCharacter }) g.DrawString(text, font, brush, box, format); }
        internal static void DarkTitle(IntPtr handle) { try { int enabled = Colors.Night ? 1 : 0; DwmSetWindowAttribute(handle, 20, ref enabled, sizeof(int)); } catch { } }
        [DllImport("dwmapi.dll")] private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int value, int size);
    }
    internal sealed class ActionButton : Button {
        internal bool Primary, Selected, ModeCard;
        internal string Caption = "";
        private float hover;
        private bool hovered;
        private DateTime transition;
        private float origin;
        private readonly Timer motion = new Timer { Interval = 15 };
        internal ActionButton() {
            SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw | ControlStyles.SupportsTransparentBackColor | ControlStyles.Opaque, true);
            FlatStyle = FlatStyle.Flat; FlatAppearance.BorderSize = 0; UseVisualStyleBackColor = false; BackColor = Color.Transparent; Cursor = Cursors.Hand; TabStop = true; AccessibleRole = AccessibleRole.PushButton;
            motion.Tick += delegate { float elapsed = Math.Min(1, (float)(DateTime.UtcNow - transition).TotalMilliseconds / 160); hover = origin + ((hovered ? 1 : 0) - origin) * elapsed; Invalidate(); if (elapsed >= 1) motion.Stop(); };
        }
        private void Animate(bool value) { hovered = value; origin = hover; transition = DateTime.UtcNow; motion.Start(); }
        protected override void OnMouseEnter(EventArgs e) { base.OnMouseEnter(e); Animate(true); }
        protected override void OnMouseLeave(EventArgs e) { base.OnMouseLeave(e); Animate(false); }
        protected override void OnGotFocus(EventArgs e) { base.OnGotFocus(e); Invalidate(); }
        protected override void OnLostFocus(EventArgs e) { base.OnLostFocus(e); Invalidate(); }
        protected override void OnPaint(PaintEventArgs e) {
            // The full client area must be painted, including the space outside
            // the rounded outline. Include the parent's custom card, not only
            // its BackColor, for buttons nested inside a painted surface.
            Drawing.ParentBackground(this, e, InvokePaintBackground, InvokePaint);
            var g = e.Graphics; g.SmoothingMode = SmoothingMode.AntiAlias; g.TextRenderingHint = TextRenderingHint.ClearTypeGridFit; float s = g.DpiX / 96;
            Color fill = Primary ? Colors.Ink : Selected ? Colors.Blend(Colors.Card, Colors.Accent, .10f) : Colors.Card;
            fill = Colors.Blend(fill, Primary ? Colors.Accent : Colors.Input, hover * (Primary ? .2f : 1)); Color ink = Primary ? Colors.Background : Colors.Ink;
            Drawing.Card(g, new RectangleF(1, 1, Width - 3, Height - 3), 13 * s, fill, Focused || Selected ? Colors.Accent : Colors.Border, Focused || Selected ? 2 * s : s);
            if (!Enabled) ink = Colors.Muted;
            if (ModeCard) {
                bool compact = Height < 80 * s; float x = 20 * s, y = (compact ? 15 : 26) * s; using (var pen = new Pen(Selected ? Colors.Accent : Colors.Muted, 1.5f * s)) g.DrawEllipse(pen, x, y, 16 * s, 16 * s);
                if (Selected) using (var brush = new SolidBrush(Colors.Accent)) g.FillEllipse(brush, x + 4 * s, y + 4 * s, 8 * s, 8 * s);
                Drawing.Text(g, Text, 13, true, ink, new RectangleF(50 * s, (compact ? 9 : 19) * s, Width - 66 * s, 26 * s));
                Drawing.Text(g, Caption, 10, false, Colors.Muted, new RectangleF(50 * s, (compact ? 37 : 51) * s, Width - 66 * s, Height - (compact ? 41 : 56) * s));
            } else Drawing.Text(g, Text, 10.5f, true, ink, new RectangleF(0, 0, Width, Height), true);
        }
        protected override void Dispose(bool disposing) { if (disposing) motion.Dispose(); base.Dispose(disposing); }
    }
    internal sealed class Surface : Panel {
        internal Surface() { SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw | ControlStyles.SupportsTransparentBackColor | ControlStyles.Opaque, true); BackColor = Color.Transparent; }
        protected override void OnPaint(PaintEventArgs e) { Drawing.ParentBackground(this, e, InvokePaintBackground, InvokePaint); e.Graphics.SmoothingMode = SmoothingMode.AntiAlias; float s = e.Graphics.DpiX / 96; Drawing.Card(e.Graphics, new RectangleF(1, 1, ClientSize.Width - 3, ClientSize.Height - 3), 16 * s, Colors.Card, Colors.Border); base.OnPaint(e); }
    }
    internal sealed class Option {
        internal object Value;
        internal string Label;
        internal Option(object value, string label) { Value = value; Label = label; }
        public override string ToString() { return Label; }
    }
    internal sealed class MenuRenderer : ToolStripProfessionalRenderer {
        protected override void OnRenderToolStripBackground(ToolStripRenderEventArgs e) { e.Graphics.Clear(Colors.Input); }
        protected override void OnRenderToolStripBorder(ToolStripRenderEventArgs e) { using (var pen = new Pen(Colors.Border)) e.Graphics.DrawRectangle(pen, 0, 0, e.ToolStrip.Width - 1, e.ToolStrip.Height - 1); }
        protected override void OnRenderMenuItemBackground(ToolStripItemRenderEventArgs e) { using (var brush = new SolidBrush(e.Item.Selected ? Colors.Blend(Colors.Input, Colors.Accent, .18f) : Colors.Input)) e.Graphics.FillRectangle(brush, new Rectangle(2, 0, e.Item.Width - 4, e.Item.Height)); }
        protected override void OnRenderItemText(ToolStripItemTextRenderEventArgs e) { e.TextColor = Colors.Ink; base.OnRenderItemText(e); }
        protected override void OnRenderImageMargin(ToolStripRenderEventArgs e) { e.Graphics.Clear(Colors.Input); }
    }
    // The closed face is fully drawn in software. Native owner-drawn ComboBox
    // edit faces are theme/WM_PRINT dependent and can render blank on old hosts.
    internal sealed class ChoiceBox : Control {
        private readonly List<Option> options = new List<Option>();
        private ContextMenuStrip popup;
        private int selectedIndex = -1;
        internal event EventHandler Changed;
        internal object Value { get { return selectedIndex < 0 ? null : options[selectedIndex].Value; } }
        internal IEnumerable<object> Values { get { return options.Select(x => x.Value); } }
        internal string SelectedLabel { get { return selectedIndex < 0 ? "" : options[selectedIndex].Label; } }
        internal ChoiceBox(string name) { SetStyle(ControlStyles.UserPaint | ControlStyles.AllPaintingInWmPaint | ControlStyles.OptimizedDoubleBuffer | ControlStyles.ResizeRedraw | ControlStyles.Selectable, true); AccessibleName = name; AccessibleRole = AccessibleRole.ComboBox; TabStop = true; Cursor = Cursors.Hand; }
        internal void Options(IEnumerable<Option> values, object selected) { options.Clear(); options.AddRange(values); selectedIndex = -1; for (int i = 0; i < options.Count; ++i) { object value = options[i].Value; bool equal = value is Adapter || selected is Adapter ? Object.ReferenceEquals(value, selected) : String.Equals(Convert.ToString(value, System.Globalization.CultureInfo.InvariantCulture), Convert.ToString(selected, System.Globalization.CultureInfo.InvariantCulture), StringComparison.Ordinal); if (equal) { selectedIndex = i; break; } } if (selectedIndex < 0 && options.Count > 0) selectedIndex = 0; AccessibleDescription = SelectedLabel; Invalidate(); }
        internal void Theme() { BackColor = Colors.Card; ForeColor = Colors.Ink; Invalidate(); }
        internal void Choose(int index) { if (index < 0 || index >= options.Count) return; selectedIndex = index; AccessibleDescription = SelectedLabel; Invalidate(); if (Changed != null) Changed(this, EventArgs.Empty); }
        private void OpenMenu() { if (!Enabled || options.Count == 0) return; if (popup != null) popup.Dispose(); popup = new ContextMenuStrip { Renderer = new MenuRenderer(), ShowImageMargin = false, ShowCheckMargin = true, Font = Drawing.Font(10.5f), BackColor = Colors.Input, ForeColor = Colors.Ink, MinimumSize = new Size(Width, 0), MaximumSize = new Size(Math.Max(Width, 380), Math.Max(180, Screen.FromControl(this).WorkingArea.Height / 2)) }; for (int i = 0; i < options.Count; ++i) { int value = i; var item = new ToolStripMenuItem(options[i].Label) { Checked = i == selectedIndex, Padding = new Padding(8, 6, 8, 6) }; item.Click += delegate { Choose(value); }; popup.Items.Add(item); } popup.Show(this, new Point(0, Height)); }
        protected override void OnMouseDown(MouseEventArgs e) { base.OnMouseDown(e); if (e.Button == MouseButtons.Left) { Focus(); OpenMenu(); } }
        protected override bool IsInputKey(Keys keyData) { Keys key = keyData & Keys.KeyCode; return key == Keys.Up || key == Keys.Down || key == Keys.Enter || key == Keys.Space || key == Keys.F4 || base.IsInputKey(keyData); }
        protected override void OnKeyDown(KeyEventArgs e) { base.OnKeyDown(e); if (e.KeyCode == Keys.Space || e.KeyCode == Keys.Enter || e.KeyCode == Keys.F4 || e.Alt && e.KeyCode == Keys.Down) { OpenMenu(); e.Handled = true; } else if (e.KeyCode == Keys.Down || e.KeyCode == Keys.Up) { Choose(Math.Max(0, Math.Min(options.Count - 1, selectedIndex + (e.KeyCode == Keys.Down ? 1 : -1)))); e.Handled = true; } }
        protected override void OnGotFocus(EventArgs e) { base.OnGotFocus(e); Invalidate(); }
        protected override void OnLostFocus(EventArgs e) { base.OnLostFocus(e); Invalidate(); }
        internal void VerifyKeyboard(Action<bool, string> verify) { verify(IsInputKey(Keys.Enter), "Enter on a dropdown would unexpectedly save the form"); OnKeyDown(new KeyEventArgs(Keys.Enter)); verify(popup != null && popup.Visible && popup.Items.Count == options.Count, "Keyboard could not open the real dropdown menu"); popup.Close(); }
        protected override void OnPaint(PaintEventArgs e) { var g = e.Graphics; float s = g.DpiX / 96; g.Clear(Colors.Card); g.SmoothingMode = SmoothingMode.AntiAlias; Drawing.Card(g, new RectangleF(1, 1, Width - 3, Height - 3), 9 * s, Colors.Input, Focused ? Colors.Accent : Colors.Border, Focused ? 2 * s : s); using (var font = Drawing.Font(10.5f)) TextRenderer.DrawText(g, SelectedLabel, font, new Rectangle((int)(13 * s), 0, Width - (int)(45 * s), Height), Enabled ? Colors.Ink : Colors.Muted, TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis | TextFormatFlags.NoPrefix); using (var pen = new Pen(Colors.Muted, 1.5f * s)) { float x = Width - 24 * s, y = Height / 2f; g.DrawLines(pen, new[] { new PointF(x - 4 * s, y - 2 * s), new PointF(x, y + 2 * s), new PointF(x + 4 * s, y - 2 * s) }); } }
        protected override void Dispose(bool disposing) { if (disposing && popup != null) popup.Dispose(); base.Dispose(disposing); }
    }
    internal sealed class SuccessDialog : Form {
        private readonly ActionButton accept = new ActionButton { Text = "确定", Primary = true, DialogResult = DialogResult.OK, AccessibleName = "确定并关闭配置程序" };
        internal SuccessDialog(Icon icon) { Text = "Rhine Music · 设置成功"; Icon = icon; FormBorderStyle = FormBorderStyle.FixedDialog; MaximizeBox = false; MinimizeBox = false; StartPosition = FormStartPosition.CenterParent; AutoScaleMode = AutoScaleMode.None; using (var g = CreateGraphics()) { float s = g.DpiX / 96; ClientSize = new Size((int)(460 * s), (int)(310 * s)); accept.SetBounds((int)(126 * s), (int)(240 * s), (int)(208 * s), (int)(44 * s)); } BackColor = Colors.Card; Controls.Add(accept); AcceptButton = accept; Shown += delegate { Drawing.DarkTitle(Handle); accept.Focus(); }; }
        protected override void OnPaint(PaintEventArgs e) { base.OnPaint(e); var g = e.Graphics; float s = g.DpiX / 96; g.SmoothingMode = SmoothingMode.AntiAlias; using (var pen = new Pen(Colors.Accent, 2 * s)) { g.DrawEllipse(pen, 202 * s, 30 * s, 56 * s, 56 * s); pen.StartCap = pen.EndCap = LineCap.Round; g.DrawLines(pen, new[] { new PointF(218 * s, 58 * s), new PointF(227 * s, 68 * s), new PointF(243 * s, 49 * s) }); } Drawing.Text(g, "设置成功。", 21, true, Colors.Ink, new RectangleF(30 * s, 99 * s, 400 * s, 44 * s), true); Drawing.Text(g, "配置已保存。关闭此窗口后，\n请手动启动 RhineMusic.exe。", 11, false, Colors.Muted, new RectangleF(36 * s, 157 * s, 388 * s, 58 * s), true); }
    }
    internal sealed class ConfigurationForm : Form {
        private Machine machine;
        private Profile profile;
        private Dictionary<string, object> saved = new Dictionary<string, object>();
        private bool building, detecting, closing;
        private string error = "";
        private readonly Surface hardware = new Surface(), settings = new Surface();
        private readonly Label gpu = new Label(), specifications = new Label(), capabilities = new Label();
        private readonly ActionButton rescan = new ActionButton { Text = "重新检测" }, theme = new ActionButton(), automatic = new ActionButton { Text = "自动", Caption = "检测设备能力，推荐适合这台电脑的画面", ModeCard = true }, custom = new ActionButton { Text = "自定义", Caption = "自己选择图形接口、反射与精细画质", ModeCard = true }, apply = new ActionButton { Text = "确认并保存", Primary = true }, cancel = new ActionButton { Text = "取消" };
        private readonly Dictionary<string, ChoiceBox> fields = new Dictionary<string, ChoiceBox>();
        private readonly List<string> fieldOrder = new List<string>();
        private readonly List<Label> labels = new List<Label>();
        private readonly Machine injected;
        internal Action<SuccessDialog> SuccessOpened;
        private float Unit { get { using (var g = CreateGraphics()) return g.DpiX / 96; } }
        internal ConfigurationForm(Machine preview = null) {
            injected = preview; Text = "Rhine Music · 图形配置 0.1"; Icon = Icon.ExtractAssociatedIcon(Application.ExecutablePath); AutoScaleMode = AutoScaleMode.None; DoubleBuffered = true; StartPosition = FormStartPosition.CenterScreen;
            float s = Unit; Rectangle work = Screen.FromControl(this).WorkingArea; Size border = SizeFromClientSize(Size.Empty);
            int availableWidth = Math.Max(400, work.Width - border.Width - 24), availableHeight = Math.Max(320, work.Height - border.Height - 24);
            MinimumSize = new Size(Math.Min((int)(760 * s) + border.Width, work.Width - 24), Math.Min((int)(480 * s) + border.Height, work.Height - 24));
            ClientSize = new Size(Math.Min((int)(1040 * s), availableWidth), Math.Min((int)(800 * s), availableHeight));
            foreach (var label in new[] { gpu, specifications, capabilities }) { label.AutoEllipsis = true; label.BackColor = Color.Transparent; hardware.Controls.Add(label); }
            gpu.Font = Drawing.Font(15, true); specifications.Font = Drawing.Font(10); capabilities.Font = Drawing.Font(10); hardware.Controls.Add(rescan); settings.AutoScroll = true;
            Controls.AddRange(new Control[] { hardware, settings, automatic, custom, apply, cancel, theme });
            automatic.Click += delegate { SwitchMode("automatic"); }; custom.Click += delegate { SwitchMode("custom"); }; rescan.Click += async delegate { await Detect(); };
            theme.Click += delegate { Colors.Night = !Colors.Night; RefreshTheme(); }; cancel.Click += delegate { Close(); }; apply.Click += delegate { Save(); };
            AcceptButton = apply; CancelButton = cancel; FormClosing += delegate { closing = true; };
            try { saved = SettingsStore.Load(); Colors.Night = JsonData.Text(saved, "theme", "day") == "night"; } catch { error = "现有配置无法读取。请先备份并检查配置文件，再保存新设置。"; }
            RefreshTheme(); Shown += async delegate { Drawing.DarkTitle(Handle); if (injected != null) CompleteDetection(injected); else await Detect(); };
        }
        private async Task Detect() { if (detecting) return; detecting = true; apply.Enabled = false; rescan.Enabled = false; automatic.Enabled = custom.Enabled = false; gpu.Text = "正在检测显卡与图形接口…"; specifications.Text = "稍候，检测在后台进行。"; capabilities.Text = ""; Invalidate(); var result = await Task.Run(() => HardwareProbe.Detect()); if (!closing && !IsDisposed) CompleteDetection(result); }
        private void CompleteDetection(Machine value) { machine = value; profile = Profile.Existing(saved, machine); detecting = false; apply.Enabled = true; rescan.Enabled = true; automatic.Enabled = custom.Enabled = true; if (!machine.Detected) error = machine.Error; UpdateHardware(); BuildFields(); Invalidate(); }
        private void UpdateHardware() { if (profile == null) return; Adapter a = profile.Adapter; gpu.Text = a.Name; specifications.Text = (a.Software ? "软件兼容渲染" : a.VideoMemory > 0 ? "显存 " + (a.VideoMemory / 1073741824.0).ToString("0.#") + " GB" : "共享显存") + "    ·    内存 " + (machine.Memory / 1073741824.0).ToString("0.#") + " GB    ·    " + machine.Processors + " 逻辑处理器" + (machine.Battery ? "    ·    电池供电" : ""); capabilities.Text = "DX11 可用" + (a.Dx12 ? "    /    DX12 可用" : "    /    DX12 不可用") + (a.HardwareRays ? "    /    支持硬件光追" : "    /    不支持硬件光追") + (a.Spectral ? "    /    彩虹光追可用" : "") + (a.Dlss ? "    /    DLSS 可选" : !a.Software ? "    /    FSR 可选" : ""); }
        private void SwitchMode(string mode) { if (profile == null || detecting) return; ReadFields(); string upscaler = profile.Upscaler, quality = profile.UpscaleQuality; if (mode == "automatic") { profile = Profile.Recommend(machine); profile.Upscaler = upscaler; profile.UpscaleQuality = quality; } profile.Mode = mode; profile.Normalize(); UpdateHardware(); BuildFields(); }
        private static Option[] Options(params string[] values) { var result = new List<Option>(); for (int i = 0; i < values.Length; i += 2) result.Add(new Option(values[i], values[i + 1])); return result.ToArray(); }
        private static Option[] Numbers(params double[] values) { return values.Select(v => new Option(v, v == 0 ? "关闭" : v.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture))).ToArray(); }
        private Option[] Upscalers() { if (profile.Adapter.Software) return Options("native", "关闭超分 · 原生分辨率"); var result = new List<Option>(Options("native", "关闭超分 · 原生分辨率", "fsr", profile.Api == "dx11" ? "AMD FSR · DX11 兼容模式" : "AMD FSR · 通用超分")); if (profile.Adapter.Dlss) result.Add(new Option("dlss", "NVIDIA DLSS · RTX 超分")); return result.ToArray(); }
        private Option[] Reflections() { var result = new List<Option>(Options("off", "关闭反射")); if (!profile.Adapter.Software) result.Add(new Option("ssr", "SSR · 屏幕空间反射")); if (profile.Adapter.Spectral && profile.Api == "dx12") result.Add(new Option("rtx", "光追 · 彩虹玻璃反射")); return result.ToArray(); }
        private void AddField(string key, string text, IEnumerable<Option> options, object selected) { var label = new Label { Text = text, Font = Drawing.Font(10), ForeColor = Colors.Muted, BackColor = Colors.Card, AutoEllipsis = true }; var field = new ChoiceBox(text); field.Options(options, selected); field.Changed += delegate { FieldChanged(key); }; fields.Add(key, field); fieldOrder.Add(key); labels.Add(label); settings.Controls.Add(label); settings.Controls.Add(field); field.Theme(); }
        private void BuildFields() {
            building = true; settings.SuspendLayout(); foreach (Control control in settings.Controls.Cast<Control>().ToArray()) control.Dispose(); fields.Clear(); fieldOrder.Clear(); labels.Clear(); settings.AutoScrollPosition = Point.Empty; automatic.Selected = profile.Mode == "automatic"; custom.Selected = !automatic.Selected; automatic.Invalidate(); custom.Invalidate();
            if (profile.Mode == "custom") { AddField("adapter", "渲染设备", machine.Adapters.Select(a => new Option(a, a.Name)), profile.Adapter); AddField("api", "图形接口", profile.Adapter.Dx12 ? Options("dx11", "Direct3D 11 · 兼容", "dx12", "Direct3D 12") : Options("dx11", profile.Adapter.Software ? "Direct3D 11 · 软件兼容" : "Direct3D 11 · 兼容"), profile.Api); }
            AddField("upscaler", "超分技术", Upscalers(), profile.Upscaler); AddField("upscaleQuality", "超分画质", Options("quality", "质量", "balanced", "平衡", "performance", "性能", "native-aa", "原生抗锯齿"), profile.UpscaleQuality);
            if (profile.Mode == "custom") {
                AddField("reflections", "玻璃与光盘反射", Reflections(), profile.Reflections); AddField("reflectionQuality", "反射画质", Options("performance", "性能", "balanced", "平衡", "high", "高"), profile.ReflectionQuality);
                AddField("preset", "基础画质", Options("performance", "性能", "original", "原始", "high", "高", "ultra", "极高"), profile.Preset); AddField("scale", "渲染比例", Enumerable.Range(0, 31).Select(i => 50 + i * 5).Select(v => new Option((double)v, v + "%")), profile.Quality["scale"]);
                AddField("shadows", "阴影分辨率", Numbers(0, 1024, 2048, 4096), profile.Quality["shadows"]); AddField("aoSamples", "环境遮蔽采样", Numbers(0, 16, 32, 64), profile.Quality["aoSamples"]);
                AddField("transmission", "玻璃透射分辨率", new[] { .25, .5, .75, 1 }.Select(v => new Option(v, (v * 100) + "%")), profile.Quality["transmission"]); AddField("depthOfField", "景深强度", Enumerable.Range(0, 31).Select(i => i * 5).Select(v => new Option((double)v, v == 0 ? "关闭" : v + "%")), profile.Quality["depthOfField"]);
                AddField("antialias", "抗锯齿", Options("off", "原始", "smaa", "SMAA"), profile.Quality["antialias"]); AddField("anisotropy", "纹理过滤", new[] { 1, 2, 4, 8, 16 }.Select(v => new Option((double)v, v + "×")), profile.Quality["anisotropy"]);
                AddField("pixelRatio", "像素密度上限", new[] { 1.0, 1.5, 2.0, 3.0 }.Select(v => new Option(v, v + "×")), profile.Quality["pixelRatio"]); AddField("aoResolution", "环境遮蔽分辨率", new[] { .5, .75, 1 }.Select(v => new Option(v, (v * 100) + "%")), profile.Quality["aoResolution"]);
            } else {
                var explanation = new Label { Name = "recommendation", Text = profile.Reason, Font = Drawing.Font(11), ForeColor = Colors.Ink, BackColor = Colors.Card };
                var tip = new Label { Name = "recommendation-tip", Text = "推荐配置基于已检测的设备能力。若仍无法显示画面，可在自定义中选择 DX11 或软件兼容渲染。", Font = Drawing.Font(10), ForeColor = Colors.Muted, BackColor = Colors.Card }; settings.Controls.Add(explanation); settings.Controls.Add(tip);
            }
            fields["upscaleQuality"].Enabled = profile.Upscaler != "native"; settings.ResumeLayout(); building = false; LayoutFields(); RefreshTheme(); Invalidate();
        }
        private void FieldChanged(string key) {
            if (building || profile == null) return;
            if (key == "adapter") { profile.Adapter = fields["adapter"].Value as Adapter; profile.Normalize(); UpdateHardware(); BuildFields(); return; }
            ReadFields(); if (key == "preset") { profile.Quality = Profile.QualityPreset(profile.Preset); BuildFields(); return; }
            if (key == "api") { building = true; fields["reflections"].Options(Reflections(), profile.Reflections); fields["upscaler"].Options(Upscalers(), profile.Upscaler); building = false; }
            fields["upscaleQuality"].Enabled = profile.Upscaler != "native"; Invalidate();
        }
        private void ReadFields() { if (profile == null || building) return; foreach (var pair in fields) { object value = pair.Value.Value; switch (pair.Key) { case "adapter": profile.Adapter = value as Adapter; break; case "api": profile.Api = Convert.ToString(value); break; case "upscaler": profile.Upscaler = Convert.ToString(value); break; case "upscaleQuality": profile.UpscaleQuality = Convert.ToString(value); break; case "reflections": profile.Reflections = Convert.ToString(value); break; case "reflectionQuality": profile.ReflectionQuality = Convert.ToString(value); break; case "preset": profile.Preset = Convert.ToString(value); break; default: profile.Quality[pair.Key] = value; break; } } profile.Normalize(); }
        private void Save() { if (profile == null || detecting) return; try { ReadFields(); SettingsStore.Save(profile); error = ""; using (var success = new SuccessDialog(Icon)) { if (SuccessOpened != null) success.Shown += delegate { SuccessOpened(success); }; success.ShowDialog(this); } Close(); } catch (Exception failure) { error = failure is InvalidOperationException || failure is InvalidDataException ? failure.Message : "设置未能保存。请检查配置文件与目录是否可写，现有设置已保留。"; Invalidate(); } }
        private void RefreshTheme() { BackColor = Colors.Background; theme.Text = Colors.Night ? "☀ 暖昼" : "☾ 深夜"; foreach (var label in new[] { gpu, specifications, capabilities }) { label.ForeColor = label == gpu ? Colors.Ink : label == capabilities ? Colors.Accent : Colors.Muted; } foreach (Label label in labels) { label.ForeColor = Colors.Muted; label.BackColor = Colors.Card; } foreach (var field in fields.Values) field.Theme(); foreach (Control label in settings.Controls) if (label is Label) { label.BackColor = Colors.Card; label.ForeColor = label.Name == "recommendation" ? Colors.Ink : Colors.Muted; } if (IsHandleCreated) Drawing.DarkTitle(Handle); Invalidate(true); }
        protected override void OnLayout(LayoutEventArgs e) {
            base.OnLayout(e); if (hardware == null || apply == null) return; float s = Unit;
            bool compact = ClientSize.Height < 720 * s, dense = ClientSize.Height < 500 * s;
            int pad = (int)(32 * s), gap = (int)(16 * s), width = ClientSize.Width - pad * 2;
            int hardwareTop = dense ? 84 : compact ? 112 : 144, hardwareHeight = dense ? 82 : compact ? 90 : 120;
            int modeTop = dense ? 178 : compact ? 212 : 284, modeHeight = dense ? 62 : compact ? 72 : 92, settingsTop = dense ? 254 : compact ? 300 : 396;
            hardware.SetBounds(pad, (int)(hardwareTop * s), width, (int)(hardwareHeight * s));
            theme.SetBounds(ClientSize.Width - pad - (int)(92 * s), (int)((dense ? 12 : 24) * s), (int)(92 * s), (int)(34 * s));
            gpu.SetBounds((int)(22 * s), (int)((compact ? 8 : 19) * s), width - (int)(195 * s), (int)((compact ? 28 : 32) * s));
            specifications.SetBounds((int)(22 * s), (int)((dense ? 36 : compact ? 40 : 57) * s), width - (int)(44 * s), (int)((compact ? 22 : 24) * s));
            capabilities.SetBounds((int)(22 * s), (int)((dense ? 58 : compact ? 65 : 86) * s), width - (int)(44 * s), (int)((compact ? 22 : 25) * s));
            rescan.SetBounds(width - (int)(135 * s), (int)((compact ? 8 : 18) * s), (int)(112 * s), (int)(34 * s));
            int half = (width - gap) / 2; automatic.SetBounds(pad, (int)(modeTop * s), half, (int)(modeHeight * s)); custom.SetBounds(pad + half + gap, automatic.Top, width - half - gap, automatic.Height);
            settings.SetBounds(pad, (int)(settingsTop * s), width, Math.Max((int)(72 * s), ClientSize.Height - (int)(settingsTop * s) - (int)(105 * s)));
            apply.SetBounds(ClientSize.Width - pad - (int)(160 * s), ClientSize.Height - (int)(65 * s), (int)(160 * s), (int)(44 * s)); cancel.SetBounds(apply.Left - (int)(99 * s), apply.Top, (int)(85 * s), apply.Height); LayoutFields();
        }
        private void LayoutFields() {
            if (building || profile == null || settings.Width < 10) return; float s = Unit; int pad = (int)(22 * s), gap = (int)(24 * s), contentWidth = settings.ClientSize.Width - pad * 2 - (settings.VerticalScroll.Visible ? SystemInformation.VerticalScrollBarWidth : 0), half = (contentWidth - gap) / 2; Point scroll = settings.AutoScrollPosition;
            int index = 0; foreach (string key in fieldOrder) { int col = index % 2, row = index / 2, x = pad + col * (half + gap), y = (int)(18 * s + row * 85 * s); labels[index].SetBounds(x + scroll.X, y + scroll.Y, half, (int)(24 * s)); fields[key].SetBounds(x + scroll.X, y + (int)(27 * s) + scroll.Y, half, (int)(42 * s)); index++; }
            if (profile.Mode == "automatic") { Control recommendation = settings.Controls["recommendation"], tip = settings.Controls["recommendation-tip"]; if (recommendation != null) recommendation.SetBounds(pad + scroll.X, (int)(116 * s) + scroll.Y, contentWidth, (int)(63 * s)); if (tip != null) tip.SetBounds(pad + scroll.X, (int)(194 * s) + scroll.Y, contentWidth, (int)(51 * s)); settings.AutoScrollMinSize = new Size(0, (int)(260 * s)); }
            else settings.AutoScrollMinSize = new Size(0, (int)(((index + 1) / 2 * 85 + 30) * s));
        }
        protected override void OnPaint(PaintEventArgs e) {
            base.OnPaint(e); var g = e.Graphics; float s = g.DpiX / 96; bool compact = ClientSize.Height < 720 * s, dense = ClientSize.Height < 500 * s; g.SmoothingMode = SmoothingMode.AntiAlias; g.TextRenderingHint = TextRenderingHint.ClearTypeGridFit;
            using (var gradient = new LinearGradientBrush(new Rectangle(0, 0, ClientSize.Width, (int)(144 * s)), Colors.Blend(Colors.Background, Colors.Accent, .05f), Colors.Background, LinearGradientMode.Horizontal)) g.FillRectangle(gradient, 0, 0, ClientSize.Width, (int)(138 * s));
            Drawing.Text(g, "RHINE MUSIC   /   CONFIGURATION", 10, true, Colors.Muted, new RectangleF(32 * s, (dense ? 15 : 25) * s, ClientSize.Width - 175 * s, 24 * s));
            Drawing.Text(g, "为这台电脑，选好画面。", compact ? 21 : 25, true, Colors.Ink, new RectangleF(30 * s, (dense ? 42 : compact ? 54 : 66) * s, ClientSize.Width - 62 * s, (compact ? 38 : 52) * s));
            if (!dense) Drawing.Text(g, "启动播放器前设置图形渲染    ·    原生版 0.1", 10, false, Colors.Muted, new RectangleF(33 * s, (compact ? 91 : 119) * s, ClientSize.Width - 70 * s, 23 * s));
            using (var pen = new Pen(Colors.Border)) g.DrawLine(pen, 32 * s, ClientSize.Height - 90 * s, ClientSize.Width - 32 * s, ClientSize.Height - 90 * s);
            string summary = profile == null ? "检测完成后即可选择配置。" : profile.Summary; Drawing.Text(g, String.IsNullOrEmpty(error) ? summary : error, 9.5f, false, String.IsNullOrEmpty(error) ? Colors.Ink : Colors.Error, new RectangleF(34 * s, ClientSize.Height - 75 * s, apply.Left - 152 * s, 39 * s));
            Drawing.Text(g, "保存后关闭此窗口，再手动启动播放器。", 9, false, Colors.Muted, new RectangleF(34 * s, ClientSize.Height - 30 * s, ClientSize.Width - 65 * s, 23 * s));
        }
        internal void CapturePreview(string file, bool night, bool customMode) { Colors.Night = night; if (customMode) SwitchMode("custom"); RefreshTheme(); PerformLayout(); Refresh(); using (var bitmap = new Bitmap(Width, Height)) { using (var graphics = Graphics.FromImage(bitmap)) graphics.Clear(Colors.Background); DrawToBitmap(bitmap, new Rectangle(0, 0, Width, Height)); bitmap.Save(file, System.Drawing.Imaging.ImageFormat.Png); } }
        internal void CaptureCompactPreview(string file) { float s = Unit; ClientSize = new Size((int)(800 * s), (int)(480 * s)); CapturePreview(file, Colors.Night, true); }
        internal void VerifyInterface(Action<bool, string> verify) {
            verify(profile.Mode == "custom", "Custom mode card did not update the UI");
            verify(!String.IsNullOrEmpty(fields["upscaler"].SelectedLabel) && !String.IsNullOrEmpty(fields["api"].SelectedLabel), "A configuration field is blank");
            verify(fields["upscaler"].Values.Contains("dlss") == profile.Adapter.Dlss, "The visible upscaler menu does not match the GPU");
            verify(fields["api"].Values.Contains("dx12") == profile.Adapter.Dx12, "The visible API menu offers unsupported DX12"); fields["upscaler"].VerifyKeyboard(verify);
            var api = fields["api"]; api.Choose(0); verify(profile.Api == "dx11" && !fields["reflections"].Values.Contains("rtx"), "Changing to DX11 left ray tracing selectable");
            var scale = fields["scale"]; scale.Choose(scale.Values.ToList().FindIndex(v => Convert.ToDouble(v) == 65)); verify(JsonData.Number(profile.Quality, "scale") == 65, "Fine quality selection did not update the saved profile");
            float s = Unit; ClientSize = new Size((int)(880 * s), (int)(720 * s)); PerformLayout(); Application.DoEvents(); settings.AutoScrollPosition = new Point(0, 10000); Application.DoEvents();
            int bottom = settings.AutoScrollPosition.Y; verify(bottom < 0, "Custom settings have no usable scrollbar"); verify(fields["aoResolution"].Bottom <= settings.ClientSize.Height && fields["aoResolution"].Bottom > 0, "Bottom settings cannot be reached by scrolling");
            settings.AutoScrollPosition = new Point(0, 10000); Application.DoEvents(); verify(settings.AutoScrollPosition.Y == bottom, "Scrolling past the bottom moves the layout"); settings.AutoScrollPosition = Point.Empty; Application.DoEvents(); verify(settings.AutoScrollPosition.Y == 0, "Scrolling back to the top failed");
            ClientSize = new Size((int)(800 * s), (int)(480 * s)); PerformLayout(); Application.DoEvents();
            verify(apply.Bottom <= ClientSize.Height && settings.Bottom <= apply.Top - 20 * s, "Small-screen layout clips or overlaps the save controls");
            verify(custom.Bottom < settings.Top && hardware.Bottom < custom.Top, "Small-screen layout overlaps hardware or mode controls");
            bool confirmed = false;
            SuccessOpened = delegate(SuccessDialog dialog) { confirmed = true; dialog.ShowInTaskbar = false; dialog.Opacity = 0; verify(dialog.Visible, "Save did not show the success confirmation"); dialog.AcceptButton.PerformClick(); };
            apply.PerformClick(); verify(confirmed && !Visible, "Confirmation did not close the configuration program");
            var persisted = SettingsStore.Load(); verify(JsonData.Text(persisted, "graphicsApi") == "dx11" && JsonData.Number(JsonData.Object(persisted["renderQuality"]), "scale") == 65, "The real save button did not persist the selected settings");
        }
    }
    internal static class Program {
        [STAThread] internal static int Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            try {
                if (args.Length > 0 && args[0] == "--self-test") return ConfigurationTests.Run(args);
                Machine preview = null; for (int i = 0; i < args.Length; ++i) { if (args[i] == "--preview-machine" && i + 1 < args.Length) preview = Machine.From(JsonData.Read(args[++i])); else throw new ArgumentException("不支持的配置程序参数。"); }
                using (var form = new ConfigurationForm(preview)) Application.Run(form); return 0;
            } catch (Exception error) { MessageBox.Show(error.Message, "Rhine Music · 图形配置", MessageBoxButtons.OK, MessageBoxIcon.Error); return 1; }
        }
    }
}

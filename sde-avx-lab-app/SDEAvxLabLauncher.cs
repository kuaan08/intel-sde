using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace SdeAvxLab
{
    internal sealed class CpuProfile
    {
        public string Name;
        public string Switch;
        public string Description;
        public CpuProfile(string name, string value, string description)
        {
            Name = name; Switch = value; Description = description;
        }
        public override string ToString() { return Name; }
    }

    internal sealed class LauncherSettings
    {
        public string SdePath = "";
        public string TargetPath = "";
        public string WorkingDirectory = "";
        public string Arguments = "";
        public string Profile = "-snb";
        public string Backend = "SDE AVX Lab Runtime (principal; experimental)";
        public bool Mix;
        public bool XInUse;

        public string Serialize()
        {
            return "SdePath=" + Escape(SdePath) + Environment.NewLine +
                   "TargetPath=" + Escape(TargetPath) + Environment.NewLine +
                   "WorkingDirectory=" + Escape(WorkingDirectory) + Environment.NewLine +
                   "Arguments=" + Escape(Arguments) + Environment.NewLine +
                   "Profile=" + Escape(Profile) + Environment.NewLine +
                   "Backend=" + Escape(Backend) + Environment.NewLine +
                   "Mix=" + Mix.ToString() + Environment.NewLine +
                   "XInUse=" + XInUse.ToString() + Environment.NewLine;
        }

        public static LauncherSettings Parse(string text)
        {
            LauncherSettings s = new LauncherSettings();
            string[] lines = text.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string line in lines)
            {
                int p = line.IndexOf('=');
                if (p <= 0) continue;
                string key = line.Substring(0, p);
                string value = Unescape(line.Substring(p + 1));
                if (key == "SdePath") s.SdePath = value;
                else if (key == "TargetPath") s.TargetPath = value;
                else if (key == "WorkingDirectory") s.WorkingDirectory = value;
                else if (key == "Arguments") s.Arguments = value;
                else if (key == "Profile") s.Profile = value;
                else if (key == "Backend") s.Backend = value;
                else if (key == "Mix") bool.TryParse(value, out s.Mix);
                else if (key == "XInUse") bool.TryParse(value, out s.XInUse);
            }
            return s;
        }

        private static string Escape(string value) { return (value ?? "").Replace("\\", "\\\\").Replace("\r", "\\r").Replace("\n", "\\n"); }
        private static string Unescape(string value)
        {
            StringBuilder result = new StringBuilder(); string text = value ?? "";
            for (int i = 0; i < text.Length; i++)
            {
                if (text[i] == '\\' && i + 1 < text.Length)
                {
                    char next = text[++i]; if (next == 'n') result.Append('\n'); else if (next == 'r') result.Append('\r'); else result.Append(next);
                }
                else result.Append(text[i]);
            }
            return result.ToString();
        }
    }

    internal sealed class MainForm : Form
    {
        private readonly Color BackgroundColor = Color.FromArgb(16, 20, 28);
        private readonly Color CardColor = Color.FromArgb(27, 34, 46);
        private readonly Color FieldColor = Color.FromArgb(11, 15, 21);
        private readonly Color AccentColor = Color.FromArgb(45, 130, 220);
        private TextBox sdePath, targetPath, workPath, targetArgs;
        private ComboBox backendBox, profileBox;
        private CheckBox mixBox, xinuseBox;
        private Label backendInfo, profileInfo, status;
        private RichTextBox eventLog;
        private string appData;
        private string profileDir;
        private readonly List<CpuProfile> profiles = new List<CpuProfile>();

        public MainForm()
        {
            Text = "SDE AVX Lab — Game Compatibility Launcher";
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new Size(940, 760);
            MinimumSize = new Size(880, 680);
            BackColor = BackgroundColor;
            ForeColor = Color.White;
            Font = new Font("Segoe UI", 9F);
            appData = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "SDEAvxLab");
            profileDir = Path.Combine(appData, "profiles");
            Directory.CreateDirectory(profileDir);
            BuildProfiles();
            BuildUi();
            AutoDetectPaths();
            WriteLog("Aplicativo iniciado. Nenhum processo foi executado.");
        }

        private void BuildProfiles()
        {
            profiles.Add(new CpuProfile("AVX1 — Sandy Bridge (-snb)", "-snb", "Primeira referência para AVX1 e o teste do Celeron N5100."));
            profiles.Add(new CpuProfile("AVX1 — Ivy Bridge (-ivb)", "-ivb", "Perfil AVX1 alternativo para comparação."));
            profiles.Add(new CpuProfile("AVX2 — Haswell (-hsw)", "-hsw", "Perfil AVX2. Use somente se a aplicação realmente exigir AVX2."));
            profiles.Add(new CpuProfile("AVX2 — Broadwell (-bdw)", "-bdw", "Perfil AVX2 alternativo."));
            profiles.Add(new CpuProfile("AVX2 — Skylake (-skl)", "-skl", "Perfil AVX2 com recursos de Skylake."));
        }

        private Panel Card(int x, int y, int w, int h)
        {
            Panel p = new Panel(); p.BackColor = CardColor; p.Location = new Point(x, y); p.Size = new Size(w, h); return p;
        }
        private Label LabelOn(Control parent, string text, int x, int y, int w, int h, bool bold)
        {
            Label l = new Label(); l.Text = text; l.Location = new Point(x, y); l.Size = new Size(w, h);
            l.ForeColor = bold ? Color.White : Color.FromArgb(180, 191, 207);
            if (bold) l.Font = new Font("Segoe UI Semibold", 10F, FontStyle.Bold);
            parent.Controls.Add(l); return l;
        }
        private TextBox TextOn(Control parent, int x, int y, int w, int h)
        {
            TextBox t = new TextBox(); t.Location = new Point(x, y); t.Size = new Size(w, h);
            t.BackColor = FieldColor; t.ForeColor = Color.White; t.BorderStyle = BorderStyle.FixedSingle;
            parent.Controls.Add(t); return t;
        }
        private Button ButtonOn(Control parent, string text, int x, int y, int w, int h, EventHandler click, bool primary)
        {
            Button b = new Button(); b.Text = text; b.Location = new Point(x, y); b.Size = new Size(w, h);
            b.FlatStyle = FlatStyle.Flat; b.FlatAppearance.BorderSize = primary ? 0 : 1;
            b.BackColor = primary ? AccentColor : Color.FromArgb(42, 53, 69); b.ForeColor = Color.White;
            b.Click += click; parent.Controls.Add(b); return b;
        }

        private void BuildUi()
        {
            ClientSize = new Size(1120, 840); MinimumSize = new Size(980, 720); AutoScroll = true;
            Label title = new Label(); title.Text = "SDE AVX Lab"; title.Font = new Font("Segoe UI Semibold", 25F, FontStyle.Bold); title.Location = new Point(30, 20); title.Size = new Size(500, 42); Controls.Add(title);
            Label subtitle = new Label(); subtitle.Text = "AVX compatibility runtime  /  Windows x64"; subtitle.ForeColor = Color.FromArgb(147, 164, 187); subtitle.Location = new Point(34, 63); subtitle.Size = new Size(650, 24); Controls.Add(subtitle);
            Label version = new Label(); version.Text = "PRIMARY BACKEND  •  EXPERIMENTAL"; version.ForeColor = Color.FromArgb(111, 190, 255); version.TextAlign = ContentAlignment.MiddleRight; version.Location = new Point(760, 28); version.Size = new Size(320, 28); Controls.Add(version);

            Panel engine = Card(24, 100, 1072, 116); Controls.Add(engine); LabelOn(engine, "01   EXECUTION BACKEND", 22, 14, 420, 24, true);
            backendBox = new ComboBox(); backendBox.DropDownStyle = ComboBoxStyle.DropDownList; backendBox.Location = new Point(22, 49); backendBox.Size = new Size(455, 31); backendBox.Items.Add("SDE AVX Lab Runtime (principal; experimental)"); backendBox.Items.Add("Intel SDE 9.58 (fallback externo)"); backendBox.SelectedIndex = 0; backendBox.BackColor = FieldColor; backendBox.ForeColor = Color.White; backendBox.SelectedIndexChanged += BackendChanged; engine.Controls.Add(backendBox);
            backendInfo = LabelOn(engine, "O runtime próprio é o foco principal. Nesta versão, EXECUTAR roda o self-test; a tradução de processos está em desenvolvimento.", 520, 45, 510, 48, false);

            Panel files = Card(24, 234, 1072, 244); Controls.Add(files); LabelOn(files, "02   TARGET APPLICATION", 22, 14, 420, 24, true);
            LabelOn(files, "Backend executable", 22, 52, 195, 22, false); sdePath = TextOn(files, 228, 49, 745, 29); ButtonOn(files, "BROWSE", 986, 48, 66, 31, BrowseSde, false);
            LabelOn(files, "Target application", 22, 93, 195, 22, false); targetPath = TextOn(files, 228, 90, 745, 29); ButtonOn(files, "BROWSE", 986, 89, 66, 31, BrowseTarget, false);
            LabelOn(files, "Working directory", 22, 134, 195, 22, false); workPath = TextOn(files, 228, 131, 745, 29); ButtonOn(files, "FOLDER", 986, 130, 66, 31, BrowseWork, false);
            LabelOn(files, "Application arguments", 22, 175, 195, 22, false); targetArgs = TextOn(files, 228, 172, 824, 29);
            LabelOn(files, "The primary runtime currently executes deterministic core tests. Select the external fallback only when you need to launch an arbitrary target.", 228, 207, 824, 22, false);

            Panel options = Card(24, 496, 1072, 142); Controls.Add(options); LabelOn(options, "03   CPU PROFILE & OPTIONS", 22, 14, 420, 24, true);
            profileBox = new ComboBox(); profileBox.DropDownStyle = ComboBoxStyle.DropDownList; profileBox.Location = new Point(22, 49); profileBox.Size = new Size(455, 31); profileBox.BackColor = FieldColor; profileBox.ForeColor = Color.White; foreach (CpuProfile p in profiles) profileBox.Items.Add(p); profileBox.SelectedIndex = 0; profileBox.SelectedIndexChanged += ProfileChanged; options.Controls.Add(profileBox);
            profileInfo = LabelOn(options, profiles[0].Description, 520, 49, 510, 28, false);
            mixBox = new CheckBox(); mixBox.Text = "Generate -mix report"; mixBox.Location = new Point(25, 96); mixBox.AutoSize = true; mixBox.ForeColor = Color.White; mixBox.Checked = false; options.Controls.Add(mixBox);
            xinuseBox = new CheckBox(); xinuseBox.Text = "-emu-xinuse 0  /  experimental"; xinuseBox.Location = new Point(235, 96); xinuseBox.AutoSize = true; xinuseBox.ForeColor = Color.White; xinuseBox.Checked = false; options.Controls.Add(xinuseBox);

            Panel actions = Card(24, 658, 1072, 72); Controls.Add(actions);
            ButtonOn(actions, "EXECUTE", 22, 16, 145, 40, Launch, true); ButtonOn(actions, "SAVE PROFILE", 180, 16, 125, 40, SaveProfile, false); ButtonOn(actions, "LOAD PROFILE", 318, 16, 135, 40, LoadProfile, false); ButtonOn(actions, "DIAGNOSTICS", 466, 16, 125, 40, Diagnose, false); ButtonOn(actions, "OPEN LOGS", 604, 16, 110, 40, OpenLogs, false);
            status = LabelOn(actions, "READY", 775, 23, 280, 28, false); status.ForeColor = Color.FromArgb(115, 210, 145); status.TextAlign = ContentAlignment.MiddleRight;

            Panel logs = Card(24, 750, 1072, 92); Controls.Add(logs); LabelOn(logs, "EVENT LOG", 22, 12, 100, 20, true);
            eventLog = new RichTextBox(); eventLog.Location = new Point(130, 10); eventLog.Size = new Size(922, 70); eventLog.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right | AnchorStyles.Bottom; eventLog.BackColor = Color.FromArgb(9, 12, 17); eventLog.ForeColor = Color.FromArgb(180, 198, 218); eventLog.ReadOnly = true; eventLog.BorderStyle = BorderStyle.FixedSingle; logs.Controls.Add(eventLog);
            BackendChanged(null, EventArgs.Empty);
        }

        private void AutoDetectPaths()
        {
            string here = AppDomain.CurrentDomain.BaseDirectory;
            string[] candidates = {
                Path.GetFullPath(Path.Combine(here, "..", "..", "windows-9.58", "sde.exe")),
                Path.GetFullPath(Path.Combine(here, "..", "..", "intel-sde", "windows-9.58", "sde.exe")),
                Path.GetFullPath(Path.Combine(here, "..", "intel-sde", "windows-9.58", "sde.exe")),
                Path.GetFullPath(Path.Combine(here, "..", "windows-9.58", "sde.exe")),
                @"C:\Sde\sde.exe"
            };
            foreach (string p in candidates) if (File.Exists(p)) { sdePath.Text = p; break; }
            if (String.IsNullOrWhiteSpace(sdePath.Text)) sdePath.Text = candidates[0];
        }

        private void BrowseSde(object sender, EventArgs e) { PickFile(sdePath, "Selecione sde.exe", "sde.exe|sde.exe|Executáveis|*.exe|Todos|*.*"); }
        private void BrowseTarget(object sender, EventArgs e) { PickFile(targetPath, "Selecione o jogo ou programa", "Programas Windows|*.exe|Todos|*.*"); }
        private void PickFile(TextBox target, string title, string filter)
        {
            using (OpenFileDialog d = new OpenFileDialog()) { d.Title = title; d.Filter = filter; d.CheckFileExists = true; if (d.ShowDialog(this) == DialogResult.OK) { target.Text = d.FileName; if (String.IsNullOrWhiteSpace(workPath.Text)) workPath.Text = Path.GetDirectoryName(d.FileName); } }
        }
        private void BrowseWork(object sender, EventArgs e) { using (FolderBrowserDialog d = new FolderBrowserDialog()) if (d.ShowDialog(this) == DialogResult.OK) workPath.Text = d.SelectedPath; }
        private void ProfileChanged(object sender, EventArgs e) { CpuProfile p = profileBox.SelectedItem as CpuProfile; if (p != null) profileInfo.Text = p.Description; }
        private void BackendChanged(object sender, EventArgs e)
        {
            bool external = backendBox != null && backendBox.SelectedIndex == 1;
            if (backendInfo != null) backendInfo.Text = external ? "Fallback separado: o Intel SDE inicia o aplicativo selecionado. O desenvolvimento principal continua no runtime próprio." : "O runtime próprio é o foco principal. Nesta versão, o botão EXECUTAR roda o self-test; a tradução de processos está em desenvolvimento.";
            if (sdePath != null) sdePath.Enabled = external;
            if (targetPath != null) targetPath.Enabled = external;
            if (workPath != null) workPath.Enabled = external;
            if (targetArgs != null) targetArgs.Enabled = external;
            if (mixBox != null) mixBox.Enabled = external;
            if (xinuseBox != null) xinuseBox.Enabled = external;
        }

        private bool ValidateCommon()
        {
            if (String.IsNullOrWhiteSpace(targetPath.Text) || !File.Exists(targetPath.Text)) { MessageBox.Show(this, "Selecione um EXE válido.", "Arquivo ausente", MessageBoxButtons.OK, MessageBoxIcon.Warning); return false; }
            if (String.IsNullOrWhiteSpace(workPath.Text)) workPath.Text = Path.GetDirectoryName(targetPath.Text);
            if (!Directory.Exists(workPath.Text)) { MessageBox.Show(this, "O diretório de trabalho não existe.", "Diretório ausente", MessageBoxButtons.OK, MessageBoxIcon.Warning); return false; }
            return true;
        }

        private void Launch(object sender, EventArgs e)
        {
            if (backendBox.SelectedIndex == 0) { RunIndependentBackend(); return; }
            if (!ValidateCommon()) return;
            if (!File.Exists(sdePath.Text)) { MessageBox.Show(this, "Não encontrei sde.exe. Selecione o arquivo Windows 9.58.", "SDE ausente", MessageBoxButtons.OK, MessageBoxIcon.Warning); return; }
            CpuProfile p = profileBox.SelectedItem as CpuProfile;
            string args = p.Switch;
            if (mixBox.Checked) args += " -mix -mix_format json";
            if (xinuseBox.Checked) args += " -emu-xinuse 0";
            args += " -- " + Quote(targetPath.Text);
            if (!String.IsNullOrWhiteSpace(targetArgs.Text)) args += " " + targetArgs.Text.Trim();
            try
            {
                ProcessStartInfo psi = new ProcessStartInfo(); psi.FileName = sdePath.Text; psi.Arguments = args; psi.WorkingDirectory = workPath.Text; psi.UseShellExecute = false; psi.CreateNoWindow = false;
                Process proc = Process.Start(psi); string line = "SDE iniciado. PID=" + proc.Id + " perfil=" + p.Switch;
                WriteLog(line + " comando=" + sdePath.Text + " " + args); status.Text = "Executando PID " + proc.Id; status.ForeColor = Color.FromArgb(115, 210, 145);
            }
            catch (Exception ex) { WriteLog("Falha ao iniciar: " + ex.Message); status.Text = "Falha ao iniciar"; status.ForeColor = Color.FromArgb(235, 120, 120); MessageBox.Show(this, ex.Message, "Falha ao executar", MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }

        private void RunIndependentBackend()
        {
            string here = AppDomain.CurrentDomain.BaseDirectory;
            string[] candidates = {
                Path.Combine(here, "..", "..", "avx-runtime", "build-windows", "Release", "avx_runtime.exe"),
                Path.Combine(here, "..", "avx_runtime.exe"),
                Path.Combine(here, "avx_runtime.exe")
            };
            string runtimePath = ""; foreach (string p in candidates) if (File.Exists(p)) { runtimePath = Path.GetFullPath(p); break; }
            if (runtimePath == "") { WriteLog("Runtime próprio selecionado, mas avx_runtime.exe não está compilado."); MessageBox.Show(this, "O runtime próprio ainda precisa ser compilado no Windows. Execute avx-runtime\\Build-Windows.bat e tente novamente.", "Runtime experimental", MessageBoxButtons.OK, MessageBoxIcon.Information); return; }
            string shimPath = Path.Combine(Path.GetDirectoryName(runtimePath), "avx_winshim.dll");
            bool canLaunchTarget = File.Exists(shimPath) && File.Exists(targetPath.Text);
            try
            {
                string arguments = canLaunchTarget ? "--run-process " + Quote(targetPath.Text) + " " + Quote(shimPath) : "--self-test";
                ProcessStartInfo start = new ProcessStartInfo { FileName = runtimePath, Arguments = arguments, WorkingDirectory = canLaunchTarget ? Path.GetDirectoryName(targetPath.Text) : Path.GetDirectoryName(runtimePath), UseShellExecute = false, CreateNoWindow = false };
                Process proc = Process.Start(start); if (proc == null) throw new InvalidOperationException("Process.Start returned no process");
                if (canLaunchTarget) { WriteLog("Runtime próprio iniciou o processo com shim AVX. PID host=" + proc.Id + " target=" + targetPath.Text); status.Text = "Processo iniciado"; }
                else { WriteLog("Runtime próprio iniciou o self-test. Shim ou target ausente; nenhum jogo foi iniciado. PID=" + proc.Id); status.Text = "Self-test executado"; MessageBox.Show(this, "O runtime próprio executou o self-test. Para iniciar um EXE pelo backend próprio, o target e avx_winshim.dll precisam estar disponíveis.\n\nO shim atual cobre somente um subconjunto experimental de AVX1 VEX128 register-only. O Intel SDE permanece disponível como fallback.", "Backend próprio experimental", MessageBoxButtons.OK, MessageBoxIcon.Information); }
            }
            catch (Exception ex) { WriteLog("Falha no runtime: " + ex.Message); }
        }

        private void SaveProfile(object sender, EventArgs e)
        {
            if (!ValidateCommon()) return; string name = Path.GetFileNameWithoutExtension(targetPath.Text); foreach (char c in Path.GetInvalidFileNameChars()) name = name.Replace(c, '_');
            try { string path = Path.Combine(profileDir, name + ".profile"); File.WriteAllText(path, CaptureSettings().Serialize(), Encoding.UTF8); WriteLog("Perfil salvo: " + path); status.Text = "Perfil salvo"; }
            catch (Exception ex) { WriteLog("Falha ao salvar perfil: " + ex.Message); MessageBox.Show(this, ex.Message, "Falha ao salvar perfil", MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }
        private void LoadProfile(object sender, EventArgs e)
        {
            using (OpenFileDialog d = new OpenFileDialog()) { d.InitialDirectory = profileDir; d.Filter = "Perfis SDE AVX Lab|*.profile|Todos|*.*"; if (d.ShowDialog(this) != DialogResult.OK) return; try { Apply(LauncherSettings.Parse(File.ReadAllText(d.FileName, Encoding.UTF8))); WriteLog("Perfil carregado: " + d.FileName); } catch (Exception ex) { WriteLog("Falha ao carregar perfil: " + ex.Message); MessageBox.Show(this, ex.Message, "Falha ao carregar perfil", MessageBoxButtons.OK, MessageBoxIcon.Error); } }
        }
        private LauncherSettings CaptureSettings()
        {
            CpuProfile p = profileBox.SelectedItem as CpuProfile; LauncherSettings s = new LauncherSettings(); s.SdePath = sdePath.Text; s.TargetPath = targetPath.Text; s.WorkingDirectory = workPath.Text; s.Arguments = targetArgs.Text; s.Profile = p == null ? "-snb" : p.Switch; s.Backend = backendBox.Text; s.Mix = mixBox.Checked; s.XInUse = xinuseBox.Checked; return s;
        }
        private void Apply(LauncherSettings s)
        {
            sdePath.Text = s.SdePath; targetPath.Text = s.TargetPath; workPath.Text = s.WorkingDirectory; targetArgs.Text = s.Arguments; mixBox.Checked = s.Mix; xinuseBox.Checked = s.XInUse;
            for (int i = 0; i < profileBox.Items.Count; i++) if (((CpuProfile)profileBox.Items[i]).Switch == s.Profile) profileBox.SelectedIndex = i;
            if (!String.IsNullOrWhiteSpace(s.Backend)) backendBox.SelectedIndex = s.Backend.IndexOf("Intel SDE", StringComparison.OrdinalIgnoreCase) >= 0 ? 1 : 0;
        }
        private void Diagnose(object sender, EventArgs e)
        {
            try { StringBuilder b = new StringBuilder(); b.AppendLine("SDE AVX Lab diagnóstico"); b.AppendLine("Data: " + DateTime.Now); b.AppendLine("OS: " + Environment.OSVersion); b.AppendLine("64-bit OS: " + Environment.Is64BitOperatingSystem); b.AppendLine("SDE: " + sdePath.Text + " => " + File.Exists(sdePath.Text)); b.AppendLine("Jogo: " + targetPath.Text + " => " + File.Exists(targetPath.Text)); b.AppendLine("Backend: " + backendBox.Text); b.AppendLine("Perfil: " + ((CpuProfile)profileBox.SelectedItem).Switch); string path = Path.Combine(appData, "diagnostic-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".txt"); File.WriteAllText(path, b.ToString(), Encoding.UTF8); WriteLog("Diagnóstico salvo: " + path); MessageBox.Show(this, b.ToString() + "\nArquivo: " + path, "Diagnóstico", MessageBoxButtons.OK, MessageBoxIcon.Information); }
            catch (Exception ex) { WriteLog("Falha ao gerar diagnóstico: " + ex.Message); MessageBox.Show(this, ex.Message, "Falha no diagnóstico", MessageBoxButtons.OK, MessageBoxIcon.Error); }
        }
        private void OpenLogs(object sender, EventArgs e) { Directory.CreateDirectory(appData); Process.Start(new ProcessStartInfo { FileName = appData, UseShellExecute = true }); }
        private void WriteLog(string text)
        {
            string line = "[" + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + "] " + text + Environment.NewLine; try { Directory.CreateDirectory(appData); File.AppendAllText(Path.Combine(appData, "launcher.log"), line, Encoding.UTF8); } catch { } if (eventLog != null) { eventLog.AppendText(line); eventLog.SelectionStart = eventLog.Text.Length; eventLog.ScrollToCaret(); }
        }
        private static string Quote(string value) { return "\"" + (value ?? "").Replace("\"", "\\\"") + "\""; }
    }

    internal static class Program
    {
        [STAThread]
        private static void Main() { Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false); Application.Run(new MainForm()); }
    }
}
